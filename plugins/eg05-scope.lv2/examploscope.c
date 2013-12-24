/*
  Copyright 2013 Robin Gareus <robin@gareus.org>

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

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/log/log.h"
#include "lv2/lv2plug.in/ns/ext/log/logger.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "./uris.h"

/** Private plugin instance structure. */
typedef struct {
	// Port buffers
	float*                   input[2];
	float*                   output[2];
	const LV2_Atom_Sequence* control;
	LV2_Atom_Sequence*       notify;

	// Atom forge and URI mapping
	LV2_URID_Map*        map;
	ScoLV2URIs           uris;
	LV2_Atom_Forge       forge;
	LV2_Atom_Forge_Frame frame;

	// Log feature and convenience API
	LV2_Log_Log*   log;
	LV2_Log_Logger logger;

	// Instantiation settings
	uint32_t n_channels;
	double   rate;

	/* The state of the UI is stored here, so that the GUI can be displayed and
	   closed without losing the current settings.  It is communicated to the
	   UI using atom messages.
	*/
	bool     ui_active;
	bool     send_settings_to_ui;
	float    ui_amp;
	uint32_t ui_spp;
} EgScope;

/** Port indices. */
typedef enum {
	SCO_CONTROL = 0,
	SCO_NOTIFY  = 1,
	SCO_INPUT0  = 2,
	SCO_OUTPUT0 = 3,
	SCO_INPUT1  = 4,
	SCO_OUTPUT1 = 5,
} PortIndex;

/** Create plugin instance. */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	(void)descriptor;   // Unused variable
	(void)bundle_path;  // Unused variable

	// Allocate and initialise instance structure.
	EgScope* self = (EgScope*)calloc(1, sizeof(EgScope));
	if (!self) {
		return NULL;
	}

	// Get host features
	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID__map)) {
			self->map = (LV2_URID_Map*)features[i]->data;
		} else if (!strcmp(features[i]->URI, LV2_LOG__log)) {
			self->log = (LV2_Log_Log*)features[i]->data;
		}
	}

	if (!self->map) {
		fprintf(stderr, "EgScope.lv2 error: Host does not support urid:map\n");
		free(self);
		return NULL;
	}

	// Decide which variant to use depending on the plugin URI
	if (!strcmp(descriptor->URI, SCO_URI "#Stereo")) {
		self->n_channels = 2;
	} else if (!strcmp(descriptor->URI, SCO_URI "#Mono")) {
		self->n_channels = 1;
	} else {
		free(self);
		return NULL;
	}

	// Initialise local variables
	self->ui_active           = false;
	self->send_settings_to_ui = false;
	self->rate                = rate;

	// Set default UI settings
	self->ui_spp = 50;
	self->ui_amp = 1.0;

	// Map URIs and initialise forge/logger
	map_sco_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);
	lv2_log_logger_init(&self->logger, self->map, self->log);

	return (LV2_Handle)self;
}

/** Connect a port to a buffer. */
static void
connect_port(LV2_Handle handle,
             uint32_t   port,
             void*      data)
{
	EgScope* self = (EgScope*)handle;

	switch ((PortIndex)port) {
	case SCO_CONTROL:
		self->control = (const LV2_Atom_Sequence*)data;
		break;
	case SCO_NOTIFY:
		self->notify = (LV2_Atom_Sequence*)data;
		break;
	case SCO_INPUT0:
		self->input[0] = (float*)data;
		break;
	case SCO_OUTPUT0:
		self->output[0] = (float*)data;
		break;
	case SCO_INPUT1:
		self->input[1] = (float*)data;
		break;
	case SCO_OUTPUT1:
		self->output[1] = (float*)data;
		break;
	}
}

/**
   Forge vector of raw data.

   @param forge Forge to use.
   @param uris Mapped URI identifiers.
   @param channel Channel ID to transmit.
   @param n_samples Number of audio samples to transmit.
   @param data Actual audio data.
*/
static void
tx_rawaudio(LV2_Atom_Forge* forge,
            ScoLV2URIs*     uris,
            const int32_t   channel,
            const size_t    n_samples,
            void*           data)
{
	LV2_Atom_Forge_Frame frame;

	// Forge container object of type 'rawaudio'
	lv2_atom_forge_frame_time(forge, 0);
	lv2_atom_forge_blank(forge, &frame, 1, uris->RawAudio);

	// Add integer attribute 'channelid'
	lv2_atom_forge_property_head(forge, uris->channelID, 0);
	lv2_atom_forge_int(forge, channel);

	// Add vector of floats raw 'audiodata'
	lv2_atom_forge_property_head(forge, uris->audioData, 0);
	lv2_atom_forge_vector(
		forge, sizeof(float), uris->atom_Float, n_samples, data);

	// Close off atom-object
	lv2_atom_forge_pop(forge, &frame);
}

