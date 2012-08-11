/*
  Copyright 2012 Harry van Haaren <harryhaaren@gmail.com>
  Copyright 2012 David Robillard <d@drobilla.net>

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
   @file synth.c Implementation of the LV2 Sin Synth example plugin.

   This is a simple LV2 synthesis plugin that demonstrates how to receive MIDI
   events and render audio in response to them.
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define SYNTH_URI "http://lv2plug.in/plugins/eg-synth"

/** Port indices. */
typedef enum {
	SYNTH_FREQ = 0,
	SYNTH_OUTPUT,
} PortIndex;

/** Plugin instance. */
typedef struct {
	// Sample rate, necessary to generate sin wave in run()
	double sample_rate;

	// Current wave phase
	float phase;

	// Port buffers
	const float* freq;
	float*       output;
} Synth;

/** Create a new plugin instance. */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Synth* self = (Synth*)malloc(sizeof(Synth));
	if (self) {
		// Store the sample rate so it is available in run()
		self->sample_rate = rate;
	}
	return (LV2_Handle)self;
}

/** Connect a port to a buffer (audio thread, must be RT safe). */
static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Synth* self = (Synth*)instance;

	switch ((PortIndex)port) {
	case SYNTH_FREQ:
		self->freq = (const float*)data;
		break;
	case SYNTH_OUTPUT:
		self->output = (float*)data;
		break;
	}
}

/** Initialise and prepare the plugin instance for running. */
static void
activate(LV2_Handle instance)
{
	Synth* self = (Synth*)instance;

	// Initialize phase so we start at the beginning of the wave
	self->phase = 0.0f;
}

/** Process a block of audio (audio thread, must be RT safe). */
static void
run(LV2_Handle instance, uint32_t n_samples)
{
	Synth* self = (Synth*)instance;

	const float  PI     = 3.1415;
	const float  volume = 0.3;
	const float  freq   = *(self->freq);
	float* const output = self->output;

	float samples_per_cycle = self->sample_rate / freq;

	/* Calculate the phase offset per sample.  Phase ranges from 0..1, so
	   phase_increment is a floating point number such that we get "freq"
	   number of cycles in "sample_rate" amount of samples. */
	float phase_increment = (1.f / samples_per_cycle);

	for (uint32_t pos = 0; pos < n_samples; pos++) {
		/* Calculate the next sample.  Phase ranges from 0..1, but sin()
		   expects its input in radians, so we multiply by 2 PI to convert it.
		   We also multiply by volume so it's not extremely loud. */
		output[pos] = sin(self->phase * 2 * PI) * volume;

		/* Increment the phase so we generate the next sample */
		self->phase += phase_increment;
		if (self->phase > 1.0f) {
			self->phase = 0.0f;
		}
	}
}

/** Finish running (counterpart to activate()). */
static void
deactivate(LV2_Handle instance)
{
	/* Nothing to do here in this trivial mostly stateless plugin. */
}

/** Destroy a plugin instance (counterpart to instantiate()). */
static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

/** Return extension data provided by the plugin. */
static const void*
extension_data(const char* uri)
{
	return NULL;  /* This plugin has no extension data. */
}

/** The LV2_Descriptor for this plugin. */
static const LV2_Descriptor descriptor = {
	SYNTH_URI,
	instantiate,
	connect_port,
	activate,
	run,
	deactivate,
	cleanup,
	extension_data
};

/** Entry point, the host will call this function to access descriptors. */
LV2_SYMBOL_EXPORT
const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
