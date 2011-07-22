/*
  LV2 Sampler Example Plugin
  Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>,
                 James Morris <jwm.art.net@gmail.com>,
                 David Robillard <d@drobilla.net>

  Permission to use, copy, modify, and/or distribute this software for any
  purpose with or without fee is hereby granted, provided that the above
  copyright notice and this permission notice appear in all copies.

  THIS SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
  WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
  MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
  ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
  WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
  ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
  OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
*/

/**
   @file sampler.c Sampler Plugin

   A simple example of an LV2 sampler that dynamically loads samples (based on
   incoming events) and also triggers their playback (based on incoming MIDI
   note events).  The sample must be monophonic.

   So that the runSampler() method stays real-time safe, the plugin creates a
   worker thread (worker_thread_main) that listens for file loading events.  It
   loads everything in plugin->pending_samp and then signals the runSampler()
   that it's time to install it.  runSampler() just has to swap pointers... so
   the change happens very fast and atomically.
*/

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <pthread.h>

#include <sndfile.h>

#include "lv2/lv2plug.in/ns/ext/event/event-helpers.h"
#include "lv2/lv2plug.in/ns/ext/persist/persist.h"
#include "lv2/lv2plug.in/ns/ext/uri-map/uri-map.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define NS_ATOM "http://lv2plug.in/ns/ext/atom#"

#define SAMPLER_URI    "http://lv2plug.in/plugins/eg-sampler"
#define MIDI_EVENT_URI "http://lv2plug.in/ns/ext/midi#MidiEvent"
#define FILENAME_URI   SAMPLER_URI "#filename"
#define STRING_BUF     8192

enum {
	SAMPLER_CONTROL = 0,
	SAMPLER_OUT     = 1
};

static const char* default_sample_file = "monosample.wav";

typedef struct {
	char    filepath[STRING_BUF];
	SF_INFO info;
	float*  data;
} SampleFile;

typedef struct {
	/* Features */
	LV2_URI_Map_Feature* uri_map;

	/* Sample */
	SampleFile*     samp;
	SampleFile*     pending_samp;
	pthread_mutex_t pending_samp_mutex;  /**< Protects pending_samp */
	pthread_cond_t  pending_samp_cond;   /**< Signaling mechanism */
	int             pending_sample_ready;

	/* Ports */
	float*             outputPort;
	LV2_Event_Buffer*  eventPort;
	LV2_Event_Feature* event_ref;
	int                midi_event_id;

	/* Playback state */
	bool       play;
	sf_count_t frame;

	/* File loading */
	pthread_t worker_thread;
} Sampler;

static void
handle_load_sample(Sampler* plugin)
{
	plugin->pending_sample_ready = 0;

	SF_INFO* const info   = &plugin->pending_samp->info;
	SNDFILE* const sample = sf_open(plugin->pending_samp->filepath,
	                                SFM_READ,
	                                info);

	if (!sample
	    || !info->frames
	    || (info->channels != 1)) {
		fprintf(stderr, "failed to open sample '%s'.\n",
		        plugin->pending_samp->filepath);
		return;
	}

	/* Read data */
	float* const data = malloc(sizeof(float) * info->frames);
	plugin->pending_samp->data = data;

	if (!data) {
		fprintf(stderr, "failed to allocate memory for sample.\n");
		return;
	}

	sf_seek(sample, 0ul, SEEK_SET);
	sf_read_float(sample,
	              data,
	              info->frames);
	sf_close(sample);

	/* Queue the sample for installation on next run() */
	plugin->pending_sample_ready = 1;
}

void*
worker_thread_main(void* arg)
{
	Sampler* plugin = (Sampler*)arg;

	pthread_mutex_lock(&plugin->pending_samp_mutex);
	while (true) {
		/* Wait for run() to signal that we need to load a sample */
		pthread_cond_wait(&plugin->pending_samp_cond,
		                  &plugin->pending_samp_mutex);

		/* Then load it */
		handle_load_sample(plugin);
	}
	pthread_mutex_unlock(&plugin->pending_samp_mutex);

	return 0;
}

