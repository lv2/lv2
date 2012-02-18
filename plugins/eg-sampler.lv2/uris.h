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
	LV2_URID atom_Path;
	LV2_URID atom_Resource;
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
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->eg_applySample     = map->map(map->handle, APPLY_SAMPLE_URI);
	uris->eg_file            = map->map(map->handle, FILE_URI);
	uris->eg_freeSample      = map->map(map->handle, FREE_SAMPLE_URI);
	uris->midi_Event         = map->map(map->handle, MIDI_EVENT_URI);
	uris->msg_Set            = map->map(map->handle, LV2_MESSAGE__Set);
	uris->msg_body           = map->map(map->handle, LV2_MESSAGE__body);
}

static inline bool
is_object_type(const SamplerURIs* uris, LV2_URID type)
{
	return type == uris->atom_Resource
		|| type == uris->atom_Blank;
}

static inline LV2_Atom*
write_set_filename_msg(LV2_Atom_Forge*    forge,
                       const SamplerURIs* uris,
                       const char*        filename,
                       const size_t       filename_len)
{
	/* Send [
	 *     a msg:Set ;
	 *     msg:body [
	 *         eg-sampler:filename </home/me/foo.wav> ;
	 *     ] ;
	 * ]
	 */
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

static inline const LV2_Atom*
get_msg_file_path(const SamplerURIs*     uris,
                  const LV2_Atom_Object* obj)
{
	/* Message should look like this:
	 * [
	 *     a msg:Set ;
	 *     msg:body [
	 *         eg-sampler:file </home/me/foo.wav> ;
	 *     ] ;
	 * ]
	 */

	if (obj->type != uris->msg_Set) {
		fprintf(stderr, "Ignoring unknown message type %d\n", obj->type);
		return NULL;
	}

	/* Get body of message. */
	const LV2_Atom_Object* body = NULL;
	lv2_object_getv(obj, uris->msg_body, &body, 0);
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
	lv2_object_getv(body, uris->eg_file, &file_path, 0);
	if (!file_path) {
		fprintf(stderr, "Ignored set message with no file PATH.\n");
		return NULL;
	}

	return file_path;
}

#endif  /* SAMPLER_URIS_H */
