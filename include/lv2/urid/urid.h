// Copyright 2008-2016 David Robillard <d@drobilla.net>
// Copyright 2011 Gabriel M. Beddingfield <gabrbedd@gmail.com>
// SPDX-License-Identifier: ISC

#ifndef LV2_URID_URID_H
#define LV2_URID_URID_H

/**
   @defgroup urid URID
   @ingroup lv2

   Features for mapping URIs to and from integers.

   See <http://lv2plug.in/ns/ext/urid> for details.

   @{
*/

// clang-format off

#define LV2_URID_URI    "http://lv2plug.in/ns/ext/urid"  ///< http://lv2plug.in/ns/ext/urid
#define LV2_URID_PREFIX LV2_URID_URI "#"                 ///< http://lv2plug.in/ns/ext/urid#

#define LV2_URID__map   LV2_URID_PREFIX "map"    ///< http://lv2plug.in/ns/ext/urid#map
#define LV2_URID__unmap LV2_URID_PREFIX "unmap"  ///< http://lv2plug.in/ns/ext/urid#unmap

#define LV2_URID_MAP_URI   LV2_URID__map    ///< Legacy
#define LV2_URID_UNMAP_URI LV2_URID__unmap  ///< Legacy

// clang-format on

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Opaque pointer to host data for LV2_URID_Map.
*/
typedef void* LV2_URID_Map_Handle;

/**
   Opaque pointer to host data for LV2_URID_Unmap.
*/
typedef void* LV2_URID_Unmap_Handle;

/**
   URI mapped to an integer.
*/
typedef uint32_t LV2_URID;

/**
   URID Map Feature (LV2_URID__map)
*/
typedef struct {
  /**
     Opaque pointer to host data.

     This MUST be passed to map_uri() whenever it is called.
     Otherwise, it must not be interpreted in any way.
  */
  LV2_URID_Map_Handle handle;

  /**
     Get the numeric ID of a URI.

     If the ID does not already exist, it will be created.

     This function is referentially transparent; any number of calls with the
     same arguments is guaranteed to return the same value over the life of a
     plugin instance.  Note, however, that several URIs MAY resolve to the
     same ID if the host considers those URIs equivalent.

     This function is not necessarily very fast or RT-safe: plugins SHOULD
     cache any IDs they might need in performance critical situations.

     The return value 0 is reserved and indicates that an ID for that URI
     could not be created for whatever reason.  However, hosts SHOULD NOT
     return 0 from this function in non-exceptional circumstances (i.e. the
     URI map SHOULD be dynamic).

     @param handle Must be the callback_data member of this struct.
     @param uri The URI to be mapped to an integer ID.
  */
  LV2_URID (*map)(LV2_URID_Map_Handle handle, const char* uri);
} LV2_URID_Map;

/**
   URI Unmap Feature (LV2_URID__unmap)
*/
typedef struct {
  /**
     Opaque pointer to host data.

     This MUST be passed to unmap() whenever it is called.
     Otherwise, it must not be interpreted in any way.
  */
  LV2_URID_Unmap_Handle handle;

  /**
     Get the URI for a previously mapped numeric ID.

     Returns NULL if `urid` is not yet mapped.  Otherwise, the corresponding
     URI is returned in a canonical form.  This MAY not be the exact same
     string that was originally passed to LV2_URID_Map::map(), but it MUST be
     an identical URI according to the URI syntax specification (RFC3986).  A
     non-NULL return for a given `urid` will always be the same for the life
     of the plugin.  Plugins that intend to perform string comparison on
     unmapped URIs SHOULD first canonicalise URI strings with a call to
     map_uri() followed by a call to unmap_uri().

     @param handle Must be the callback_data member of this struct.
     @param urid The ID to be mapped back to the URI string.
  */
  const char* (*unmap)(LV2_URID_Unmap_Handle handle, LV2_URID urid);
} LV2_URID_Unmap;

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_URID_URID_H
