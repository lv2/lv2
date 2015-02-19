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

#ifndef LV2_FEATURE_HPP
#define LV2_FEATURE_HPP

#include <string.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

namespace lv2 {

/**
   Feature.

   Features allow hosts to make additional functionality available to plugins
   without requiring modification to the LV2 API.  Extensions may define new
   features and specify the `URI` and `data` to be used if necessary.
   Some features, such as lv2:isLive, do not require the host to pass data.
*/
template<typename Data, bool Required>
class Feature
{
public:
	/**
	   Initialize feature by retrieving data from the host.

	   @param features Feature array passed by the host.
	   @param uri URI of this feature.
	   @param valid Set to false iff feature is required but unsupported.
	*/
	Feature(const LV2_Feature*const* features,
	        const char*              uri,
	        bool*                    valid)
		: m_data(nullptr)
		, m_supported(false)
	{
		for (const LV2_Feature*const* f = features; *f; ++f) {
			if (!strcmp((*f)->URI, uri)) {
				m_data      = (Data*)(*f)->data;
				m_supported = true;
				break;
			}
		}
		if (Required && !m_supported) {
			*valid = false;
		}
	}

	Data* data()      const { return m_data; }
	bool  supported() const { return m_supported; }

protected:
	Data* m_data;
	bool  m_supported;
};

} // namespace lv2

#endif // LV2_URID_HPP
