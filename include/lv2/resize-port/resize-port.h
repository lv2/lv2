// Copyright 2007-2016 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#ifndef LV2_RESIZE_PORT_RESIZE_PORT_H
#define LV2_RESIZE_PORT_RESIZE_PORT_H

/**
   @defgroup resize-port Resize Port
   @ingroup lv2

   Dynamically sized LV2 port buffers.

   @{
*/

#include <stddef.h>
#include <stdint.h>

// clang-format off

#define LV2_RESIZE_PORT_URI    "http://lv2plug.in/ns/ext/resize-port"  ///< http://lv2plug.in/ns/ext/resize-port
#define LV2_RESIZE_PORT_PREFIX LV2_RESIZE_PORT_URI "#"                 ///< http://lv2plug.in/ns/ext/resize-port#

#define LV2_RESIZE_PORT__asLargeAs   LV2_RESIZE_PORT_PREFIX "asLargeAs"    ///< http://lv2plug.in/ns/ext/resize-port#asLargeAs
#define LV2_RESIZE_PORT__minimumSize LV2_RESIZE_PORT_PREFIX "minimumSize"  ///< http://lv2plug.in/ns/ext/resize-port#minimumSize
#define LV2_RESIZE_PORT__resize      LV2_RESIZE_PORT_PREFIX "resize"       ///< http://lv2plug.in/ns/ext/resize-port#resize

// clang-format on

#ifdef __cplusplus
extern "C" {
#endif

/** A status code for state functions. */
typedef enum {
  LV2_RESIZE_PORT_SUCCESS      = 0, /**< Completed successfully. */
  LV2_RESIZE_PORT_ERR_UNKNOWN  = 1, /**< Unknown error. */
  LV2_RESIZE_PORT_ERR_NO_SPACE = 2  /**< Insufficient space. */
} LV2_Resize_Port_Status;

/** Opaque data for resize method. */
typedef void* LV2_Resize_Port_Feature_Data;

/** Host feature to allow plugins to resize their port buffers. */
typedef struct {
  /** Opaque data for resize method. */
  LV2_Resize_Port_Feature_Data data;

  /**
     Resize a port buffer to at least `size` bytes.

     This function MAY return an error, in which case the port buffer was not
     resized and the port is still connected to the same location.  Plugins
     MUST gracefully handle this situation.

     This function is in the audio threading class.

     The host MUST preserve the contents of the port buffer when resizing.

     Plugins MAY resize a port many times in a single run callback.  Hosts
     SHOULD make this as inexpensive as possible.
  */
  LV2_Resize_Port_Status (*resize)(LV2_Resize_Port_Feature_Data data,
                                   uint32_t                     index,
                                   size_t                       size);
} LV2_Resize_Port_Resize;

#ifdef __cplusplus
} /* extern "C" */
#endif

/**
   @}
*/

#endif // LV2_RESIZE_PORT_RESIZE_PORT_H
