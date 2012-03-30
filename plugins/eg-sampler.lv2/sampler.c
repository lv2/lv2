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

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/ext/worker/worker.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "./uris.h"

enum {
	SAMPLER_CONTROL  = 0,
	SAMPLER_RESPONSE = 1,
	SAMPLER_OUT      = 2
};

static const char* default_sample_file = "monosample.wav";

typedef struct {
	SF_INFO info;      /**< Info about sample from sndfile */
	float*  data;      /**< Sample data in float */
	char*   path;      /**< Path of file */
	size_t  path_len;  /**< Length of path */
} Sample;

typedef struct {
	/* Features */
	LV2_URID_Map*        map;
	LV2_Worker_Schedule* schedule;

	/* Forge for creating atoms */
	LV2_Atom_Forge forge;

	/* Sample */
	Sample* sample;

	/* Ports */
	float*               output_port;
	LV2_Atom_Sequence*   control_port;
	LV2_Atom_Sequence*   notify_port;
	LV2_Atom_Forge_Frame notify_frame;

	/* URIs */
	SamplerURIs uris;

	/* Current position in run() */
	uint32_t frame_offset;

	/* Playback state */
	sf_count_t frame;
	bool       play;
} Sampler;

/**
   An atom-like message used internally to apply/free samples.
 
   This is only used internally to communicate with the worker, it is not an
   Atom because it is not POD.
*/
typedef struct {
	LV2_Atom atom;
	Sample*  sample;
} SampleMessage;

static Sample*
load_sample(Sampler* plugin, const char* path)
{
	const size_t path_len  = strlen(path);

	printf("Loading sample %s\n", path);
	Sample* const  sample  = (Sample*)malloc(sizeof(Sample));
	SF_INFO* const info    = &sample->info;
	SNDFILE* const sndfile = sf_open(path, SFM_READ, info);

	if (!sndfile || !info->frames || (info->channels != 1)) {
		fprintf(stderr, "Failed to open sample '%s'.\n", path);
		free(sample);
		return NULL;
	}

	/* Read data */
	float* const data = malloc(sizeof(float) * info->frames);
	if (!data) {
		fprintf(stderr, "Failed to allocate memory for sample.\n");
		return NULL;
	}
	sf_seek(sndfile, 0ul, SEEK_SET);
	sf_read_float(sndfile, data, info->frames);
	sf_close(sndfile);

	/* Fill sample struct and return it. */
	sample->data     = data;
	sample->path     = (char*)malloc(path_len + 1);
	sample->path_len = path_len;
	memcpy(sample->path, path, path_len + 1);

	return sample;
}

static void
free_sample(Sample* sample)
{
	if (sample) {
		fprintf(stderr, "Freeing %s\n", sample->path);
		free(sample->path);
		free(sample->data);
		free(sample);
	}
}

/** Handle work (load or free a sample) in a non-realtime thread. */
static LV2_Worker_Status
work(LV2_Handle                  instance,
     LV2_Worker_Respond_Function respond,
     LV2_Worker_Respond_Handle   handle,
     uint32_t                    size,
     const void*                 data)
{
	Sampler*  self = (Sampler*)instance;
	LV2_Atom* atom = (LV2_Atom*)data;
	if (atom->type == self->uris.eg_freeSample) {
		/* Free old sample */
		SampleMessage* msg = (SampleMessage*)data;
		free_sample(msg->sample);
	} else {
		/* Handle set message (load sample). */
		LV2_Atom_Object* obj = (LV2_Atom_Object*)data;

		/* Get file path from message */
		const LV2_Atom* file_path = read_set_file(&self->uris, obj);
		if (!file_path) {
			return LV2_WORKER_ERR_UNKNOWN;
		}

		/* Load sample. */
		Sample* sample = load_sample(self, LV2_ATOM_BODY(file_path));
		if (sample) {
			/* Loaded sample, send it to run() to be applied. */
			respond(handle, sizeof(sample), &sample);
		}
	}

	return LV2_WORKER_SUCCESS;
}

/** Handle a response from work() in the audio thread. */
static LV2_Worker_Status
work_response(LV2_Handle  instance,
              uint32_t    size,
              const void* data)
{
	Sampler* self = (Sampler*)instance;

	SampleMessage msg = { { sizeof(Sample*), self->uris.eg_freeSample },
	                      self->sample };

	/* Send a message to the worker to free the current sample */
	self->schedule->schedule_work(self->schedule->handle, sizeof(msg), &msg);
	                         
	/* Install the new sample */
	self->sample = *(Sample**)data;

	/* Send a notification that we're using a new sample. */
	lv2_atom_forge_frame_time(&self->forge, self->frame_offset);
	write_set_file(&self->forge, &self->uris,
	               self->sample->path,
	               self->sample->path_len);

	return LV2_WORKER_SUCCESS;
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
		plugin->notify_port = (LV2_Atom_Sequence*)data;
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

	memset(plugin, 0, sizeof(Sampler));
	plugin->sample = (Sample*)malloc(sizeof(Sample));
	if (!plugin->sample) {
		return NULL;
	}

	memset(plugin->sample, 0, sizeof(Sample));

	/* Scan and store host features */
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			plugin->map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_WORKER__schedule)) {
			plugin->schedule = (LV2_Worker_Schedule*)features[i]->data;
		}
	}
	if (!plugin->map) {
		fprintf(stderr, "Host does not support urid:map.\n");
		goto fail;
	} else if (!plugin->schedule) {
		fprintf(stderr, "Host does not support work:schedule.\n");
		goto fail;
	}

	/* Map URIS and initialise forge */
	map_sampler_uris(plugin->map, &plugin->uris);
	lv2_atom_forge_init(&plugin->forge, plugin->map);

	/* Load the default sample file */
	const size_t path_len    = strlen(path);
	const size_t file_len    = strlen(default_sample_file);
	const size_t len         = path_len + file_len;
	char*        sample_path = (char*)malloc(len + 1);
	snprintf(sample_path, len + 1, "%s%s", path, default_sample_file);
	plugin->sample = load_sample(plugin, sample_path);

	return (LV2_Handle)plugin;

