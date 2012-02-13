/*
  LV2 Sampler Example Plugin
  Copyright 2011-2012 David Robillard <d@drobilla.net>
  Copyright 2011 Gabriel M. Beddingfield <gabriel@teuton.org>
  Copyright 2011 James Morris <jwm.art.net@gmail.com>

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

   So that the run() method stays real-time safe, the plugin creates a worker
   thread (worker_thread_main) that listens for file loading events.  It loads
   everything in plugin->pending_samp and then signals the run() that it's time
   to install it.  run() just has to swap pointers... so the change happens
   very fast and atomically.
*/

#include <assert.h>
#include <math.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>

#include <sndfile.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/message/message.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "zix/sem.h"
#include "zix/thread.h"

#include "./uris.h"

#define STRING_BUF 8192

enum {
	SAMPLER_CONTROL  = 0,
	SAMPLER_RESPONSE = 1,
	SAMPLER_OUT      = 2
};

static const char* default_sample_file = "monosample.wav";

typedef struct {
	char    filepath[STRING_BUF];
	SF_INFO info;
	float*  data;
} SampleFile;

typedef struct {
	/* Features */
	LV2_URID_Map* map;

	/* Worker thread */
	ZixThread worker_thread;
	ZixSem    signal;
	bool      exit;

	/* Sample */
	SampleFile* samp;
	SampleFile* pending_samp;
	int         pending_sample_ready;

	/* Ports */
	float*             output_port;
	LV2_Atom_Sequence* control_port;
	LV2_Atom_Sequence* response_port;

	/* URIs */
	SamplerURIs uris;

	/* Playback state */
	sf_count_t frame;
	bool       play;

} Sampler;

static void
handle_load_sample(Sampler* plugin)
{
	plugin->pending_sample_ready = 0;

	printf("Loading sample %s\n", plugin->pending_samp->filepath);
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
	sf_read_float(sample, data, info->frames);
	sf_close(sample);

	/* Queue the sample for installation on next run() */
	plugin->pending_sample_ready = 1;
}

