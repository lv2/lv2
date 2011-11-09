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
   @file atom-helpers.h Helper functions for the LV2 Atom extension.

   These functions are provided for convenience only, use of them is not
   required for supporting atoms.

   Note that these functions are all static inline which basically means:
   do not take the address of these functions.
*/

#ifndef LV2_ATOM_HELPERS_H
#define LV2_ATOM_HELPERS_H

#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

typedef LV2_Atom_Property* LV2_Thing_Iter;

/** Get an iterator pointing to @c prop in some LV2_Thing */
static inline LV2_Thing_Iter
lv2_thing_begin(const LV2_Thing* obj)
{
	return (LV2_Thing_Iter)(obj->properties);
}

/** Return true iff @c iter has reached the end of @c thing */
static inline bool
lv2_thing_iter_is_end(const LV2_Thing* obj, LV2_Thing_Iter iter)
{
	return (uint8_t*)iter >= ((uint8_t*)obj + sizeof(LV2_Atom) + obj->size);
}

/** Return true iff @c l points to the same property as @c r */
static inline bool
lv2_thing_iter_equals(const LV2_Thing_Iter l, const LV2_Thing_Iter r)
{
	return l == r;
}

/** Return an iterator to the property following @c iter */
static inline LV2_Thing_Iter
lv2_thing_iter_next(const LV2_Thing_Iter iter)
{
	return (LV2_Thing_Iter)((uint8_t*)iter
	                        + sizeof(LV2_Atom_Property)
	                        + lv2_atom_pad_size(iter->value.size));
}

/** Return the property pointed to by @c iter */
static inline LV2_Atom_Property*
lv2_thing_iter_get(LV2_Thing_Iter iter)
{
	return (LV2_Atom_Property*)iter;
}

/**
   A macro for iterating over all properties of an Thing.
   @param obj  The thing to iterate over
   @param iter The name of the iterator

   This macro is used similarly to a for loop (which it expands to), e.g.:
   <pre>
   LV2_THING_FOREACH(thing, i) {
   LV2_Atom_Property* prop = lv2_thing_iter_get(i);
   // Do things with prop here...
   }
   </pre>
*/
#define LV2_THING_FOREACH(thing, iter)                                  \
	for (LV2_Thing_Iter (iter) = lv2_thing_begin(thing); \
	     !lv2_thing_iter_is_end(thing, (iter)); \
	     (iter) = lv2_thing_iter_next(iter))

/**
   Append a Property body to an Atom that contains properties (e.g. atom:Thing).
   @param thing Pointer to the atom that contains the property to add.  thing.size
   must be valid, but thing.type is ignored.
   @param key The key of the new property
   @param value_type The type of the new value
   @param value_size The size of the new value
   @param value_body Pointer to the new value's data
   @return a pointer to the new LV2_Atom_Property in @c body.

   This function will write the property body (not including an LV2_Thing
   header) at lv2_atom_pad_size(body + size).  Thus, it can be used with any
   Atom type that contains headerless 32-bit aligned properties.
*/
static inline LV2_Atom_Property*
lv2_thing_append(LV2_Thing* thing,
                 uint32_t    key,
                 uint32_t    value_type,
                 uint32_t    value_size,
                 const void* value_body)
{
	thing->size = lv2_atom_pad_size(thing->size);
	LV2_Atom_Property* prop = (LV2_Atom_Property*)(
		(uint8_t*)thing + sizeof(LV2_Atom) + thing->size);
	prop->key = key;
	prop->value.type = value_type;
	prop->value.size = value_size;
	memcpy(prop->value.body, value_body, value_size);
	thing->size += sizeof(LV2_Atom_Property) + value_size;
	return prop;
}

/** Return true iff @c atom is NULL */
static inline bool
lv2_atom_is_null(LV2_Atom* atom)
{
	return !atom || (atom->type == 0 && atom->size == 0);
}

/** A single entry in an Thing query. */
typedef struct {
	uint32_t         key;    /**< Key to query (input set by user) */
	const LV2_Atom** value;  /**< Found value (output set by query function) */
} LV2_Thing_Query;

static const LV2_Thing_Query LV2_THING_QUERY_END = { 0, NULL };

/**
   "Query" an thing, getting a pointer to the values for various keys.

   The value pointer of each item in @c query will be set to the location of
   the corresponding value in @c thing.  Every value pointer in @c query MUST
   be initialised to NULL.  This function reads @c thing in a single linear
   sweep.  By allocating @c query on the stack, things can be "queried"
   quickly without allocating any memory.  This function is realtime safe.
*/
static inline int
lv2_thing_query(const LV2_Thing* thing, LV2_Thing_Query* query)
{
	int matches   = 0;
	int n_queries = 0;

	/* Count number of query keys so we can short-circuit when done */
	for (LV2_Thing_Query* q = query; q->key; ++q)
		++n_queries;

	LV2_THING_FOREACH(thing, o) {
		const LV2_Atom_Property* prop = lv2_thing_iter_get(o);
		for (LV2_Thing_Query* q = query; q->key; ++q) {
			if (q->key == prop->key && !*q->value) {
				*q->value = &prop->value;
				if (++matches == n_queries)
					return matches;
				break;
			}
		}
	}
	return matches;
}

#endif /* LV2_ATOM_HELPERS_H */
