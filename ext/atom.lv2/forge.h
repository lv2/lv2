/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>

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
   @file forge.h Helper constructor functions for LV2 atoms.
*/

#ifndef LV2_ATOM_FORGE_H
#define LV2_ATOM_FORGE_H

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef struct {
	uint32_t ID;
	uint32_t Message;
	uint32_t Property;
} LV2_Atom_Forge;

static inline LV2_Atom_Forge*
lv2_atom_forge_new(LV2_URID_Mapper* mapper)
{
	LV2_Atom_Forge* forge = (LV2_Atom_Forge*)malloc(sizeof(LV2_Atom_Forge));
	forge->ID       = mapper->map_uri(mapper->handle, LV2_ATOM_URI "#ID");
	forge->Message  = mapper->map_uri(mapper->handle, LV2_ATOM_URI "#Message");
	forge->Property = mapper->map_uri(mapper->handle, LV2_ATOM_URI "#Property");
	return forge;
}

static inline void
lv2_atom_forge_free(LV2_Atom_Forge* forge)
{
	free(forge);
}

static inline LV2_Atom_ID
lv2_atom_forge_make_id(LV2_Atom_Forge* forge, uint32_t id)
{
	const LV2_Atom_ID atom = { forge->ID, sizeof(uint32_t), id };
	return atom;
}

static inline void
lv2_atom_forge_set_message(LV2_Atom_Forge* forge,
                           LV2_Object*     msg,
                           uint32_t        id)
{
	msg->type    = forge->Message;
	msg->size    = sizeof(LV2_Object) - sizeof(LV2_Atom);
	msg->context = 0;
	msg->id      = id;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_ATOM_FORGE_H */
