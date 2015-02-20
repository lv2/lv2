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

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.hpp"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.hpp"
#include "lv2/lv2plug.in/ns/lv2core/Lib.hpp"
#include "lv2/lv2plug.in/ns/lv2core/Plugin.hpp"

/** MIDI-controlled amplifier. */
class MidiAmp : public lv2::Plugin<MidiAmp> {
public:
	MidiAmp(double                    rate,
	        const char*               bundle_path,
	        const LV2_Feature* const* features,
	        bool*                     valid)
		: Plugin(rate, bundle_path, features, valid)
		, m_map(features, valid)
		, m_vol(1.0f)
	{
		if (!*valid) {
			return;
		}

		midi_MidiEvent = m_map(LV2_MIDI__MidiEvent);
	}

	typedef enum {
		AMP_CONTROL = 0,
		AMP_INPUT   = 1,
		AMP_OUTPUT  = 2
	} PortIndex;

	void connect_port(uint32_t port, void* data) {
		switch ((PortIndex)port) {
		case AMP_CONTROL:
			m_ports.control = (const lv2::atom::Sequence*)data;
			break;
		case AMP_INPUT:
			m_ports.input = (const float*)data;
			break;
		case AMP_OUTPUT:
			m_ports.output = (float*)data;
			break;
		}
	}

	void run(const uint32_t n_samples) {
		uint32_t offset = 0;
		for (const lv2::atom::Event& ev : *m_ports.control) {
			// Emit audio up to this event's time
			for (uint32_t o = offset; o < ev.time.frames; ++o) {
				m_ports.output[o] = m_ports.input[0] * m_vol;
			}

			// Process event
			if (ev.type() == midi_MidiEvent &&
			    (ev[0] & 0xF0) == LV2_MIDI_MSG_CONTROLLER &&
			    ev[1] == LV2_MIDI_CTL_MSB_MAIN_VOLUME) {
				m_vol = ev[2] / 127.0f;
			}

			offset = ev.time.frames;
		}

		// Emit remaining audio to the end of the block
		for (uint32_t o = offset; o < n_samples; ++o) {
			m_ports.output[o] = m_ports.input[0] * m_vol;
		}
	}

private:
	typedef struct {
		const lv2::atom::Sequence* control;
		const float*               input;
		float*                     output;
	} Ports;

	lv2::urid::Map<true> m_map;
	lv2::urid::URID      midi_MidiEvent;
	Ports                m_ports;
	float                m_vol;
};

/** Plugin library. */
class MidiAmpLib : public lv2::Lib<MidiAmpLib>
{
public:
	MidiAmpLib(const char*              bundle_path,
	           const LV2_Feature*const* features)
		: lv2::Lib<MidiAmpLib>(bundle_path, features)
		, m_amp(MidiAmp::descriptor("http://lv2plug.in/plugins/eg-midiamp"))
	{}

	const LV2_Descriptor* get_plugin(uint32_t index) {
		return index == 0 ? &m_amp : NULL;
	}

private:
	LV2_Descriptor m_amp;
};

/** Library entry point. */
LV2_SYMBOL_EXPORT const LV2_Lib_Descriptor*
lv2_lib_descriptor(const char*                bundle_path,
                   const LV2_Feature *const * features)

{
	return new MidiAmpLib(bundle_path, features);
}