fail:
	free(plugin);
	return 0;
}

static void
cleanup(LV2_Handle instance)
{
	Sampler* plugin = (Sampler*)instance;

	free_sample(plugin->sample);
	free(plugin);
}

static void
run(LV2_Handle instance,
    uint32_t   sample_count)
{
	Sampler*     plugin      = (Sampler*)instance;
	SamplerURIs* uris        = &plugin->uris;
	sf_count_t   start_frame = 0;
	sf_count_t   pos         = 0;
	float*       output      = plugin->output_port;

	/* Set up forge to write directly to notify output port. */
	const uint32_t notify_capacity = plugin->notify_port->atom.size;
	lv2_atom_forge_set_buffer(&plugin->forge,
	                          (uint8_t*)plugin->notify_port,
	                          notify_capacity);

	/* Start a sequence in the notify output port. */
	lv2_atom_forge_sequence_head(&plugin->forge, &plugin->notify_frame, 0);

	/* Read incoming events */
	LV2_SEQUENCE_FOREACH(plugin->control_port, i) {
		LV2_Atom_Event* const ev = lv2_sequence_iter_get(i);
		plugin->frame_offset = ev->time.frames;
		if (ev->body.type == uris->midi_Event) {
			uint8_t* const data = (uint8_t* const)(ev + 1);
			if ((data[0] & 0xF0) == 0x90) {
				start_frame   = ev->time.frames;
				plugin->frame = 0;
				plugin->play  = true;
			}
		} else if (is_object_type(uris, ev->body.type)) {
			const LV2_Atom_Object* obj = (LV2_Atom_Object*)&ev->body;
			if (obj->body.otype == uris->patch_Set) {
				/* Received a set message, send it to the worker. */
				fprintf(stderr, "Queueing set message\n");
				plugin->schedule->schedule_work(plugin->schedule->handle,
				                                lv2_atom_total_size(&ev->body),
				                                &ev->body);
			} else {
				fprintf(stderr, "Unknown object type %d\n", obj->body.otype);
			}
		} else {
			fprintf(stderr, "Unknown event type %d\n", ev->body.type);
		}
	}

	/* Render the sample (possibly already in progress) */
	if (plugin->play) {
		uint32_t       f  = plugin->frame;
		const uint32_t lf = plugin->sample->info.frames;

		for (pos = 0; pos < start_frame; ++pos) {
			output[pos] = 0;
		}

		for (; pos < sample_count && f < lf; ++pos, ++f) {
			output[pos] = plugin->sample->data[f];
		}

		plugin->frame = f;

		if (f == lf) {
			plugin->play = false;
		}
	}

	/* Add zeros to end if sample not long enough (or not playing) */
	for (; pos < sample_count; ++pos) {
		output[pos] = 0.0f;
	}
}

static LV2_State_Status
save(LV2_Handle                instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	LV2_State_Map_Path* map_path = NULL;
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_STATE__mapPath)) {
			map_path = (LV2_State_Map_Path*)features[i]->data;
		}
	}

	Sampler* plugin = (Sampler*)instance;
	char*    apath  = map_path->abstract_path(map_path->handle,
	                                          plugin->sample->path);

	store(handle,
	      plugin->uris.eg_file,
	      apath,
	      strlen(plugin->sample->path) + 1,
	      plugin->uris.atom_Path,
	      LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	free(apath);

	return LV2_STATE_SUCCESS;
}

static LV2_State_Status
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	Sampler* plugin = (Sampler*)instance;

	size_t   size;
	uint32_t type;
	uint32_t valflags;

	const void* value = retrieve(
		handle,
		plugin->uris.eg_file,
		&size, &type, &valflags);

	if (value) {
		const char* path = (const char*)value;
		printf("Restoring file %s\n", path);
		free_sample(plugin->sample);
		plugin->sample = load_sample(plugin, path);
	}

	return LV2_STATE_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
	static const LV2_State_Interface  state  = { save, restore };
	static const LV2_Worker_Interface worker = { work, work_response, NULL };
	if (!strcmp(uri, LV2_STATE__interface)) {
		return &state;
	} else if (!strcmp(uri, LV2_WORKER__interface)) {
		return &worker;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	EG_SAMPLER_URI,
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
