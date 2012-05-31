/*
  Copyright 2012 David Robillard <http://drobilla.net>

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

#ifndef LV2_MORPH_H
#define LV2_MORPH_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define LV2_MORPH_URI    "http://lv2plug.in/ns/ext/morph"
#define LV2_MORPH_PREFIX LV2_MORPH_URI "#"

#define LV2_MORPH__AutoMorphPort LV2_MORPH_PREFIX "AutoMorphPort"
#define LV2_MORPH__MorphPort     LV2_MORPH_PREFIX "MorphPort"
#define LV2_MORPH__interface     LV2_MORPH_PREFIX "interface"
#define LV2_MORPH__supportsType  LV2_MORPH_PREFIX "supportsType"

#ifdef __cplusplus
extern "C" {
#else
#    include <stdbool.h>
#endif

typedef enum {
	LV2_MORPH_SUCCESS      = 0,  /**< Completed successfully. */
	LV2_MORPH_ERR_UNKNOWN  = 1,  /**< Unknown error. */
	LV2_MORPH_ERR_BAD_TYPE = 2,  /**< Unsupported type. */
	LV2_MORPH_ERR_BAD_PORT = 3   /**< Port is not morphable. */
} LV2_Morph_Status;

/** A port property. */
typedef struct {
	LV2_URID    key;    /**< Key (predicate) */
	uint32_t    size;   /**< Value size */
	LV2_URID    type;   /**< Value type */
	const void* value;  /**< Value (object) */
} LV2_Morph_Property;

/** The interface provided by a plugin to support morph ports. */
typedef struct {
	/**
	   Morph a port to a different type.
	 
	   This function is in the audio threading class.

	   This function MAY return an error, in which case the port's type was not
	   changed.  If the type was changed and the plugin has AutoMorphPort
	   ports, the host MUST check the type of every AutoMorphPort using the
	   port_type() function since they may have changed.

	   This function MUST gracefully handle being called for ports that are not
	   MorpPorts by ignoring the request and returning LV2_MORPH_ERR_BAD_PORT.

	   A NULL-terminated array of additional properties to set on the port may
	   be passed via @p properties.  These properties and their values are
	   owned by the caller and valid only for the duration of the call.

	   @param instance The plugin instance.
	   @param port The index of the port to change the type of.
	   @param type The new port type URID.
	   @param properties Additional properties to set, or NULL.
	*/
	LV2_Morph_Status (*morph_port)(LV2_Handle                      instance,
	                               uint32_t                        port,
	                               LV2_URID                        type,
	                               const LV2_Morph_Property*const* properties);

	/**
	   Get the type of an AutoMorphPort.

	   This function is in the audio threading class.

	   If the plugin has no auto morph ports, this field may be NULL.  This
	   function may only be called for ports which are AutoMorphPorts.

	   This function MAY return 0, which indicates that the current
	   configuration of MorphPort types is invalid and the port is
	   non-functional.  If the port is not lv2:connectionOptional, then the
	   plugin MUST NOT be used.

	   The @p properties parameter may be used to get additional properties of
	   the port.  To do so, the host passes a NULL-terminated property array
	   with keys set to the desired properties and all other fields zeroed.
	   The plugin sets these fields appropriately if possible.  The data
	   pointed to is owned by the plugin and only valid until the next call to
	   a method on this plugin (this mechanism is only meant for accessing
	   simple properties, such as buffer size).

	   @param instance The plugin instance.
	   @param port The index of the port to return the type of.
	   @param properties Additional properties to get, or NULL.
	   @return The current type of the port.
	*/
	LV2_URID (*port_type)(LV2_Handle                instance,
	                      uint32_t                  port,
	                      LV2_Morph_Property*const* properties);
} LV2_Morph_Interface;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_MORPH_H */
