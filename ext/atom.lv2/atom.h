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
   host MAY marshal an LV2_Event to an LV2_Atom by simply pointing to the
   offset of <code>type</code>. The macro LV2_ATOM_FROM_EVENT is provided in
   this header for this purpose.
*/
typedef struct {

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
typedef struct {
	uint32_t datatype;  /**< The ID of the datatype of this literal */
	uint32_t lang;      /**< The ID of the language of this literal */
	uint8_t  str[];     /**< Null-terminated string data in UTF-8 encoding */
} LV2_Atom_Literal;

/**
   The body of an atom:Vector.
*/
typedef struct {
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
typedef struct {
	uint32_t context;      /**< ID of context graph, or 0 for default */
	uint32_t id;           /**< URID (for Resource) or blank ID (for Blank) */
	uint8_t  properties[]; /**< Sequence of LV2_Atom_Property */
} LV2_Object;

/**
   An Event (a timestamped Atom).

   Note this struct is different from the other structs in this header in that
   it does not describe the body of some LV2_Atom, but instead is a "larger"
   type which contains an LV2_Atom as its payload.  This makes it possible for
   an Event to be interpreted as an Atom in-place by simply pointing at
   the @ref body field of the Event.
*/
typedef struct {

	/**
	   The frames portion of timestamp.  The unit of this value may depend on
	   context, but for events processed by LV2_Descriptor::run() the unit is
	   audio frames relative to this block (e.g. frame 0 is the first frame in
	   this call to run())
	*/
	uint32_t frames;

	/**
	   The sub-frames portion of timestamp.  The unit of this value may depend
	   on context, but for events processed by LV2_Descriptor::run() the unit
	   is 1/(2^32) of an audio frame.
	*/
	uint32_t subframes;

	/**
	   The body of this event.
	*/
	LV2_Atom body;

} LV2_Atom_Event;

/**
   A buffer of events (the contents of an atom:EventPort).

   The host MAY elect to allocate buffers as a single chunk of POD by using
   this struct as a header much like LV2_Atom, or it may choose to point to
   a fragment of a buffer elsewhere.  In either case, @ref data points to the
   start of the data contained in this buffer.

   The buffer at @ref data contains a sequence of LV2_Atom_Event padded such
   that the start of each event is aligned to 64 bits, e.g.:
   <pre>
   | Event 1 (size 6)                              | Event 2
   |       |       |       |       |       |       |       |       |
   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
   |FRAMES |SUBFRMS|TYPE   |SIZE   |DATADATADATAPAD|FRAMES |SUBFRMS|...
   </pre>
*/
typedef struct {

	/**
	   The contents of the event buffer. This may or may not reside in the
	   same block of memory as this header, plugins must not assume either.
	   The host guarantees this points to at least capacity bytes of allocated
	   memory (though only size bytes of that are valid events).
	*/
	uint8_t* data;

	/**
	   The number of events in this buffer.

	   INPUTS: The host must set this field to the number of events contained
	   in the data buffer before calling run(). The plugin must not change
	   this field.

	   OUTPUTS: The plugin must set this field to the number of events it has
	   written to the buffer before returning from run(). Any initial value
	   should be ignored by the plugin.
	*/
	uint32_t event_count;

	/**
	   The capacity of the data buffer in bytes.
	   This is set by the host and must not be changed by the plugin.
	   The host is allowed to change this between run() calls.
	*/
	uint32_t capacity;

	/**
	   The size of the initial portion of the data buffer containing data.

	   INPUTS: The host must set this field to the number of bytes used
	   by all events it has written to the buffer (including headers)
	   before calling the plugin's run().
	   The plugin must not change this field.

	   OUTPUTS: The plugin must set this field to the number of bytes
	   used by all events it has written to the buffer (including headers)
	   before returning from run().
	   Any initial value should be ignored by the plugin.
	*/
	uint32_t size;

} LV2_Atom_Buffer;

#endif /* LV2_ATOM_H */
