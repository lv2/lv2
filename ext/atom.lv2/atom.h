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
   @file atom.h C header for the LV2 Atom extension
   <http://lv2plug.in/ns/ext/atom>.
   
   This extension defines convenience structs that should match the definition
   of the built-in types of the atom extension. The layout of atoms in this
   header must match the description in RDF. The RDF description of an atom
   type should be considered normative. This header is a non-normative (but
   hopefully accurate) implementation of that specification.
*/

#ifndef LV2_ATOM_H
#define LV2_ATOM_H

#define LV2_ATOM_URI "http://lv2plug.in/ns/ext/atom"

#define LV2_ATOM_REFERENCE_TYPE 0

#include <stdint.h>
#include <stddef.h>

#define LV2_ATOM_FROM_EVENT(ev) ((LV2_Atom*)&((LV2_Event*)ev)->type)

/**
   An LV2 Atom.
 
   An "Atom" is a generic chunk of memory with a given type and size.
   The type field defines how to interpret an atom.
 
   All atoms are by definition Plain Old Data (POD) and may be safely copied
   (e.g. with memcpy) using the size field, except atoms with type 0.  An atom
   with type 0 is a reference, and may only be used via the functions provided
   in LV2_Blob_Support (e.g. it MUST NOT be manually copied).
 
   Note that an LV2_Atom is the latter two fields of an LV2_Event as defined by
   the <a href="http://lv2plug.in/ns/ext/event">LV2 events extension</a>. The
   host MAY marshal an <a href="urn:struct:LV2_Event">LV2_Event</a> to an <a
   href="urn:struct:LV2_Atom">LV2_Atom</a> by simply pointing to the offset of
   <code>type</code>. The macro LV2_ATOM_FROM_EVENT is provided in this header
   for this purpose.
*/
typedef struct _LV2_Atom {

	/**
	   The type of this atom.

	   This number is mapped from a URI using the extension
	    <http://lv2plug.in/ns/ext/uri-map> with 'map' =
	    "http://lv2plug.in/ns/ext/atom". Type 0 is a special case which
	    indicates this atom is a reference and MUST NOT be copied manually.
	*/
	uint16_t type;

	/**
	   The size of this atom, not including this header, in bytes.
	*/
	uint16_t size;

	/**
	   Size bytes of data follow here.
	*/
	uint8_t body[];

} LV2_Atom;

/**
   The body of an atom:Literal.
*/
typedef struct _LV2_Atom_Literal {
	uint32_t datatype;  /**< The ID of the datatype of this literal */
	uint32_t lang;      /**< The ID of the language of this literal */
	uint8_t  str[];     /**< Null-terminated string data in UTF-8 encoding */
} LV2_Atom_Literal;

/**
   The body of an atom:Vector.
*/
typedef struct _LV2_Atom_Vector {
	uint16_t elem_count; /**< The number of elements in the vector */
	uint16_t elem_type;  /**< The type of each element in the vector */
	uint8_t  elems[];    /**< Sequence of element bodies */
} LV2_Atom_Vector;

/**
   The body of an atom:Property.
*/
typedef struct _LV2_Atom_Property {
	uint32_t key;   /**< ID of key (predicate) */
	LV2_Atom value; /**< Value (object) */
} LV2_Atom_Property;

/**
   The body of an atom:Resource or atom:Blank.
*/
typedef struct _LV2_Object {
	uint32_t context;      /**< ID of context graph, or 0 for the default context */
	uint32_t id;           /**< ID for atom:Resource or blank ID for atom:Blank */
	uint8_t  properties[]; /**< Sequence of LV2_Atom_Property */
} LV2_Object;

#endif /* LV2_ATOM_H */
