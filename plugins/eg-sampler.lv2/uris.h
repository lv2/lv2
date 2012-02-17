/*
  LV2 Sampler Example Plugin
  Copyright 2011 David Robillard <d@drobilla.net>

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

#ifndef SAMPLER_URIS_H
#define SAMPLER_URIS_H

#include "lv2/lv2plug.in/ns/ext/state/state.h"

#define NS_ATOM "http://lv2plug.in/ns/ext/atom#"
#define NS_RDF  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#define SAMPLER_URI      "http://lv2plug.in/plugins/eg-sampler"
#define MIDI_EVENT_URI   "http://lv2plug.in/ns/ext/midi#MidiEvent"
#define FILE_URI         SAMPLER_URI "#file"
#define APPLY_SAMPLE_URI SAMPLER_URI "#applySample"
#define FREE_SAMPLE_URI  SAMPLER_URI "#freeSample"

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Resource;
	LV2_URID eg_applySample;
	LV2_URID eg_freeSample;
	LV2_URID eg_file;
	LV2_URID midi_Event;
	LV2_URID msg_Set;
	LV2_URID msg_body;
	LV2_URID state_Path;
} SamplerURIs;

static inline void
map_sampler_uris(LV2_URID_Map* map, SamplerURIs* uris)
{
	uris->atom_Blank     = map->map(map->handle, NS_ATOM "Blank");
	uris->atom_Resource  = map->map(map->handle, NS_ATOM "Resource");
	uris->eg_applySample = map->map(map->handle, APPLY_SAMPLE_URI);
	uris->eg_file        = map->map(map->handle, FILE_URI);
	uris->eg_freeSample  = map->map(map->handle, FREE_SAMPLE_URI);
	uris->midi_Event     = map->map(map->handle, MIDI_EVENT_URI);
	uris->msg_Set        = map->map(map->handle, LV2_MESSAGE_Set);
	uris->msg_body       = map->map(map->handle, LV2_MESSAGE_body);
	uris->state_Path     = map->map(map->handle, LV2_STATE_PATH_URI);
}

#endif  /* SAMPLER_URIS_H */
