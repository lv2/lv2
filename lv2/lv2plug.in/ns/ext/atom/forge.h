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

typedef void* LV2_Atom_Forge_Sink_Handle;

typedef uint32_t (*LV2_Atom_Forge_Sink)(LV2_Atom_Forge_Sink_Handle handle,
                                        const void*                buf,
                                        uint32_t                   size);

/**
   A "forge" for creating atoms by appending to a buffer.
*/
typedef struct {
	uint8_t* buf;
	uint32_t offset;
	uint32_t size;

	LV2_Atom_Forge_Sink        sink;
	LV2_Atom_Forge_Sink_Handle handle;

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
	LV2_URID URI;
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

static inline void
lv2_atom_forge_set_sink(LV2_Atom_Forge*            forge,
                        LV2_Atom_Forge_Sink        sink,
                        LV2_Atom_Forge_Sink_Handle handle)
{
	forge->buf    = NULL;
	forge->size   = forge->offset = 0;
	forge->sink   = sink;
	forge->handle = handle;
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
	forge->URI      = map->map(map->handle, LV2_ATOM_URI "#URI");
	forge->URID     = map->map(map->handle, LV2_ATOM_URI "#URID");
	forge->Vector   = map->map(map->handle, LV2_ATOM_URI "#Vector");
}

static inline void*
lv2_atom_forge_write_nopad(LV2_Atom_Forge* forge,
                           LV2_Atom*       parent,
                           const void*     data,
                           uint32_t        size)
{
	uint8_t* const out = forge->buf + forge->offset;
	if (forge->offset + size > forge->size) {
		return NULL;
	}
	if (parent) {
		parent->size += size;
	}
	forge->offset += size;
	memcpy(out, data, size);
	return out;
}

static inline void
lv2_atom_forge_pad(LV2_Atom_Forge* forge,
                   LV2_Atom*       parent,
                   uint32_t        written)
{
	const uint64_t pad      = 0;
	const uint32_t pad_size = lv2_atom_pad_size(written) - written;
	lv2_atom_forge_write_nopad(forge, parent, &pad, pad_size);
}

static inline void*
lv2_atom_forge_write(LV2_Atom_Forge* forge,
                     LV2_Atom*       parent,
                     const void*     data,
                     uint32_t        size)
{
	void* out = lv2_atom_forge_write_nopad(forge, parent, data, size);
	if (out) {
		lv2_atom_forge_pad(forge, parent, size);
	}
	return out;
}

/** Write an atom:Int32. */
static inline LV2_Atom_Int32*
lv2_atom_forge_int32(LV2_Atom_Forge* forge, LV2_Atom* parent, int32_t val)
{
	const LV2_Atom_Int32 a = { { forge->Int32, sizeof(val) }, val };
	return (LV2_Atom_Int32*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
}

/** Write an atom:Int64. */
static inline LV2_Atom_Int64*
lv2_atom_forge_int64(LV2_Atom_Forge* forge, LV2_Atom* parent, int64_t val)
{
	const LV2_Atom_Int64 a = { { forge->Int64, sizeof(val) }, val };
	return (LV2_Atom_Int64*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
}

/** Write an atom:Float. */
static inline LV2_Atom_Float*
lv2_atom_forge_float(LV2_Atom_Forge* forge, LV2_Atom* parent, float val)
{
	const LV2_Atom_Float a = { { forge->Float, sizeof(val) }, val };
	return (LV2_Atom_Float*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
}

/** Write an atom:Double. */
static inline LV2_Atom_Double*
lv2_atom_forge_double(LV2_Atom_Forge* forge, LV2_Atom* parent, double val)
{
	const LV2_Atom_Double a = { { forge->Double, sizeof(val) }, val };
	return (LV2_Atom_Double*)lv2_atom_forge_write(
		forge, parent, &a, sizeof(a));
}

/** Write an atom:Bool. */
static inline LV2_Atom_Bool*
lv2_atom_forge_bool(LV2_Atom_Forge* forge, LV2_Atom* parent, bool val)
{
	const LV2_Atom_Bool a = { { forge->Bool, sizeof(val) }, val };
	return (LV2_Atom_Bool*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
}

/** Write an atom:URID. */
static inline LV2_Atom_URID*
lv2_atom_forge_urid(LV2_Atom_Forge* forge, LV2_Atom* parent, LV2_URID id)
{
	const LV2_Atom_URID a = { { forge->Int32, sizeof(id) }, id };
	return (LV2_Atom_URID*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
}

/** Write a string body.  Used internally. */
static inline uint8_t*
lv2_atom_forge_string_body(LV2_Atom_Forge* forge,
                           LV2_Atom*       parent,
                           const uint8_t*  str,
                           size_t          len)
{
	uint8_t* out = NULL;
	if ((out = lv2_atom_forge_write_nopad(forge, parent, str, len))
	    && (out = lv2_atom_forge_write_nopad(forge, parent, "", 1))) {
		lv2_atom_forge_pad(forge, parent, len + 1);
	}
	return out;
}

/** Write an atom:String.  Note that @p str need not be NULL terminated. */
static inline LV2_Atom_String*
lv2_atom_forge_string(LV2_Atom_Forge* forge,
                      LV2_Atom*       parent,
                      const uint8_t*  str,
                      size_t          len)
{
	const LV2_Atom_String a = { { forge->String, len + 1 } };
	LV2_Atom_String* out = (LV2_Atom_String*)
		lv2_atom_forge_write_nopad(forge, parent, &a, sizeof(a));
	if (out) {
		if (!lv2_atom_forge_string_body(forge, parent, str, len)) {
			out->atom.type = 0;
			out->atom.size = 0;
			out = NULL;
		}
	}
	return out;
}

/**
   Write an atom:URI.  Note that @p uri need not be NULL terminated.
   This does not map the URI, but writes the complete URI string.  To write
   a mapped URI, use lv2_atom_forge_urid().
*/
static inline LV2_Atom_String*
lv2_atom_forge_uri(LV2_Atom_Forge* forge,
                   LV2_Atom*       parent,
                   const uint8_t*  uri,
                   size_t          len)
{
	const LV2_Atom_String a = { { forge->URI, len + 1 } };
	LV2_Atom_String* out = (LV2_Atom_String*)
		lv2_atom_forge_write_nopad(forge, parent, &a, sizeof(a));
	if (!out) {
		return NULL;
	}
	if (!lv2_atom_forge_string_body(forge, parent, uri, len)) {
		out->atom.type = 0;
		out->atom.size = 0;
		return NULL;
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
	const LV2_Atom_Literal a = {
		{ forge->Literal,
		  sizeof(LV2_Atom_Literal) - sizeof(LV2_Atom) + len + 1 },
		datatype,
		lang
	};
	LV2_Atom_Literal* out = (LV2_Atom_Literal*)
		lv2_atom_forge_write_nopad(forge, parent, &a, sizeof(a));
	if (out) {
		if (!lv2_atom_forge_string_body(forge, parent, str, len)) {
			out->atom.type = 0;
			out->atom.size = 0;
			out = NULL;
		}
	}
	return out;
}

/** Write an atom:Vector header, but not the vector body. */
static inline LV2_Atom_Vector*
lv2_atom_forge_vector_head(LV2_Atom_Forge* forge,
                           LV2_Atom*       parent,
                           uint32_t        elem_count,
                           uint32_t        elem_type,
                           uint32_t        elem_size)
{
	const size_t size = sizeof(LV2_Atom_Vector) + (elem_size * elem_count);
	const LV2_Atom_Vector a = {
		{ forge->Vector, size - sizeof(LV2_Atom) },
		elem_count,
		elem_type
	};
	return (LV2_Atom_Vector*)lv2_atom_forge_write(
		forge, parent, &a, sizeof(a));
}

/** Write a complete atom:Vector. */
static inline LV2_Atom_Vector*
lv2_atom_forge_vector(LV2_Atom_Forge* forge,
                      LV2_Atom*       parent,
                      uint32_t        elem_count,
                      uint32_t        elem_type,
                      uint32_t        elem_size,
                      void*           elems)
{
	LV2_Atom_Vector* out = lv2_atom_forge_vector_head(
		forge, parent, elem_count, elem_type, elem_size);
	if (out) {
		lv2_atom_forge_write(forge, parent, elems, elem_size * elem_count);
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
	const LV2_Atom_Tuple a = { { forge->Tuple, 0 } };
	return (LV2_Atom_Tuple*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
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
	const LV2_Atom_Object a = {
		{ forge->Resource, sizeof(LV2_Atom_Object) - sizeof(LV2_Atom) },
		id,
		otype
	};
	return (LV2_Atom_Object*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
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
	const LV2_Atom_Object a = {
		{ forge->Blank, sizeof(LV2_Atom_Object) - sizeof(LV2_Atom) },
		id,
		otype
	};
	return (LV2_Atom_Object*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
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
	const LV2_Atom_Property_Body a = { key, context, { 0, 0 } };
	return (LV2_Atom_Property_Body*)lv2_atom_forge_write(
		forge, parent, &a, 2 * sizeof(uint32_t));
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
	const LV2_Atom_Sequence a = {
		{ forge->Sequence, sizeof(LV2_Atom_Sequence) - sizeof(LV2_Atom) },
		unit,
		0
	};
	return (LV2_Atom_Sequence*)lv2_atom_forge_write(
		forge, parent, &a, sizeof(a));
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
	const LV2_Atom_Audio_Time a = { frames, subframes };
	return (LV2_Atom_Event*)lv2_atom_forge_write(forge, parent, &a, sizeof(a));
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
	return (LV2_Atom_Event*)lv2_atom_forge_write(
		forge, parent, &beats, sizeof(beats));
}

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_ATOM_FORGE_H */
