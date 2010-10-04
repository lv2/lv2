/* lv2_atom.h - C header file for the LV2 Atom extension.
 * Copyright (C) 2008-2009 David Robillard <http://drobilla.net>
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

/** @file
 * C header for the LV2 Atom extension <http://lv2plug.in/ns/ext/atom>.
 * This extension defines convenience structs that
 * should match the definition of the built-in types of the atom extension.
 * The layout of atoms in this header must match the description in RDF.
 * The RDF description of an atom type should be considered normative.
 * This header is a non-normative (but hopefully accurate) implementation
 * of that specification.
 */

#ifndef LV2_ATOM_H
#define LV2_ATOM_H

#define LV2_ATOM_URI       "http://lv2plug.in/ns/ext/atom"
#define LV2_BLOB_SUPPORT_URI "http://lv2plug.in/ns/ext/atom#blobSupport"

#define LV2_ATOM_REFERENCE_TYPE 0

#include <stdint.h>
#include <stddef.h>

#define LV2_ATOM_FROM_EVENT(ev) ((LV2_Atom*)&((LV2_Event*)ev)->type)

/** An LV2 Atom.
 *
 * An "Atom" is a generic chunk of memory with a given type and size.
 * The type field defines how to interpret an atom.
 *
 * All atoms are by definition Plain Old Data (POD) and may be safely
 * copied (e.g. with memcpy) using the size field, except atoms with type 0.
 * An atom with type 0 is a reference, and may only be used via the functions
 * provided in LV2_Blob_Support (e.g. it MUST NOT be manually copied).
 *
 * Note that an LV2_Atom is the latter two fields of an LV2_Event as defined
 * by the <a href="http://lv2plug.in/ns/ext/event">LV2 events extension</a>.
 * The host MAY marshal an Event to an Atom simply by pointing to the offset
 * of the 'type' field of the LV2_Event, which is also the type field (i.e. start)
 * of a valid LV2_Atom.  The macro LV2_ATOM_FROM_EVENT is provided in this
 * header for this purpose.
 */
typedef struct _LV2_Atom {

	/** The type of this atom.  This number represents a URI, mapped to an
	 * integer using the extension <http://lv2plug.in/ns/ext/uri-map>
	 * with "http://lv2plug.in/ns/ext/atom" as the 'map' argument.
	 * Type 0 is a special case which indicates this atom
	 * is a reference and MUST NOT be copied manually.
	 */
	uint16_t type;

	/** The size of this atom, not including this header, in bytes. */
	uint16_t size;

	/** Size bytes of data follow here */
	uint8_t body[];

} LV2_Atom;

/** Reference, an LV2_Atom with type 0 */
typedef LV2_Atom LV2_Reference;

/** The body of an LV2_Atom with type atom:Vector
 */
typedef struct _LV2_Vector_Body {

	/** The size of each element in the vector */
	uint16_t elem_count;

	/** The type of each element in the vector */
	uint16_t elem_type;

	/** Elements follow here */
	uint8_t elems[];

} LV2_Vector_Body;


/** The body of an LV2_Atom with type atom:Triple
 */
typedef struct _LV2_Triple_Body {
	uint32_t subject;
	uint32_t predicate;
	LV2_Atom object;
} LV2_Triple_Body;


/** The body of an LV2_Atom with type atom:Message
 */
typedef struct _LV2_Message_Body {
	uint32_t selector; /***< Selector URI mapped to integer */
	LV2_Atom triples; /***< Always an atom:Triples */
} LV2_Message_Body;


/* Everything below here is related to blobs, which are dynamically allocated
 * atoms that are not necessarily POD.  This functionality is optional,
 * hosts may support atoms without implementing blob support.
 * Blob support is an LV2 Feature.
 */


typedef void* LV2_Blob_Data;

/** Dynamically Allocated LV2 Blob.
 *
 * This is a blob of data of any type, dynamically allocated in memory.
 * Unlike an LV2_Atom, a blob is not necessarily POD.  Plugins may only
 * refer to blobs via a Reference (an LV2_Atom with type 0), there is no
 * way for a plugin to directly create, copy, or destroy a Blob.
 */