static void
cleanup(LV2_Handle instance)
{
	Sampler* plugin = (Sampler*)instance;
	pthread_cancel(plugin->worker_thread);
	pthread_join(plugin->worker_thread, 0);

	free(plugin->samp->data);
	free(plugin->pending_samp->data);
	free(plugin->samp);
	free(plugin->pending_samp);
	free(instance);
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Sampler* plugin = (Sampler*)instance;

	switch (port) {
	case SAMPLER_CONTROL:
		plugin->eventPort = (LV2_Event_Buffer*)data;
		break;
	case SAMPLER_OUT:
		plugin->outputPort = (float*)data;
		break;
	default:
		break;
	}
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
	Sampler* plugin = (Sampler*)malloc(sizeof(Sampler));
	assert(plugin);
	memset(plugin, 0, sizeof(Sampler));

	plugin->samp = (SampleFile*)malloc(sizeof(SampleFile));
	assert(plugin->samp);
	memset(plugin->samp, 0, sizeof(SampleFile));
	plugin->pending_samp = (SampleFile*)malloc(sizeof(SampleFile));
	assert(plugin->pending_samp);
	memset(plugin->pending_samp, 0, sizeof(SampleFile));

	plugin->midi_event_id = -1;
	plugin->event_ref     = 0;

	/* Initialise mutexes and conditions for the worker thread */
	if (pthread_mutex_init(&plugin->pending_samp_mutex, 0)) {
		fprintf(stderr, "Could not initialize next_sample_mutex.\n");
		goto fail;
	}
	if (pthread_cond_init(&plugin->pending_samp_cond, 0)) {
		fprintf(stderr, "Could not initialize next_sample_waitcond.\n");
		goto fail;
	}
	if (pthread_create(&plugin->worker_thread, 0, worker_thread_main, plugin)) {
		fprintf(stderr, "Could not initialize worker thread.\n");
		goto fail;
	}

	/* Scan host features for event and uri-map */
	for (int i = 0; features[i]; ++i) {
		if (strcmp(features[i]->URI, LV2_URI_MAP_URI) == 0) {
			plugin->uri_map = (LV2_URI_Map_Feature*)features[i]->data;
			plugin->midi_event_id = plugin->uri_map->uri_to_id(
				plugin->uri_map->callback_data,
				LV2_EVENT_URI, MIDI_EVENT_URI);
		} else if (strcmp(features[i]->URI, LV2_EVENT_URI) == 0) {
			plugin->event_ref = (LV2_Event_Feature*)features[i]->data;
		}
	}

	if (plugin->midi_event_id == -1) {
		/* Host does not support uri-map extension */
		fprintf(stderr, "Host does not support uri-map extension.\n");
		goto fail;
	}

	/* Open the default sample file */
	strncpy(plugin->pending_samp->filepath, path, STRING_BUF);
	strncat(plugin->pending_samp->filepath,
	        default_sample_file,
	        STRING_BUF - strlen(plugin->pending_samp->filepath) );
	handle_load_sample(plugin);

	return (LV2_Handle)plugin;

fail:
	free(plugin);
	return 0;
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sampler*   plugin = (Sampler*)instance;
	LV2_Event* ev     = NULL;

	sf_count_t start_frame = 0;
	sf_count_t pos         = 0;
	float*     output      = plugin->outputPort;

	/* Read incoming events */
	LV2_Event_Iterator iterator;
	for (lv2_event_begin(&iterator, plugin->eventPort);
	     lv2_event_is_valid(&iterator);
	     lv2_event_increment(&iterator)) {

		ev = lv2_event_get(&iterator, NULL);

		if (ev->type == 0) {
			if (plugin->event_ref) {
				plugin->event_ref->lv2_event_unref(
					plugin->event_ref->callback_data, ev);
			}
		} else if (ev->type == plugin->midi_event_id) {
			uint8_t* const data = (uint8_t* const)(ev + 1);

			if ((data[0] & 0xF0) == 0x90) {
				start_frame   = ev->frames;
				plugin->frame = 0;
				plugin->play  = true;
			}
		}
		/***************************************************
		 * XXX TODO:                                       *
		 * ADD CODE HERE TO DETECT AN INCOMING MESSAGE TO  *
		 * DYNAMICALLY LOAD A SAMPLE                       *
		 ***************************************************
		 */
		else if (0) {
			/* message to load a sample comes in */
			/* write filename to plugin->pending_samp->filepath */
			/* strncpy(plugin->pending_samp->filepath, some_src_string, STRING_BUF); */
			pthread_cond_signal(&plugin->pending_samp_cond);
		}
	}

	/* Render the sample (possibly already in progress) */
	if (plugin->play) {
		uint32_t       f  = plugin->frame;
		const uint32_t lf = plugin->samp->info.frames;

		for (pos = 0; pos < start_frame; ++pos) {
			output[pos] = 0;
		}

		for (; pos < sample_count && f < lf; ++pos, ++f) {
			output[pos] = plugin->samp->data[f];
		}

		plugin->frame = f;

		if (f == lf) {
			plugin->play = false;
		}
	}

	/* Check if we have a sample pending */
	if (!plugin->play
	    && plugin->pending_sample_ready
	    && pthread_mutex_trylock(&plugin->pending_samp_mutex)) {
		/* Install the new sample */
		SampleFile* tmp;
		tmp                          = plugin->samp;
		plugin->samp                 = plugin->pending_samp;
		plugin->pending_samp         = tmp;
		plugin->pending_sample_ready = 0;
		free(plugin->pending_samp->data);

		pthread_mutex_unlock(&plugin->pending_samp_mutex);
	}

	/* Add zeros to end if sample not long enough (or not playing) */
	for (; pos < sample_count; ++pos) {
		output[pos] = 0;
	}
}

static uint32_t
uri_to_id(Sampler* plugin, const char* uri)
{
	return plugin->uri_map->uri_to_id(plugin->uri_map->callback_data,
	                                  NULL,
	                                  uri);
}

static void
save(LV2_Handle                 instance,
     LV2_Persist_Store_Function store,
     void*                      callback_data)
{
	Sampler* plugin = (Sampler*)instance;
	store(callback_data,
	      uri_to_id(plugin, FILENAME_URI),
	      plugin->samp->filepath,
	      strlen(plugin->samp->filepath) + 1,
	      uri_to_id(plugin, NS_ATOM "String"),
	      LV2_PERSIST_IS_POD | LV2_PERSIST_IS_PORTABLE);
}

static void
restore(LV2_Handle                    instance,
        LV2_Persist_Retrieve_Function retrieve,
        void*                         callback_data)
{
	Sampler* plugin = (Sampler*)instance;

	size_t   size;
	uint32_t type;
	uint32_t flags;

	const void* value = retrieve(
		callback_data,
		uri_to_id(plugin, FILENAME_URI),
		&size, &type, &flags);

	if (value) {
		printf("Restored filename %s\n", (const char*)value);
	}
}

const void*
extension_data(const char* uri)
{
	static const LV2_Persist persist = { save, restore };
	if (!strcmp(uri, LV2_PERSIST_URI)) {
		return &persist;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	SAMPLER_URI,
	instantiate,
	connect_port,
	NULL, // activate,
	run,
	NULL, // deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor* lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
