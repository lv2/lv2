/*
  Copyright (C) 2008-2011 David Robillard <http://drobilla.net>
 
  This header is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2 of the License, or
  (at your option) any later version.
 
  This header is distributed in the hope that it will be useful, but WITHOUT
  ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
  FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General Public
  License for more details.
 
  You should have received a copy of the GNU Lesser General Public License
  along with this header; if not, write to the Free Software Foundation,
  Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
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

#define LV2_ATOM_URI         "http://lv2plug.in/ns/ext/atom"
#define LV2_BLOB_SUPPORT_URI "http://lv2plug.in/ns/ext/atom#blobSupport"

#define LV2_ATOM_REFERENCE_TYPE 0

#include <stdint.h>
#include <stddef.h>

#define LV2_ATOM_FROM_EVENT(ev) ((LV2_Atom*)&((LV2_Event*)ev)->type)

/**
   An LV2 Atom.
 
   An "Atom" is a generic chunk of memory with a given type and size.
   The type field defines how to interpret an atom.
 
   All atoms are by definition Plain Old Data (POD) and may be safely copied
   (e.g. with memcpy) using the size field, except atoms with type 0. An atom
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
   Reference, an LV2_Atom with type 0.
*/
typedef LV2_Atom LV2_Atom_Reference;

/**
   The body of an atom:String.
*/
typedef struct _LV2_Atom_String {
	uint32_t lang;  /**< The ID of the language of this string */
	uint8_t  str[]; /**< Null-terminated string data in UTF-8 encoding */
} LV2_Atom_String;

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

/* Optional Blob Support */

/**
   Dynamically Allocated LV2 Blob.
 
   This is an opaque blob of data of any type, dynamically allocated in memory.
   Unlike an LV2_Atom, a blob is not necessarily POD. Plugins MUST only refer
   to blobs via a Reference (an LV2_Atom with type 0), there is no way for a
   plugin to directly copy or destroy a Blob.
 
   This is a pointer to host data which is opaque to the plugin. Plugins MUST
   NOT interpret this data in any way, except via host-provided functions in
   LV2_Blob_Support.
*/
typedef void* LV2_Blob;

typedef void* LV2_Blob_Support_Data;

typedef void (*LV2_Blob_Destroy)(LV2_Blob* blob);

/**
   The data field of the LV2_Feature for atom:BlobSupport.
 
   A host which supports blobs must pass an LV2_Feature to the plugin's
   instantiate method with 'URI' = "http://lv2plug.in/ns/ext/atom#BlobSupport"
   and 'data' pointing to an instance of this struct. All fields of this struct
   MUST be set to non-NULL values by the host, except possibly 'data'.
*/
typedef struct {

	/**
	   Pointer to opaque host data.
	 
	   The plugin MUST pass this to any call to functions in this struct.
	   Otherwise, the plugin MUST NOT interpret this value in any way.
	*/
	LV2_Blob_Support_Data data;

	/**
	   The size of a reference, in bytes.
	 
	   This value is provided by the host so plugins can allocate large enough
	   chunks of memory to store references. Note a reference is an LV2_Atom
	   with type atom:Reference, hence ref_size is a uint16, like
	   LV2_Atom.size.
	*/
	uint16_t ref_size;
	
	/**
	   Return the Blob referred to by @a ref.
	 
	   The returned value MUST NOT be used in any way other than by calling
	   methods defined in LV2_Blob_Support (e.g. it MUST NOT be directly
	   accessed, copied, or destroyed). The actual payload of the blob can
	   be accessed with LV2_Blob_Support.blob_get.
	*/
	LV2_Blob (*ref_get)(LV2_Blob_Support_Data data,
	                    LV2_Atom_Reference*   ref);

	/**
	   Copy a reference.
	   This copies a reference but not the blob it refers to,
	   i.e. after this call @a dst and @a src refer to the same LV2_Blob.
	*/
	void (*ref_copy)(LV2_Blob_Support_Data data,
	                 LV2_Atom_Reference*   dst,
	                 LV2_Atom_Reference*   src);

	/**
	   Reset (release) a reference.
	   After this call, @a ref is invalid. Implementations must be sure to
	   call this function when necessary, or memory leaks will result. The
	   specific times this is necessary MUST be defined by any extensions that
	   define a mechanism for transporting atoms. The standard semantics are:
	   <ul><li>Whenever passed a Reference (e.g. via a Port) and run, the
	   plugin owns that reference.</li>
	   <li>The plugin owns any reference it creates (e.g. by using blob_new or
	   ref_copy).</li>
	   <li>For any reference it owns, the plugin MUST either:
	   <ul><li>Copy the reference and store it (to be used in future runs and
	   released later).</li>
	   <li>Copy the reference to an output port exactly once.</li>
	   <li>Release it with ref_reset.</li></ul></li>
	   </ul>
	*/
	void (*ref_reset)(LV2_Blob_Support_Data data,
	                  LV2_Atom_Reference*   ref);

	/**
	   Initialize a reference to point to a newly allocated Blob.

	   @param data Must be the data member of this struct.
	   @param ref Pointer to an area of memory at least as large as
	   the ref_size field of this struct. On return, this will
	   be the unique reference to the new blob, which is owned by the
	   caller. Assumed to be uninitialised, i.e. the caller MUST NOT
	   pass a valid reference since this could cause a memory leak.
	   @param destroy Function to destroy this blob. This function
	   MUST clean up any resources contained in the blob, but MUST NOT
	   attempt to free the memory pointed to by its LV2_Blob* parameter
	   (since this is allocated by the host).
	   @param type ID of type of blob to allocate.
	   @param size Size of blob to allocate in bytes.
	*/
	void (*blob_new)(LV2_Blob_Support_Data data,
	                 LV2_Atom_Reference*   ref,
	                 LV2_Blob_Destroy      destroy,
	                 uint32_t              type,
	                 size_t                size);
	
	/**
	   Get blob's type as an ID.
	 
	   The return value may be any type URI, mapped to an integer with the
	   URI Map extension with <code>context = NULL</code>.
	*/
	uint32_t (*blob_type)(LV2_Blob blob);

	/**
	   Get blob's body.
	 
	   Returns a pointer to the start of the blob data. The format of this
	   data is defined by the return value of the type method. It MUST NOT
	   be used in any way by code which does not understand that type.
	*/
	void* (*blob_data)(LV2_Blob blob);
	
} LV2_Blob_Support;

#endif /* LV2_ATOM_H */
