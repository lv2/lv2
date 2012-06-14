/*
  Copyright 2006-2011 David Robillard <d@drobilla.net>
  Copyright 2006 Steve Harris <steve@plugin.org.uk>

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
   @file amp.c Implementation of the LV2 Amp example plugin.

   This is a basic working LV2 plugin, about as small as one can get.  It is
   useful as a skeleton to copy to build more advanced plugins.  See lv2.h for
   more detailed descriptions of the rules for the various functions.
*/

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define AMP_URI "http://lv2plug.in/plugins/eg-amp"

/** Port indices. */
typedef enum {
	AMP_GAIN   = 0,
	AMP_INPUT  = 1,
	AMP_OUTPUT = 2
} PortIndex;

/** Plugin instance. */
typedef struct {
	// Port buffers
	float* gain;
	float* input;
	float* output;
} Amp;

/** Create a new plugin instance. */
static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Amp* amp = (Amp*)malloc(sizeof(Amp));

	return (LV2_Handle)amp;
}

/** Connect a port to a buffer (audio thread, must be RT safe). */
static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Amp* amp = (Amp*)instance;

	switch ((PortIndex)port) {
	case AMP_GAIN:
		amp->gain = (float*)data;
		break;
	case AMP_INPUT:
		amp->input = (float*)data;
		break;
	case AMP_OUTPUT:
		amp->output = (float*)data;
		break;
	}
}

/** Initialise and prepare the plugin instance for running. */
static void
activate(LV2_Handle instance)
{
	/* Nothing to do here in this trivial mostly stateless plugin. */
}

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

/** Process a block of audio (audio thread, must be RT safe). */
static void
run(LV2_Handle instance, uint32_t n_samples)
{
	Amp* amp = (Amp*)instance;

	const float        gain   = *(amp->gain);
	const float* const input  = amp->input;
	float* const       output = amp->output;

	const float coef = DB_CO(gain);

	for (uint32_t pos = 0; pos < n_samples; pos++) {
		output[pos] = input[pos] * coef;
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
const void*
extension_data(const char* uri)
{
	return NULL;  /* This plugin has no extension data. */
}

/** The LV2_Descriptor for this plugin. */
static const LV2_Descriptor descriptor = {
	AMP_URI,
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