void*
worker_thread_main(void* arg)
{
	Sampler* plugin = (Sampler*)arg;

	while (!plugin->exit) {
		/* Wait for run() to signal that we need to load a sample */
		zix_sem_wait(&plugin->signal);

		/* Then load it */
		handle_load_sample(plugin);
	}

	return 0;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Sampler* plugin = (Sampler*)instance;

	switch (port) {
	case SAMPLER_CONTROL:
		plugin->control_port = (LV2_Atom_Sequence*)data;
		break;
	case SAMPLER_RESPONSE:
		plugin->response_port = (LV2_Atom_Sequence*)data;
		break;
	case SAMPLER_OUT:
		plugin->output_port = (float*)data;
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
	if (!plugin) {
		return NULL;
	}

	plugin->samp         = (SampleFile*)malloc(sizeof(SampleFile));
	plugin->pending_samp = (SampleFile*)malloc(sizeof(SampleFile));
	if (!plugin->samp || !plugin->pending_samp) {
		return NULL;
	}

	memset(plugin->samp, 0, sizeof(SampleFile));
	memset(plugin->pending_samp, 0, sizeof(SampleFile));
	memset(&plugin->uris, 0, sizeof(plugin->uris));

	/* Create signal for waking up worker thread */
	if (zix_sem_init(&plugin->signal, 0)) {
		fprintf(stderr, "Could not initialize semaphore.\n");
		goto fail;
	}

	/* Create worker thread */
	plugin->exit = false;
	if (zix_thread_create(
		    &plugin->worker_thread, 1024, worker_thread_main, plugin)) {
		fprintf(stderr, "Could not initialize worker thread.\n");
		goto fail;
	}

	/* Scan host features for URID map */
	LV2_URID_Map* map = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!map) {
		fprintf(stderr, "Host does not support urid:map.\n");
		goto fail;
	}

	plugin->map = map;
	map_sampler_uris(plugin->map, &plugin->uris);

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
cleanup(LV2_Handle instance)
{
	Sampler* plugin = (Sampler*)instance;

	plugin->exit = true;
	zix_sem_post(&plugin->signal);
	zix_thread_join(plugin->worker_thread, 0);
	zix_sem_destroy(&plugin->signal);

	free(plugin->samp->data);
	free(plugin->pending_samp->data);
	free(plugin->samp);
	free(plugin->pending_samp);
	free(instance);
}

static bool
handle_message(Sampler*               plugin,
               const LV2_Atom_Object* obj)
{
	if (obj->type != plugin->uris.msg_Set) {
		fprintf(stderr, "Ignoring unknown message type %d\n", obj->type);
		return false;
	}

	/* Get body of message */
	const LV2_Atom_Object* body = NULL;
	LV2_Atom_Object_Query q1[] = {
		{ plugin->uris.msg_body, (const LV2_Atom**)&body },
		LV2_OBJECT_QUERY_END
	};
	lv2_object_get(obj, q1);

	if (!body) {  // TODO: check type
		fprintf(stderr, "Malformed set message with no body.\n");
		return false;
	}

	/* Get filename from body */
	const LV2_Atom* filename = NULL;
	LV2_Atom_Object_Query q2[] = {
		{ plugin->uris.eg_filename, &filename },
		LV2_OBJECT_QUERY_END
	};
	lv2_object_get((LV2_Atom_Object*)body, q2);

	if (!filename) {
		fprintf(stderr, "Ignored set message with no filename\n");
		return false;
	}

	char* str = (char*)LV2_ATOM_BODY(filename);
	fprintf(stderr, "Request to load %s\n", str);
	memcpy(plugin->pending_samp->filepath, str, filename->size);
	zix_sem_post(&plugin->signal);

	return true;
}
               
static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sampler*   plugin      = (Sampler*)instance;
	sf_count_t start_frame = 0;
	sf_count_t pos         = 0;
	float*     output      = plugin->output_port;

	/* Read incoming events */
	LV2_SEQUENCE_FOREACH(plugin->control_port, i) {
		LV2_Atom_Event* const ev = lv2_sequence_iter_get(i);
		if (ev->body.type == plugin->uris.midi_Event) {
			uint8_t* const data = (uint8_t* const)(ev + 1);
			if ((data[0] & 0xF0) == 0x90) {
				start_frame   = ev->time.audio.frames;
				plugin->frame = 0;
				plugin->play  = true;
			}
		} else if (ev->body.type == plugin->uris.atom_Resource
		           || ev->body.type == plugin->uris.atom_Blank) {
			handle_message(plugin, (LV2_Atom_Object*)&ev->body);
		} else {
			fprintf(stderr, "Unknown event type %d\n", ev->body.type);
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
	if (!plugin->play && plugin->pending_sample_ready) {
		/* Install the new sample */
		SampleFile* tmp = plugin->samp;
		plugin->samp                 = plugin->pending_samp;
		plugin->pending_samp         = tmp;
		plugin->pending_sample_ready = 0;
		free(plugin->pending_samp->data); // FIXME: non-realtime!
	}

	/* Add zeros to end if sample not long enough (or not playing) */
	for (; pos < sample_count; ++pos) {
		output[pos] = 0.0f;
	}
}

static uint32_t
map_uri(Sampler* plugin, const char* uri)
{
	return plugin->map->map(plugin->map->handle, uri);
}

static void
save(LV2_Handle                instance,
     LV2_State_Store_Function  store,
     void*                     callback_data,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	LV2_State_Map_Path* map_path = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_STATE_MAP_PATH_URI)) {
			map_path = (LV2_State_Map_Path*)features[i]->data;
		}
	}

	Sampler* plugin = (Sampler*)instance;
	char*    apath  = map_path->abstract_path(map_path->handle,
	                                          plugin->samp->filepath);

	store(callback_data,
	      map_uri(plugin, FILENAME_URI),
	      apath,
	      strlen(plugin->samp->filepath) + 1,
	      plugin->uris.state_Path,
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	free(apath);
}

static void
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        void*                       callback_data,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	Sampler* plugin = (Sampler*)instance;

	size_t   size;
	uint32_t type;
	uint32_t valflags;

	const void* value = retrieve(
		callback_data,
		map_uri(plugin, FILENAME_URI),
		&size, &type, &valflags);

	if (value) {
		printf("Restoring filename %s\n", (const char*)value);
		strncpy(plugin->pending_samp->filepath, value, STRING_BUF);
		handle_load_sample(plugin);
	}
}

const void*
extension_data(const char* uri)
{
	static const LV2_State_Interface state = { save, restore };
	if (!strcmp(uri, LV2_STATE_URI "#Interface")) {
		return &state;
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
