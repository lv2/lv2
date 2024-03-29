// Copyright 2008-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_DATA_ACCESS_DATA_ACCESS_H
#define LV2_DATA_ACCESS_DATA_ACCESS_H

/**
   @defgroup data-access Data Access
   @ingroup lv2

   Access to plugin extension_data() for UIs.

   See <http://lv2plug.in/ns/ext/data-access> for details.

   @{
*/

// clang-format off

#define LV2_DATA_ACCESS_URI    "http://lv2plug.in/ns/ext/data-access"  ///< http://lv2plug.in/ns/ext/data-access
#define LV2_DATA_ACCESS_PREFIX LV2_DATA_ACCESS_URI "#"                 ///< http://lv2plug.in/ns/ext/data-access#

// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

/**
   The data field of the LV2_Feature for this extension.

   To support this feature the host must pass an LV2_Feature struct to the
   instantiate method with URI "http://lv2plug.in/ns/ext/data-access"
   and data pointed to an instance of this struct.
*/
typedef struct {
  /**
     A pointer to a method the UI can call to get data (of a type specified
     by some other extension) from the plugin.

     This call never is never guaranteed to return anything, UIs should
     degrade gracefully if direct access to the plugin data is not possible
     (in which case this function will return NULL).

     This is for access to large data that can only possibly work if the UI
     and plugin are running in the same process.  For all other things, use
     the normal LV2 UI communication system.
  */
  const void* (*data_access)(const char* uri);
} LV2_Extension_Data_Feature;

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_DATA_ACCESS_DATA_ACCESS_H
