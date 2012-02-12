/*
  Copyright 2008-2012 David Robillard <http://drobilla.net>

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
   @file forge.h An API for constructing LV2 atoms.

   This file provides a simple API which can be used to create complex nested
   atoms by calling the provided functions to append atoms (or atom headers) in
   the correct order.  The size of the parent atom is automatically updated,
   but the caller must handle this situation if atoms are more deeply nested.

   All output is written to a user-provided buffer.  This entire API is
   realtime safe and suitable for writing to output port buffers in the run
   method.

   Note these functions are all static inline, do not take their address.

   This header is non-normative, it is provided for convenience.
*/

#ifndef LV2_ATOM_FORGE_H
#define LV2_ATOM_FORGE_H

#include <assert.h>
#include <string.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

/**
   A "forge" for creating atoms by appending to a buffer.
*/
typedef struct {
	uint8_t* buf;
	size_t   offset;
	size_t   size;

	LV2_URID Blank;
	LV2_URID Bool;
	LV2_URID Double;
	LV2_URID Float;
	LV2_URID Int32;
	LV2_URID Int64;
	LV2_URID Literal;
	LV2_URID Property;
	LV2_URID Resource;
	LV2_URID Sequence;
	LV2_URID String;
	LV2_URID Tuple;
	LV2_URID URID;
	LV2_URID Vector;
} LV2_Atom_Forge;

/** Set the output buffer where @c forge will write atoms. */
static inline void
lv2_atom_forge_set_buffer(LV2_Atom_Forge* forge, uint8_t* buf, size_t size)
{
	forge->buf    = buf;
	forge->size   = size;
	forge->offset = 0;
}

/**
   Initialise @c forge.

   URIs will be mapped using @c map and stored, a reference to @c map itself is
   not held.
*/
static inline void
lv2_atom_forge_init(LV2_Atom_Forge* forge, LV2_URID_Map* map)
{
	lv2_atom_forge_set_buffer(forge, NULL, 0);
	forge->Blank    = map->map(map->handle, LV2_ATOM_URI "#Blank");
	forge->Bool     = map->map(map->handle, LV2_ATOM_URI "#Bool");
	forge->Double   = map->map(map->handle, LV2_ATOM_URI "#Double");
	forge->Float    = map->map(map->handle, LV2_ATOM_URI "#Float");
	forge->Int32    = map->map(map->handle, LV2_ATOM_URI "#Int32");
	forge->Int64    = map->map(map->handle, LV2_ATOM_URI "#Int64");
	forge->Literal  = map->map(map->handle, LV2_ATOM_URI "#Literal");
	forge->Property = map->map(map->handle, LV2_ATOM_URI "#Property");
	forge->Resource = map->map(map->handle, LV2_ATOM_URI "#Resource");
	forge->Sequence = map->map(map->handle, LV2_ATOM_URI "#Sequence");
	forge->String   = map->map(map->handle, LV2_ATOM_URI "#String");
	forge->Tuple    = map->map(map->handle, LV2_ATOM_URI "#Tuple");
	forge->URID     = map->map(map->handle, LV2_ATOM_URI "#URID");
	forge->Vector   = map->map(map->handle, LV2_ATOM_URI "#Vector");
}

/**
   Reserve @c size bytes in the output buffer (used internally).
   @return The start of the reserved memory, or NULL if out of space.
*/
static inline uint8_t*
lv2_atom_forge_reserve(LV2_Atom_Forge* forge,
                       LV2_Atom*       parent,
                       uint32_t        size)
{
	uint8_t* const out         = forge->buf + forge->offset;
	const uint32_t padded_size = lv2_atom_pad_size(size);
	if (forge->offset + padded_size > forge->size) {
		return NULL;
	}
	if (parent) {
		parent->size += padded_size;
	}
	forge->offset += padded_size;
	return out;
}

