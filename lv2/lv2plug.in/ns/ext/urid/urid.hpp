/*
  Copyright 2015 David Robillard <http://drobilla.net>

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

#ifndef LV2_URID_HPP
#define LV2_URID_HPP

#include "lv2/lv2plug.in/ns/lv2core/Feature.hpp"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

namespace lv2 {
namespace urid {

/**
   URI mapped to an integer.
*/
typedef LV2_URID URID;

/**
   URID Map Feature (LV2_URID__map)
*/
template<bool Required>
class Map : public Feature<LV2_URID_Map, Required>
{
public:
	Map(const LV2_Feature*const* features,
	    bool*                    valid)
		: Feature<LV2_URID_Map, true>(features, LV2_URID__map, valid)
	{}

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
	LV2_URID map(const char* uri) {
		return this->m_data->map(this->m_data->handle, uri);
	}

	/**
	   Convenience wrapper for calling map().
	*/
	LV2_URID operator()(const char* uri) { return map(uri); }
};

/**
   URI Unmap Feature (LV2_URID__unmap)
*/
template<bool Required>
class Unmap : public Feature<LV2_URID_Unmap, Required>
{
public:
	Unmap(const LV2_Feature*const* features,
	      bool*                    valid)
		: Feature<LV2_URID_Map, true>(features, LV2_URID__unmap, valid)
	{}

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
	const char* unmap(LV2_URID urid) {
		return this->m_data->unmap(this->m_data->handle, urid);
	}

	/**
	   Convenience wrapper for calling unmap().
	*/
	const char* operator()(LV2_URID urid) { return unmap(urid); }
};

} // namespace urid
} // namespace lv2

#endif // LV2_URID_HPP