typedef struct _LV2_Blob {

	/** Pointer to opaque data.
	 *
	 * Plugins MUST NOT interpret this data in any way.  Hosts may store
	 * whatever information they need to associate with references here.
	 */
	LV2_Blob_Data data;

	/** Get blob's type as a URI mapped to an integer.
	 *
	 * The return value may be any type URI, mapped to an integer with the
	 * URI Map extension.  If this type is an LV2_Atom type, get returns
	 * a pointer to the LV2_Atom header (e.g. a blob with type atom:Int32
	 * does NOT return a pointer to a int32_t).
	 */
	uint32_t (*type)(struct _LV2_Blob* blob);

	/** Get blob's body.
	 *
	 * Returns a pointer to the start of the blob data.  The format of this
	 * data is defined by the return value of the type method.  It MUST NOT
	 * be used in any way by code which does not understand that type.
	 */
	void* (*get)(struct _LV2_Blob* blob);

} LV2_Blob;


typedef void* LV2_Blob_Support_Data;

typedef void (*LV2_Blob_Destroy)(LV2_Blob* blob);

/** The data field of the LV2_Feature for the LV2 Atom extension.
 *
 * A host which supports this extension must pass an LV2_Feature struct to the
 * plugin's instantiate method with 'URI' "http://lv2plug.in/ns/ext/atom" and
 * 'data' pointing to an instance of this struct.  All fields of this struct,
 * MUST be set to non-NULL values by the host (except possibly data).
 */
typedef struct {

	/** Pointer to opaque data.
	 *
	 * The plugin MUST pass this to any call to functions in this struct.
	 * Otherwise, it must not be interpreted in any way.
	 */
	LV2_Blob_Support_Data data;

	/** The size of a reference, in bytes.
	 *
	 * This value is provided by the host so plugins can allocate large
	 * enough chunks of memory to store references.
	 */
	size_t reference_size;

	/** Initialize a reference to point to a newly allocated Blob.
	 *
	 * @param data Must be the data member of this struct.
	 * @param reference Pointer to an area of memory at least as large as
	 *     the reference_size field of this struct.  On return, this will
	 *     be the unique reference to the new blob which is owned by the
	 *     caller.  Caller MUST NOT pass a valid reference.
	 * @param destroy Function to destroy a blob of this type.  This function
	 *     MUST clean up any resources contained in the blob, but MUST NOT
	 *     attempt to free the memory pointed to by its LV2_Blob* parameter.
	 * @param type Type of blob to allocate (URI mapped integer).
	 * @param size Size of blob to allocate in bytes.
	 */
	void (*lv2_blob_new)(LV2_Blob_Support_Data data,
	                     LV2_Reference*        reference,
	                     LV2_Blob_Destroy      destroy_func,
	                     uint32_t              type,
	                     size_t                size);

	/** Return a pointer to the Blob referred to by @a ref.
	 *
	 * The returned value MUST NOT be used in any way other than by calling
	 * methods defined in LV2_Blob (e.g. it MUST NOT be copied or destroyed).
	 */
	LV2_Blob* (*lv2_reference_get)(LV2_Blob_Support_Data data,
	                               LV2_Reference*        ref);

	/** Copy a reference.
	 * This copies a reference but not the blob it refers to,
	 * i.e. after this call @a dst and @a src refer to the same LV2_Blob.
	 */
	void (*lv2_reference_copy)(LV2_Blob_Support_Data data,
	                           LV2_Reference*        dst,
	                           LV2_Reference*        src);

	/** Reset (release) a reference.
	 * After this call, @a ref is invalid.  Use of this function is only
	 * necessary if a plugin makes a copy of a reference it does not later
	 * send to an output (which transfers ownership to the host).
	 */
	void (*lv2_reference_reset)(LV2_Blob_Support_Data data,
	                            LV2_Reference*        ref);

} LV2_Blob_Support;


#endif /* LV2_ATOM_H */

