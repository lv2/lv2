/* lv2_atom_helpers.h - Helper functions for the LV2 atoms extension.
 * Copyright (C) 2008-2010 David Robillard <http://drobilla.net>
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

#ifndef LV2_ATOM_HELPERS_H
#define LV2_ATOM_HELPERS_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include "lv2/http/lv2plug.in/ns/ext/atom/atom.h"

/** @file
 * Helper functions for the LV2 Atom extension
 * <http://lv2plug.in/ns/ext/atom>.
 *
 * These functions are provided for convenience only, use of them is not
 * required for supporting atoms (i.e. the definition of atoms in memory
 * is described in atom.h and NOT by this API).
 *
 * Note that these functions are all static inline which basically means:
 * do not take the address of these functions.
 */


/** Pad a size to 4 bytes (32 bits) */
static inline uint16_t
lv2_atom_pad_size(uint16_t size)
{
	return (size + 3) & (~3);
}

typedef LV2_Atom_Property* LV2_Atom_Dict_Iter;

static inline LV2_Atom_Dict_Iter
lv2_atom_dict_get_iter(LV2_Atom_Property* prop)
{
	return (LV2_Atom_Dict_Iter)prop;
}

static inline bool
lv2_atom_dict_iter_is_end(const LV2_Atom* dict, LV2_Atom_Dict_Iter iter)
{
	return (uint8_t*)iter >= (dict->body + dict->size);
}

static inline bool
lv2_atom_dict_iter_equals(const LV2_Atom_Dict_Iter l, const LV2_Atom_Dict_Iter r)
{
	return l == r;
}

static inline LV2_Atom_Dict_Iter
lv2_atom_dict_iter_next(const LV2_Atom_Dict_Iter iter)
{
	return (LV2_Atom_Dict_Iter)(
		(uint8_t*)iter + sizeof(LV2_Atom_Property) + lv2_atom_pad_size(iter->object.size));
}

static inline LV2_Atom_Property*
lv2_atom_dict_iter_get(LV2_Atom_Dict_Iter iter)
{
	return (LV2_Atom_Property*)iter;
}

/** Append a Property body to an Atom that contains properties (e.g. atom:Dict).
 * This function will write the property body (not including an LV2_Object
 * header) at lv2_atom_pad_size(body + size).  Thus, it can be used with any
 * Atom type that contains headerless 64-bit aligned properties.
 * @param dict Pointer to the atom that contains the property to add.  dict.size
 *        must be valid, but dict.type is ignored.
 * @param size Must point to the size field of the container atom, and will be
 *        padded up to 64 bits then increased by @a value_size.
 * @param body Must point to the body of the container atom.
 * @return a pointer to the new LV2_Atom_Property in @a body.
 */
static inline LV2_Atom_Property*
lv2_atom_append_property(LV2_Atom*   dict,
                         uint32_t    key,
                         uint16_t    value_type,
                         uint16_t    value_size,
                         const char* value_body)
{
	dict->size = lv2_atom_pad_size(dict->size);
	LV2_Atom_Property* prop = (LV2_Atom_Property*)(dict->body + dict->size);
	prop->predicate = key;
	prop->object.type = value_type;
	prop->object.size = value_size;
	memcpy(prop->object.body, value_body, value_size);
	dict->size += sizeof(uint32_t) + sizeof(LV2_Atom_Property) + value_size;
	return prop;
}

#endif /* LV2_ATOM_HELPERS_H */

