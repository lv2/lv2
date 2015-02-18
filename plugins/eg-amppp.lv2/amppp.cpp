/*
  Copyright 2015 David Robillard <d@drobilla.net>

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

#include "lv2/lv2plug.in/ns/lv2core/Plugin.hpp"

class Amppp : public lv2::Plugin {
public:
	Amppp(double                    rate,
	      const char*               bundle_path,
	      const LV2_Feature* const* features)
		: Plugin(rate, bundle_path, features)
	{}

	typedef enum {
		AMP_GAIN   = 0,
		AMP_INPUT  = 1,
		AMP_OUTPUT = 2
	} PortIndex;

	void connect_port(uint32_t port, void* data) {
		switch ((PortIndex)port) {
		case AMP_GAIN:
			m_ports.gain = (const float*)data;
			break;
		case AMP_INPUT:
			m_ports.input = (const float*)data;
			break;
		case AMP_OUTPUT:
			m_ports.output = (float*)data;
			break;
		}
	}

	#define DB_CO(g) ((g) > -90.0f ? powf(10.0f, (g) * 0.05f) : 0.0f)

	void run(uint32_t n_samples) {
		const float coef = DB_CO(*m_ports.gain);
		for (uint32_t pos = 0; pos < n_samples; pos++) {
			m_ports.output[pos] = m_ports.input[pos] * coef;
		}
	}

private:
	typedef struct {
		const float* gain;
		const float* input;
		float*       output;
	} Ports;

	Ports m_ports;
};

static const LV2_Descriptor descriptor = lv2::descriptor<Amppp>("http://lv2plug.in/plugins/eg-amppp");

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	switch (index) {
	case 0:  return &descriptor;
	default: return NULL;
	}
}
