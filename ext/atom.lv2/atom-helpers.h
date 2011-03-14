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
#include <stdio.h>
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

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

typedef LV2_Atom_Property* LV2_Object_Iter;

/** Get an iterator pointing to @a prop in some LV2_Object */
static inline LV2_Object_Iter
lv2_object_begin(const LV2_Atom* obj)
{
	return (LV2_Object_Iter)(((const LV2_Object*)obj->body)->properties);
}

/** Return true iff @a iter has reached the end of @a object */
static inline bool
lv2_object_iter_is_end(const LV2_Atom* object, LV2_Object_Iter iter)
{
	return (uint8_t*)iter >= ((uint8_t*)object->body + object->size);
}

/** Return true iff @a l points to the same property as @a r */
static inline bool
lv2_object_iter_equals(const LV2_Object_Iter l, const LV2_Object_Iter r)
{
	return l == r;
}

/** Return an iterator to the property following @a iter */
static inline LV2_Object_Iter
lv2_object_iter_next(const LV2_Object_Iter iter)
{
	return (LV2_Object_Iter)(
		(uint8_t*)iter + sizeof(LV2_Atom_Property) + lv2_atom_pad_size(iter->value.size));
}

/** Return the property pointed to by @a iter */
static inline LV2_Atom_Property*
lv2_object_iter_get(LV2_Object_Iter iter)
{
	return (LV2_Atom_Property*)iter;
}

/** A macro for iterating over all properties of an Object.
 * @param obj  The object to iterate over
 * @param iter The name of the iterator
 *
 * This macro is used similarly to a for loop (which it expands to), e.g.:
 * <pre>
 * LV2_OBJECT_FOREACH(object, i) {
 *    LV2_Atom_Property* prop = lv2_object_iter_get(i);
 *    // Do things with prop here...
 * }
 * </pre>
 */
#define LV2_OBJECT_FOREACH(obj, iter) \
	for (LV2_Object_Iter (iter) = lv2_object_begin(obj); \
	     !lv2_object_iter_is_end(obj, (iter)); \
	     (iter) = lv2_object_iter_next(iter))

/** Append a Property body to an Atom that contains properties (e.g. atom:Object).
 * This function will write the property body (not including an LV2_Object
 * header) at lv2_atom_pad_size(body + size).  Thus, it can be used with any
 * Atom type that contains headerless 32-bit aligned properties.
 * @param object Pointer to the atom that contains the property to add.  object.size
 *        must be valid, but object.type is ignored.
 * @param key The key of the new property
 * @param value_type The type of the new value
 * @param value_size The size of the new value
 * @param value_body Pointer to the new value's data
 * @return a pointer to the new LV2_Atom_Property in @a body.
 */
static inline LV2_Atom_Property*
lv2_atom_append_property(LV2_Atom*      object,
                         uint32_t       key,
                         uint16_t       value_type,
                         uint16_t       value_size,
                         const uint8_t* value_body)
{
	object->size = lv2_atom_pad_size(object->size);
	LV2_Atom_Property* prop = (LV2_Atom_Property*)(object->body + object->size);
	prop->key = key;
	prop->value.type = value_type;
	prop->value.size = value_size;
	memcpy(prop->value.body, value_body, value_size);
	object->size += sizeof(LV2_Atom_Property) + value_size;
	return prop;
}

/** Return true iff @a atom is NULL */
static inline bool
lv2_atom_is_null(LV2_Atom* atom)
{
	return !atom || (atom->type == 0 && atom->size == 0);
}

/** Return true iff @a object has rdf:type @a type */
static inline bool
lv2_atom_is_a(LV2_Atom* object,
              uint32_t  rdf_type,
              uint32_t  atom_URIInt,
              uint32_t  atom_Object,
              uint32_t  type)
{
	if (lv2_atom_is_null(object))
		return false;

	if (object->type == type)
		return true;

	if (object->type == atom_Object) {
		LV2_OBJECT_FOREACH(object, o) {
			LV2_Atom_Property* prop = lv2_object_iter_get(o);
			if (prop->key == rdf_type) {
				if (prop->value.type == atom_URIInt) {
					const uint32_t object_type = *(uint32_t*)prop->value.body;
					if (object_type == type)
						return true;
				} else {
					fprintf(stderr, "error: rdf:type is not a URIInt\n");
				}
			}
		}
	}

	return false;
}

/** A single entry in an Object query. */
typedef struct {
	uint32_t        key;   ///< Set by the user to the desired key to query.
	const LV2_Atom* value; ///< Possibly set by query function to the found value
} LV2_Object_Query;

/** "Query" an object, getting a pointer to the values for various keys.
 * The value pointer of each item in @a q will be set to the location of
 * the corresponding value in @a object.  Every value pointer in @a query
 * MUST be initialised to NULL.  This function reads @a object in a single
 * linear sweep.  By allocating @a q on the stack, objects can be "queried"
 * quickly without allocating any memory.  This function is realtime safe.
 */
static inline int
lv2_object_query(const LV2_Atom* object, LV2_Object_Query* query)
{
	int matches   = 0;
	int n_queries = 0;

	// Count number of query keys so we can short-circuit when done
	for (LV2_Object_Query* q = query; q->key; ++q)
		++n_queries;
	
	LV2_OBJECT_FOREACH(object, o) {
		const LV2_Atom_Property* prop = lv2_object_iter_get(o);
		for (LV2_Object_Query* q = query; q->key; ++q) {
			if (q->key == prop->key && !q->value) {
				q->value = &prop->value;
				if (++matches == n_queries)
					return matches;
				break;
			}
		}
	}
	return matches;
}

#endif /* LV2_ATOM_HELPERS_H */

