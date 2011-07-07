/*
  LV2 Amp Example Plugin
  Copyright 2006-2011 Steve Harris, David Robillard.
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

#include <math.h>
#include <stdlib.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define AMP_URI    "http://lv2plug.in/plugins/eg-amp"
#define AMP_GAIN   0
#define AMP_INPUT  1
#define AMP_OUTPUT 2

typedef struct {
	float* gain;
	float* input;
	float* output;
} Amp;

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               bundle_path,
            const LV2_Feature* const* features)
{
	Amp* amp = (Amp*)malloc(sizeof(Amp));

	return (LV2_Handle)amp;
}

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Amp* amp = (Amp*)instance;

	switch (port) {
	case AMP_GAIN:
		amp->gain = data;
		break;
	case AMP_INPUT:
		amp->input = data;
		break;
	case AMP_OUTPUT:
		amp->output = data;
		break;
	}
}

static void
activate(LV2_Handle instance)
{
}

#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

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

static void
deactivate(LV2_Handle instance)
{
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

const void*
extension_data(const char * uri)
{
	return NULL;
}

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
