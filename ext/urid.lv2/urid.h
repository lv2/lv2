/*
  Copyright 2011 Gabriel M. Beddingfield <gabrbedd@gmail.com>
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
   @file
   C header for the LV2 URID extension <http://lv2plug.in/ns/ext/urid>
*/

#ifndef LV2_URID_H
#define LV2_URID_H

#define LV2_URID_URI "http://lv2plug.in/ns/ext/urid"

#include <stdint.h>

/**
   Opaque pointer to host data.
*/
typedef void* LV2_URID_Callback_Data;

/**
   URI mapped to an integer.
*/
typedef uint32_t LV2_URID;

/**
   URID Feature.

   To support this feature the host must pass an LV2_Feature struct to the
   plugin's instantiate method with URI "http://lv2plug.in/ns/ext/urid#URIMap"
   and data pointed to an instance of this struct.
*/
typedef struct {

	/**
	   Opaque pointer to host data.

	   The plugin MUST pass this to any call to functions in this struct.
	   Otherwise, it must not be interpreted in any way.
	*/
	LV2_URID_Callback_Data callback_data;

	/**
	   Get the numeric ID of a URI from the host.

	   If the ID does not already exist, it will be created.

	   @param callback_data Must be the callback_data member of this struct.
	   @param uri The URI to be mapped to an integer ID.

	   This function is referentially transparent; any number of calls with the
	   same arguments is guaranteed to return the same value over the life of a
	   plugin instance.  However, this function is not necessarily very
	   fast or RT-safe: plugins SHOULD cache any IDs they might need in
	   performance critical situations.

	   The return value 0 is reserved and indicates that an ID for that URI
	   could not be created for whatever reason.  However, hosts SHOULD NOT
	   return 0 from this function in non-exceptional circumstances (i.e. the
	   URI map SHOULD be dynamic).
	*/
	LV2_URID (*map_uri)(LV2_URID_Callback_Data callback_data,
	                    const char*            uri);

	/**
	   Get the URI for a given numeric ID from the host.

	   @param callback_data Must be the callback_data member of this struct.
	   @param id The ID to be mapped back to the URI string.

	   Returns NULL if @c id is not yet mapped.  Otherwise, the corresponding
	   URI is returned in a canonical form.  This may not be the exact same
	   string that was originally passed to map_uri(), but it will be an
	   identical URI according to the URI spec.  A non-NULL return for a given
	   @c id will always be the same for the life of the plugin.  Plugins that
	   intend to perform string comparison on unmapped URIs should first
	   canonicalise URI strings with a call to map_uri() followed by a call to
	   unmap_uri().
	*/
	const char* (*unmap_uri)(LV2_URID_Callback_Data callback_data,
	                         LV2_URID               urid);

} LV2_URID_Feature;

#endif /* LV2_URID_H */
