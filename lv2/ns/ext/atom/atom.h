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
   @file atom.h C header for the LV2 Atom extension
   <http://lv2plug.in/ns/ext/atom>.

   This header describes the binary layout of various types defined in the
   atom extension.
*/

#ifndef LV2_ATOM_H
#define LV2_ATOM_H

#define LV2_ATOM_URI "http://lv2plug.in/ns/ext/atom"

#define LV2_ATOM_REFERENCE_TYPE 0

#include <stdint.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

/** This expression will fail to compile if double does not fit in 64 bits. */
typedef char lv2_atom_assert_double_fits_in_64_bits[
	((sizeof(double) <= sizeof(uint64_t)) * 2) - 1];

/**
   Return a pointer to the contents of a variable-sized atom.
   @param type The type of the atom, e.g. LV2_Atom_String.
   @param atom A variable-sized atom.
*/
#define LV2_ATOM_CONTENTS(type, atom) \
	((void*)((uint8_t*)(atom) + sizeof(type)))

/** Return a pointer to the body of @p atom (just past the LV2_Atom head). */
#define LV2_ATOM_BODY(atom) LV2_ATOM_CONTENTS(LV2_Atom, atom)

/** The header of an atom:Atom. */
typedef struct {
	uint32_t type;  /**< Type of this atom (mapped URI). */
	uint32_t size;  /**< Size in bytes, not including type and size. */
} LV2_Atom;

/** An atom:Int32 or atom:Bool.  May be cast to LV2_Atom. */
typedef struct {
	LV2_Atom atom;
	int32_t  value;
} LV2_Atom_Int32;

/** An atom:Int64.  May be cast to LV2_Atom. */
typedef struct {
	LV2_Atom atom;
	int64_t  value;
} LV2_Atom_Int64;

/** An atom:Float.  May be cast to LV2_Atom. */
typedef struct {
	LV2_Atom atom;
	float    value;
} LV2_Atom_Float;

/** An atom:Double.  May be cast to LV2_Atom. */
typedef struct {
	LV2_Atom atom;
	double   value;
} LV2_Atom_Double;

/** An atom:Bool.  May be cast to LV2_Atom. */
typedef LV2_Atom_Int32 LV2_Atom_Bool;

/** An atom:URID.  May be cast to LV2_Atom. */
typedef struct {
	LV2_Atom atom;  /**< Atom header. */
	uint32_t id;    /**< URID. */
} LV2_Atom_URID;

/** The complete header of an atom:String. */
typedef struct {
	LV2_Atom atom;  /**< Atom header. */
	/* Contents (a null-terminated UTF-8 string) follow here. */
} LV2_Atom_String;

/** The header of an atom:Literal body. */
typedef struct {
	uint32_t datatype;  /**< The ID of the datatype of this literal. */
	uint32_t lang;      /**< The ID of the language of this literal. */
} LV2_Atom_Literal_Head;

/** The complete header of an atom:Literal. */
typedef struct {
	LV2_Atom              atom;     /**< Atom header. */
	LV2_Atom_Literal_Head literal;  /**< Literal body header. */
	/* Contents (a null-terminated UTF-8 string) follow here. */
} LV2_Atom_Literal;

/** The complete header of an atom:Tuple. */
typedef struct {
	LV2_Atom atom;  /**< Atom header. */
	/* Contents (a series of complete atoms) follow here. */
} LV2_Atom_Tuple;

/** The complete header of an atom:Vector. */
typedef struct {
	LV2_Atom atom;        /**< Atom header. */
	uint32_t elem_count;  /**< The number of elements in the vector */
	uint32_t elem_type;   /**< The type of each element in the vector */
	/* Contents (a series of packed atom bodies) follow here. */
} LV2_Atom_Vector;

/** The header of an atom:Property body (e.g. in an atom:Object). */
typedef struct {
	uint32_t key;      /**< Key (predicate) (mapped URI). */
	uint32_t context;  /**< Context URID (may be, and generally is, 0). */
	LV2_Atom value;    /**< Value atom header. */
} LV2_Atom_Property_Body;

/** The complete header of an atom:Property. */
typedef struct {
	LV2_Atom atom;     /**< Atom header. */
	uint32_t key;      /**< Key (predicate) (mapped URI). */
	uint32_t context;  /**< Context URID (may be, and generally is, 0). */
	LV2_Atom value;    /**< Value atom header. */
	/* Value atom body follows here. */
} LV2_Atom_Property;

/** The complete header of an atom:Object. */
typedef struct {
	LV2_Atom atom;  /**< Atom header. */
	uint32_t id;    /**< URID for atom:Resource, or blank ID for atom:Blank. */
	uint32_t type;  /**< Type URID (same as rdf:type, for fast dispatch). */
	/* Contents (a series of property bodies) follow here. */
} LV2_Atom_Object;

/** The complete header of an atom:Response. */
typedef struct {
	LV2_Atom atom;    /**< Atom header. */
	uint32_t source;  /**< ID of message this is a response to (may be 0). */
	uint32_t type;    /**< Specific response type URID (may be 0). */
	uint32_t seq;     /**< Response sequence number, 0 for end. */
	LV2_Atom body;    /**< Body atom header (may be empty). */
	/* Body optionally follows here. */
} LV2_Atom_Response;

/** A time stamp in frames.  Note this type is NOT an LV2_Atom. */
typedef struct {
	uint32_t frames;     /**< Time in frames relative to this block. */
	uint32_t subframes;  /**< Fractional time in 1/(2^32)ths of a frame. */
} LV2_Atom_Audio_Time;

/** The header of an atom:Event.  Note this type is NOT an LV2_Atom. */
typedef struct {
	/** Time stamp.  Which type is valid is determined by context. */
	union {
		LV2_Atom_Audio_Time audio;  /**< Time in audio frames. */
		double              beats;  /**< Time in beats. */
	} time;
	LV2_Atom body;  /**< Event body atom header. */
	/* Body atom contents follow here. */
} LV2_Atom_Event;

/**
   A sequence of events (time-stamped atoms).

   This is used as the contents of an atom:EventPort, but is a generic Atom
   type which can be used anywhere.

   The contents of a sequence is a series of LV2_Atom_Event, each aligned
   to 64-bits, e.g.:
   <pre>
   | Event 1 (size 6)                              | Event 2
   |       |       |       |       |       |       |       |       |
   | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | | |
   |FRAMES |SUBFRMS|TYPE   |SIZE   |DATADATADATAPAD|FRAMES |SUBFRMS|...
   </pre>
*/
typedef struct {
	LV2_Atom atom;      /**< Atom header. */
	uint32_t capacity;  /**< Maximum size of contents. */
	uint32_t pad;
	/* Contents (a series of events) follow here. */
} LV2_Atom_Sequence;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_ATOM_H */
