/*
  Copyright 2011 David Robillard <http://drobilla.net>

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

#ifndef LV2_UI_RESIZE_H
#define LV2_UI_RESIZE_H

#define LV2_UI_RESIZE_URI "http://lv2plug.in/ns/ext/ui-resize"

typedef void* LV2_UI_Resize_Feature_Data;

/**
   UI Resize Feature.

   This structure may be used in two ways: as a feature passed by the host
   (e.g. via the features parameter of LV2UI_Descriptor::instantiate()) or
   as a feature exposed by a UI (e.g. via LV2UI_Descriptor::extension_data()).

   In both cases, the URI to be used is
   http://lv2plug.in/ns/ext/ui-resize#UIResize
*/
typedef struct {

	LV2_UI_Resize_Feature_Data data;

	/**
	   Request or notify a size change.

	   When this struct is provided by the host, the UI may call this
	   function to notify the host that a size change is desired, or notify
	   the host of the initial size of the UI.

	   When this struct is provided by the plugin, the host may call this
	   function in the UI thread to notify the UI that it should change its
	   size to the given dimensions.

	   @return 0 on success.
	*/
	int (*ui_resize)(LV2_UI_Resize_Feature_Data data,
	                 int                        width,
	                 int                        height);

} LV2_UI_Resize_Feature;

#endif  /* LV2_UI_RESIZE_H */