/**
   Write the header of an atom:Atom.

   Space for the complete atom will be reserved, but uninitialised.
*/
static inline LV2_Atom*
lv2_atom_forge_atom_head(LV2_Atom_Forge* forge,
                         LV2_Atom*       parent,
                         uint32_t        type,
                         uint32_t        size)
{
	LV2_Atom* const out = (LV2_Atom*)lv2_atom_forge_reserve(
		forge, parent, size);
	if (out) {
		out->type = type;
		out->size = size - sizeof(LV2_Atom);
	}
	return out;
}

/** Write an atom:Int32. */
static inline LV2_Atom_Int32*
lv2_atom_forge_int32(LV2_Atom_Forge* forge, LV2_Atom* parent, int32_t val)
{
	LV2_Atom_Int32* out = (LV2_Atom_Int32*)lv2_atom_forge_atom_head(
		forge, parent, forge->Int32, sizeof(LV2_Atom_Int32));
	if (out) {
		out->value = val;
	}
	return out;
}

/** Write an atom:Int64. */
static inline LV2_Atom_Int64*
lv2_atom_forge_int64(LV2_Atom_Forge* forge, LV2_Atom* parent, int64_t val)
{
	LV2_Atom_Int64* out = (LV2_Atom_Int64*)lv2_atom_forge_atom_head(
		forge, parent, forge->Int64, sizeof(LV2_Atom_Int64));
	if (out) {
		out->value = val;
	}
	return out;
}

/** Write an atom:Float. */
static inline LV2_Atom_Float*
lv2_atom_forge_float(LV2_Atom_Forge* forge, LV2_Atom* parent, float val)
{
	LV2_Atom_Float* out = (LV2_Atom_Float*)lv2_atom_forge_atom_head(
		forge, parent, forge->Float, sizeof(LV2_Atom_Float));
	if (out) {
		out->value = val;
	}
	return out;
}

/** Write an atom:Double. */
static inline LV2_Atom_Double*
lv2_atom_forge_double(LV2_Atom_Forge* forge, LV2_Atom* parent, double val)
{
	LV2_Atom_Double* out = (LV2_Atom_Double*)lv2_atom_forge_atom_head(
		forge, parent, forge->Double, sizeof(LV2_Atom_Double));
	if (out) {
		out->value = val;
	}
	return out;
}

/** Write an atom:Bool. */
static inline LV2_Atom_Bool*
lv2_atom_forge_bool(LV2_Atom_Forge* forge, LV2_Atom* parent, bool val)
{
	LV2_Atom_Bool* out = (LV2_Atom_Bool*)lv2_atom_forge_atom_head(
		forge, parent, forge->Bool, sizeof(LV2_Atom_Bool));
	if (out) {
		out->value = val ? 1 : 0;
	}
	return out;
}

/** Write an atom:URID. */
static inline LV2_Atom_URID*
lv2_atom_forge_urid(LV2_Atom_Forge* forge, LV2_Atom* parent, LV2_URID id)
{
	LV2_Atom_URID* out = (LV2_Atom_URID*)lv2_atom_forge_reserve(
		forge, parent, sizeof(LV2_Atom_URID));
	if (out) {
		out->atom.type = forge->URID;
		out->atom.size = sizeof(uint32_t);
		out->id        = id;
	}
	return out;
}

/** Write an atom:String. */
static inline LV2_Atom_String*
lv2_atom_forge_string(LV2_Atom_Forge* forge,
                      LV2_Atom*       parent,
                      const uint8_t*  str,
                      size_t          len)
{
	LV2_Atom_String* out = (LV2_Atom_String*)lv2_atom_forge_reserve(
		forge, parent, sizeof(LV2_Atom_String) + len + 1);
	if (out) {
		out->atom.type = forge->String;
		out->atom.size = len + 1;
		assert(LV2_ATOM_CONTENTS(LV2_Atom_String, out) == LV2_ATOM_BODY(out));
		uint8_t* buf = LV2_ATOM_CONTENTS(LV2_Atom_String, out);
		memcpy(buf, str, len);
		buf[len] = '\0';
	}
	return out;
}

