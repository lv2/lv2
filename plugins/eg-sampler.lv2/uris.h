/*
  LV2 Sampler Example Plugin
  Copyright 2011-2012 David Robillard <d@drobilla.net>

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

#define LV2_MIDI__MidiEvent "http://lv2plug.in/ns/ext/midi#MidiEvent"

#define EG_SAMPLER_URI          "http://lv2plug.in/plugins/eg-sampler"
#define EG_SAMPLER__file        EG_SAMPLER_URI "#file"
#define EG_SAMPLER__applySample EG_SAMPLER_URI "#applySample"
#define EG_SAMPLER__freeSample  EG_SAMPLER_URI "#freeSample"

typedef struct {
	LV2_URID atom_Blank;
	LV2_URID atom_Path;
	LV2_URID atom_Resource;
	LV2_URID atom_Sequence;
	LV2_URID atom_eventTransfer;
	LV2_URID eg_applySample;
	LV2_URID eg_file;
	LV2_URID eg_freeSample;
	LV2_URID midi_Event;
	LV2_URID msg_Set;
	LV2_URID msg_body;
} SamplerURIs;

static inline void
map_sampler_uris(LV2_URID_Map* map, SamplerURIs* uris)
{
	uris->atom_Blank         = map->map(map->handle, LV2_ATOM__Blank);
	uris->atom_Path          = map->map(map->handle, LV2_ATOM__Path);
	uris->atom_Resource      = map->map(map->handle, LV2_ATOM__Resource);
	uris->atom_Sequence      = map->map(map->handle, LV2_ATOM__Sequence);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->eg_applySample     = map->map(map->handle, EG_SAMPLER__applySample);
	uris->eg_file            = map->map(map->handle, EG_SAMPLER__file);
	uris->eg_freeSample      = map->map(map->handle, EG_SAMPLER__freeSample);
	uris->midi_Event         = map->map(map->handle, LV2_MIDI__MidiEvent);
	uris->msg_Set            = map->map(map->handle, LV2_MESSAGE__Set);
	uris->msg_body           = map->map(map->handle, LV2_MESSAGE__body);
}

static inline bool
is_object_type(const SamplerURIs* uris, LV2_URID type)
{
	return type == uris->atom_Resource
		|| type == uris->atom_Blank;
}

/**
 * Write a message like the following to @p forge:
 * [
 *     a msg:Set ;
 *     msg:body [
 *         eg-sampler:file </home/me/foo.wav> ;
 *     ] ;
 * ]
 */
static inline LV2_Atom*
write_set_file(LV2_Atom_Forge*    forge,
               const SamplerURIs* uris,
               const char*        filename,
               const size_t       filename_len)
{
	LV2_Atom_Forge_Frame set_frame;
	LV2_Atom* set = (LV2_Atom*)lv2_atom_forge_blank(
		forge, &set_frame, 1, uris->msg_Set);

	lv2_atom_forge_property_head(forge, uris->msg_body, 0);
	LV2_Atom_Forge_Frame body_frame;
	lv2_atom_forge_blank(forge, &body_frame, 2, 0);

	lv2_atom_forge_property_head(forge, uris->eg_file, 0);
	lv2_atom_forge_path(forge, (const uint8_t*)filename, filename_len);

	lv2_atom_forge_pop(forge, &body_frame);
	lv2_atom_forge_pop(forge, &set_frame);

	return set;
}

/**
 * Get the file path from a message like:
 * [
 *     a msg:Set ;
 *     msg:body [
 *         eg-sampler:file </home/me/foo.wav> ;
 *     ] ;
 * ]
 */
static inline const LV2_Atom*
read_set_file(const SamplerURIs*     uris,
              const LV2_Atom_Object* obj)
{
	if (obj->body.otype != uris->msg_Set) {
		fprintf(stderr, "Ignoring unknown message type %d\n", obj->body.otype);
		return NULL;
	}

	/* Get body of message. */
	const LV2_Atom_Object* body = NULL;
	lv2_object_get(obj, uris->msg_body, &body, 0);
	if (!body) {
		fprintf(stderr, "Malformed set message has no body.\n");
		return NULL;
	}
	if (!is_object_type(uris, body->atom.type)) {
		fprintf(stderr, "Malformed set message has non-object body.\n");
		return NULL;
	}

	/* Get file path from body. */
	const LV2_Atom* file_path = NULL;
	lv2_object_get(body, uris->eg_file, &file_path, 0);
	if (!file_path) {
		fprintf(stderr, "Ignored set message with no file PATH.\n");
		return NULL;
	}

	return file_path;
}

#endif  /* SAMPLER_URIS_H */