/** Process a block of audio */
static void
run(LV2_Handle handle, uint32_t n_samples)
{
	EgScope* self = (EgScope*)handle;

	/* Ensure notify port buffer is large enough to hold all audio-samples and
	   configuration settings.  A minimum size was requested in the .ttl file,
	   but check here just to be sure.

	   TODO: Explain these magic numbers.
	*/
	const size_t   size  = (sizeof(float) * n_samples + 64) * self->n_channels;
	const uint32_t space = self->notify->atom.size;
	if (space < size + 128) {
		/* Insufficient space, report error and do nothing.  Note that a
		   real-time production plugin mustn't call log functions in run(), but
		   this can be useful for debugging and example purposes.
		*/
		lv2_log_error(&self->logger, "Buffer size is insufficient\n");
		return;
	}

	// Prepare forge buffer and initialize atom-sequence
	lv2_atom_forge_set_buffer(&self->forge, (uint8_t*)self->notify, space);
	lv2_atom_forge_sequence_head(&self->forge, &self->frame, 0);

	/* Send settings to UI

	   The plugin can continue to run while the UI is closed and re-opened.
	   The state and settings of the UI are kept here and transmitted to the UI
	   every time it asks for them or if the user initializes a 'load preset'.
	*/
	if (self->send_settings_to_ui && self->ui_active) {
		self->send_settings_to_ui = false;
		// Forge container object of type 'ui_state'
		LV2_Atom_Forge_Frame frame;
		lv2_atom_forge_frame_time(&self->forge, 0);
		lv2_atom_forge_blank(&self->forge, &frame, 1, self->uris.ui_State);

		// Add UI state as properties
		lv2_atom_forge_property_head(&self->forge, self->uris.ui_spp, 0);
		lv2_atom_forge_int(&self->forge, self->ui_spp);
		lv2_atom_forge_property_head(&self->forge, self->uris.ui_amp, 0);
		lv2_atom_forge_float(&self->forge, self->ui_amp);
		lv2_atom_forge_property_head(&self->forge, self->uris.param_sampleRate, 0);
		lv2_atom_forge_float(&self->forge, self->rate);
		lv2_atom_forge_pop(&self->forge, &frame);
	}

	// Process incoming events from GUI
	if (self->control) {
		const LV2_Atom_Event* ev = lv2_atom_sequence_begin(
			&(self->control)->body);
		// For each incoming message...
		while (!lv2_atom_sequence_is_end(
			       &self->control->body, self->control->atom.size, ev)) {
			// If the event is an atom:Blank object
			if (ev->body.type == self->uris.atom_Blank) {
				const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
				if (obj->body.otype == self->uris.ui_On) {
					// If the object is a ui-on, the UI was activated
					self->ui_active           = true;
					self->send_settings_to_ui = true;
				} else if (obj->body.otype == self->uris.ui_Off) {
					// If the object is a ui-off, the UI was closed
					self->ui_active = false;
				} else if (obj->body.otype == self->uris.ui_State) {
					// If the object is a ui-state, it's the current UI settings
					const LV2_Atom* spp = NULL;
					const LV2_Atom* amp = NULL;
					lv2_atom_object_get(obj, self->uris.ui_spp, &spp,
					                    self->uris.ui_amp, &amp,
					                    0);
					if (spp) {
						self->ui_spp = ((const LV2_Atom_Int*)spp)->body;
					}
					if (amp) {
						self->ui_amp = ((const LV2_Atom_Float*)amp)->body;
					}
				}
			}
			ev = lv2_atom_sequence_next(ev);
		}
	}

	// Process audio data
	for (uint32_t c = 0; c < self->n_channels; ++c) {
		if (self->ui_active) {
			// If UI is active, send raw audio data to UI
			tx_rawaudio(&self->forge, &self->uris, c, n_samples, self->input[c]);
		}
		// If not processing audio in-place, forward audio
		if (self->input[c] != self->output[c]) {
			memcpy(self->output[c], self->input[c], sizeof(float) * n_samples);
		}
	}

	// Close off sequence
	lv2_atom_forge_pop(&self->forge, &self->frame);
}

static void
cleanup(LV2_Handle handle)
{
	free(handle);
}

static LV2_State_Status
state_save(LV2_Handle                instance,
           LV2_State_Store_Function  store,
           LV2_State_Handle          handle,
           uint32_t                  flags,
           const LV2_Feature* const* features)
{
	EgScope* self = (EgScope*)instance;
	if (!self) {
		return LV2_STATE_SUCCESS;
	}

	/* Store state values.  Note these values are POD, but not portable, since
	   different machines may have a different integer endianness or floating
	   point format.  However, since standard Atom types are used, a good host
	   will be able to save them portably as text anyway. */

	store(handle, self->uris.ui_spp,
	      (void*)&self->ui_spp, sizeof(uint32_t),
	      self->uris.atom_Int,
	      LV2_STATE_IS_POD);

	store(handle, self->uris.ui_amp,
	      (void*)&self->ui_amp, sizeof(float),
	      self->uris.atom_Float,
	      LV2_STATE_IS_POD);

	return LV2_STATE_SUCCESS;
}

static LV2_State_Status
state_restore(LV2_Handle                  instance,
              LV2_State_Retrieve_Function retrieve,
              LV2_State_Handle            handle,
              uint32_t                    flags,
              const LV2_Feature* const*   features)
{
	EgScope* self = (EgScope*)instance;

	size_t   size;
	uint32_t type;
	uint32_t valflags;

	const void* spp = retrieve(
		handle, self->uris.ui_spp, &size, &type, &valflags);
	if (spp && size == sizeof(uint32_t) && type == self->uris.atom_Int) {
		self->ui_spp              = *((const uint32_t*)spp);
		self->send_settings_to_ui = true;
	}

	const void* amp = retrieve(
		handle, self->uris.ui_amp, &size, &type, &valflags);
	if (amp && size == sizeof(float) && type == self->uris.atom_Float) {
		self->ui_amp              = *((const float*)amp);
		self->send_settings_to_ui = true;
	}

	return LV2_STATE_SUCCESS;
}

static const void*
extension_data(const char* uri)
{
	static const LV2_State_Interface state = { state_save, state_restore };
	if (!strcmp(uri, LV2_STATE__interface)) {
		return &state;
	}
	return NULL;
}

static const LV2_Descriptor descriptor_mono = {
	SCO_URI "#Mono",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

static const LV2_Descriptor descriptor_stereo = {
	SCO_URI "#Stereo",
	instantiate,
	connect_port,
	NULL,
	run,
	NULL,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor_mono;
	case 1:
		return &descriptor_stereo;
	default:
		return NULL;
	}
}
