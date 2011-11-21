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
   @file reference.h C header for the LV2 Reference extension
   <http://lv2plug.in/ns/ext/reference>.
*/

#ifndef LV2_REFERENCE_H
#define LV2_REFERENCE_H

#define LV2_REFERENCE_URI              "http://lv2plug.in/ns/ext/reference"
#define LV2_REFERENCE_BLOB_SUPPORT_URI LV2_REFERENCE_URI "#blobSupport"

#include <stdint.h>
#include <stddef.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

/**
   Dynamically Allocated Data.

   This is an opaque piece of data of any type, dynamically allocated in memory.
   Unlike an "atom", a "blob" is not necessarily POD.  Non-POD data is referred
   to by a "reference (a special case of atom with type 0).

   This is a pointer to host data which is opaque to the plugin.  Plugins MUST
   NOT interpret this data in any way, except via host-provided functions in
   LV2_Blob_Support.
*/
typedef void* LV2_Blob;

typedef LV2_Atom LV2_Reference;

typedef void* LV2_Blob_Support_Data;

typedef void (*LV2_Blob_Destroy)(LV2_Blob* blob);

/**
   The data field of the LV2_Feature for reference:blobSupport.

   A host which supports blobs must pass an LV2_Feature to the plugin's
   instantiate method with 'URI' = "http://lv2plug.in/ns/ext/reference#blobSupport"
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
	   chunks of memory to store references. Note a reference is an LV2_Reference
	   with type reference:Reference, hence ref_size is a uint16, like
	   LV2_Reference.size.
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
	                    LV2_Reference*        ref);

	/**
	   Copy a reference.
	   This copies a reference but not the blob it refers to,
	   i.e. after this call @a dst and @a src refer to the same LV2_Blob.
	*/
	void (*ref_copy)(LV2_Blob_Support_Data data,
	                 LV2_Reference*        dst,
	                 LV2_Reference*        src);

	/**
	   Reset (release) a reference.
	   After this call, @a ref is invalid. Implementations must be sure to
	   call this function when necessary, or memory leaks will result. The
	   specific times this is necessary MUST be defined by any extensions that
	   define a mechanism for transporting references. The standard semantics are:
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
	                  LV2_Reference*        ref);

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
	                 LV2_Reference*        ref,
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

#endif /* LV2_REFERENCE_H */