/** Write an atom:Literal. */
static inline LV2_Atom_Literal*
lv2_atom_forge_literal(LV2_Atom_Forge* forge,
                       LV2_Atom*       parent,
                       const uint8_t*  str,
                       size_t          len,
                       uint32_t        datatype,
                       uint32_t        lang)
{
	LV2_Atom_Literal* out = (LV2_Atom_Literal*)lv2_atom_forge_reserve(
		forge, parent, sizeof(LV2_Atom_Literal) + len + 1);
	if (out) {
		out->atom.type        = forge->Literal;
		out->atom.size        = sizeof(LV2_Atom_Literal_Head) + len + 1;
		out->literal.datatype = datatype;
		out->literal.lang     = lang;
		uint8_t* buf = LV2_ATOM_CONTENTS(LV2_Atom_Literal, out);
		memcpy(buf, str, len);
		buf[len] = '\0';
	}
	return out;
}

/** Write an atom:Vector header and reserve space for the body. */
static inline LV2_Atom_Vector*
lv2_atom_forge_reserve_vector(LV2_Atom_Forge* forge,
                              LV2_Atom*       parent,
                              uint32_t        elem_count,
                              uint32_t        elem_type,
                              uint32_t        elem_size)
{
	const size_t     size = sizeof(LV2_Atom_Vector) + (elem_size * elem_count);
	LV2_Atom_Vector* out  = (LV2_Atom_Vector*)lv2_atom_forge_reserve(
		forge, parent, size);
	if (out) {
		out->atom.type  = forge->Vector;
		out->atom.size  = size - sizeof(LV2_Atom);
		out->elem_count = elem_count;
		out->elem_type  = elem_type;
	}
	return out;
}

/** Write an atom:Vector. */
static inline LV2_Atom_Vector*
lv2_atom_forge_vector(LV2_Atom_Forge* forge,
                      LV2_Atom*       parent,
                      uint32_t        elem_count,
                      uint32_t        elem_type,
                      uint32_t        elem_size,
                      void*           elems)
{
	LV2_Atom_Vector* out = lv2_atom_forge_reserve_vector(
		forge, parent, elem_count, elem_type, elem_size);
	if (out) {
		uint8_t* buf = LV2_ATOM_CONTENTS(LV2_Atom_Vector, out);
		memcpy(buf, elems, elem_size * elem_count);
	}
	return out;
}

/**
   Write the header of an atom:Tuple.

   To complete the tuple, write a sequence of atoms, always passing the
   returned tuple as the @c parent parameter (or otherwise ensuring the size is
   updated correctly).

   For example:
   @code
   // Write tuple (1, 2.0)
   LV2_Atom* tup = (LV2_Atom*)lv2_atom_forge_tuple(forge, NULL);
   lv2_atom_forge_int32(forge, tup, 1);
   lv2_atom_forge_float(forge, tup, 2.0);
   @endcode
*/
static inline LV2_Atom_Tuple*
lv2_atom_forge_tuple(LV2_Atom_Forge* forge,
                     LV2_Atom*       parent)
{
	return (LV2_Atom_Tuple*)lv2_atom_forge_atom_head(
		forge, parent, forge->Tuple, sizeof(LV2_Atom));
}

/**
   Write the header of an atom:Resource.

   To complete the object, write a sequence of properties, always passing the
   object as the @c parent parameter (or otherwise ensuring the size is updated
   correctly).

   For example:
   @code
   LV2_URID eg_Cat  = map("http://example.org/Cat");
   LV2_URID eg_name = map("http://example.org/name");

   // Write object header
   LV2_Atom* obj = (LV2_Atom*)lv2_atom_forge_resource(forge, NULL, 0, eg_Cat);

   // Write property: eg:name = "Hobbes"
   lv2_atom_forge_property_head(forge, obj, eg_name, 0);
   lv2_atom_forge_string(forge, obj, "Hobbes", strlen("Hobbes"));
   @endcode
*/
static inline LV2_Atom_Object*
lv2_atom_forge_resource(LV2_Atom_Forge* forge,
                        LV2_Atom*       parent,
                        LV2_URID        id,
                        LV2_URID        otype)
{
	LV2_Atom_Object* out = (LV2_Atom_Object*)lv2_atom_forge_reserve(
		forge, parent, sizeof(LV2_Atom_Object));
	if (out) {
		out->atom.type = forge->Resource;
		out->atom.size = sizeof(LV2_Atom_Object) - sizeof(LV2_Atom);
		out->id        = id;
		out->type      = otype;
	}
	return out;
}

/**
   The same as lv2_atom_forge_resource(), but for object:Blank.
*/
static inline LV2_Atom_Object*
lv2_atom_forge_blank(LV2_Atom_Forge* forge,
                     LV2_Atom*       parent,
                     uint32_t        id,
                     LV2_URID        otype)
{
	LV2_Atom_Object* out = (LV2_Atom_Object*)lv2_atom_forge_reserve(
		forge, parent, sizeof(LV2_Atom_Object));
	if (out) {
		out->atom.type = forge->Blank;
		out->atom.size = sizeof(LV2_Atom_Object) - sizeof(LV2_Atom);
		out->id        = id;
		out->type      = otype;
	}
	return out;
}

/**
   Write the header for a property body (likely in an Object).
   See lv2_atom_forge_object() documentation for an example.
*/
static inline LV2_Atom_Property_Body*
lv2_atom_forge_property_head(LV2_Atom_Forge* forge,
                             LV2_Atom*       parent,
                             LV2_URID        key,
                             LV2_URID        context)
{
	LV2_Atom_Property_Body* out = (LV2_Atom_Property_Body*)
		lv2_atom_forge_reserve(forge, parent, 2 * sizeof(uint32_t));
	if (out) {
		out->key     = key;
		out->context = context;
	}
	return out;
}

/**
   Write the header for a Sequence.
   The size of the returned sequence will be 0, so passing it as the parent
   parameter to other forge methods will do the right thing.
*/
static inline LV2_Atom_Sequence*
lv2_atom_forge_sequence_head(LV2_Atom_Forge* forge,
                             LV2_Atom*       parent,
                             uint32_t        capacity,
                             uint32_t        unit)
{
	LV2_Atom_Sequence* out = (LV2_Atom_Sequence*)
		lv2_atom_forge_reserve(forge, parent, sizeof(LV2_Atom_Sequence));
	if (out) {
		out->atom.type = forge->Sequence;
		out->atom.size = sizeof(LV2_Atom_Sequence) - sizeof(LV2_Atom);
		out->unit      = unit;
		out->pad       = 0;
	}
	return out;
}

/**
   Write the time stamp header of an Event (in a Sequence) in audio frames.
   After this, call the appropriate forge method(s) to write the body, passing
   the same @c parent parameter.  Note the returned LV2_Event is NOT an Atom.
*/
static inline LV2_Atom_Event*
lv2_atom_forge_audio_time(LV2_Atom_Forge* forge,
                          LV2_Atom*       parent,
                          uint32_t        frames,
                          uint32_t        subframes)
{
	LV2_Atom_Event* out = (LV2_Atom_Event*)
		lv2_atom_forge_reserve(forge, parent, 2 * sizeof(uint32_t));
	if (out) {
		out->time.audio.frames    = frames;
		out->time.audio.subframes = subframes;
	}
	return out;
}

/**
   Write the time stamp header of an Event (in a Sequence) in beats.
   After this, call the appropriate forge method(s) to write the body, passing
   the same @c parent parameter.  Note the returned LV2_Event is NOT an Atom.
*/
static inline LV2_Atom_Event*
lv2_atom_forge_beat_time(LV2_Atom_Forge* forge,
                         LV2_Atom*       parent,
                         double          beats)
{
	LV2_Atom_Event* out = (LV2_Atom_Event*)
		lv2_atom_forge_reserve(forge, parent, sizeof(double));
	if (out) {
		out->time.beats = beats;
	}
	return out;
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_ATOM_FORGE_H */
