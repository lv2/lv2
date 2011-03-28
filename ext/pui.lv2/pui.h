/************************************************************************
 *
 * Plugin UI extension for LV2
 *
 * Copyright (C) 2006-2011 Lars Luthman <mail@larsluthman.net>
 * 
 * Based on lv2.h, which was
 *
 * Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis, 
 *                         Stefan Westerfeld
 * Copyright (C) 2006 Steve Harris, David Robillard.
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2.1 of the License,
 * or (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
 * Lesser General Public License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
 * USA.
 *
 ***********************************************************************/

/**
   @file pui.h C API for the LV2 UI extension <http://lv2plug.in/ns/ext/pui>.

   This file specifies a C API for communication between an LV2 host and an
   LV2 UI. The interface is similar to the one used for actual LV2 plugins.
    
   The entry point is the function lv2ui_descriptor().
*/

#ifndef LV2_PUI_H
#define LV2_PUI_H

#include <lv2.h>

/** The URI of this extension (note this is not the same as the prefix). */
#define LV2_PUI_URI "http://lv2plug.in/ns/ext/pui"

/** The numerical ID returned by LV2_PUI_Host_Descriptor::port_index() for
    invalid port symbols. */
#define LV2_PUI_INVALID_PORT_INDEX ((uint32_t)-1)

/** The numerical ID returned by LV2_PUI_Host_Descriptor::port_protocol_id() for
    invalid or unsupported PortProtocols. */
#define LV2_PUI_INVALID_PORT_PROTOCOL_ID 0

/** The full URI for the pui:floatControl PortProtocol. */
#define LV2_PUI_FLOAT_CONTROL_URI "http://lv2plug.in/ns/ext/pui#floatControl"

/** The full URI for the pui:floatPeakRMS PortProtocol. */
#define LV2_PUI_FLOAT_PEAK_RMS_URI "http://lv2plug.in/ns/ext/pui#floatPeakRMS"

#ifdef __cplusplus
extern "C" {
#endif

/** 
   A data type that is used to pass peak and RMS values for a period of
   audio data at an input or output port to an UI, using port_event. See the
   documentation for pui:floatPeakRMS for details about how and when this
   should be done.
*/
typedef struct _LV2_PUI_Peak_RMS_Data {
  
  /**
     The start of the measurement period. This is just a running counter that
     must not be interpreted as any sort of global frame position. It should
     only be interpreted relative to the starts of other measurement periods
     in port_event() calls to the same plugin instance.
     
     This counter is allowed to overflow, in which case it should just wrap
     around.
  */
  uint32_t period_start;
  
  /**
     The size of the measurement period, in the same units as period_start.
  */
  uint32_t period_size;
  
  /**
     The peak value for the measurement period. This should be the maximal
     value for abs(sample) over all the samples in the period.
  */
  float peak;
  
  /**
     The RMS value for the measurement period. This should be the root mean
     square value of the samples in the period, equivalent to
     sqrt((pow(sample1, 2) + pow(sample2, 2) + ... + pow(sampleN, 2)) / N)
     where N is period_size.
  */
  float rms;

} LV2_PUI_Peak_RMS_Data;

/**
   A pointer to a widget or other type of UI.
   The actual type is defined by the type of the UI defined in the RDF data.
   All the functionality provided by this extension is toolkit independent, the
   host only needs to pass the necessary callbacks and display the widget, if
   possible. Plugins may have several UIs, in various toolkits.
*/
typedef void* LV2_PUI_Widget;

/**
   Handle for a particular instance of a UI.
   It is valid to compare this to NULL (0 for C++) but otherwise the host MUST
   NOT attempt to interpret it. The UI may use it to reference internal
   instance data.
*/
typedef void* LV2_PUI_Handle;

/**
   Handle for host functions and data provided to a UI.
   An object of this type is passed to the UI's instantiate() function, and the
   UI must in turn pass it as the first parameter to the callbacks in
   LV2_PUI_Host_Descriptor. The host may use it to reference internal data, such
   as the plugin instance that the UI is associated with. The UI MUST NOT
   interpret the value of an LV2_PUI_Host_Handle in any way.
*/
typedef void* LV2_PUI_Host_Handle;

/**
   Host-provided functions that the UI can use to control the plugin instance.
   
   A pointer to an object of this type is passed to the lv2ui_descriptor()
   function.

   The host MUST provide non-NULL values for all the function pointers.
*/
typedef struct _LV2_PUI_Host_Descriptor {

	/**
	   Send data to one of the plugin's input ports.

	   The @a buffer parameter MUST point to a block of data @a buffer_size
	   bytes large. The contents of this buffer and what the host should do
	   with it depends on the value of the @a port_protocol parameter.
    
	   The @a port_protocol parameter MUST be a numeric ID for a 
	   pui:PortProtocol. Numeric IDs for PortProtocols are retrieved using the 
	   port_protocol_id() function.

	   The @a buffer is only valid during the time of this function call, so if 
	   the host wants to keep it for later use it has to copy the contents to an
	   internal buffer.
    
	   @param host_handle The @a host_handle that was passed to the UI's 
	   instantiate() function.
	   @param port_index The index of the port that the data should be written
	   to, as returned by port_index().
	   @param buffer_size The size of the data pointed to by @a buffer, in
	   bytes.
	   @param port_protocol The numeric ID of the Port Protocol to use,
	   as returned by port_protocol_id().
	*/
	void (*write_port)(LV2_PUI_Host_Handle host_handle,
	                   uint32_t            port_index,
	                   uint32_t            buffer_size,
	                   uint32_t            port_protocol,
	                   void const*         buffer);
  
	/**
	   Return the numerical index for a port.
	   This index is used when writing data to ports using write_port() and whe
	   receiving data using port_event(). If @a port_symbol is not a valid port
	   symbol for @a plugin the host it MUST return
	   LV2_PUI_INVALID_PORT_INDEX. For performance reasons it may be a good idea
	   to cache port indices in the UI at instantiation time.
      
	   @param host_handle The LV2_PUI_Host_Handle that was passed to the UI's 
	   instantiate() function.
	   @param port_symbol The port's symbol, as defined in the RDF data for
	   the plugin.
	*/
	uint32_t (*port_index)(LV2_PUI_Host_Handle host_handle,
	                       char const*         port_symbol);
  
	/**
	   This function is used by the UI, typically at instantiation, to get
	   the numeric IDs that are mapped to certain pui:PortProtocols (see
	   pui.ttl for details). If the host does not support the given
	   pui:PortProtocol it MUST return LV2_PUI_INVALID_PORT_PROTOCOL_ID,
	   but the UI SHOULD not rely on this to find out which protocols
	   are supported, it should check the @a features array passed to
	   instantiate() for this.

	   @param host_handle The @a host_handle that was passed to the UI's 
	   instantiate() function.
	   @param port_protocol_uri The URI of the pui:PortProtocol.
	*/
	uint32_t (*port_protocol_id)(LV2_PUI_Host_Handle host_handle,
	                             char const*         port_protocol_uri);
  
	/**
	   Subscribe to updates for a port.
	   This means that the host will call the UI's port_event() function when
	   the port value changes (as defined by the PortProtocol).
      
	   Calling this function with the same @a port_index and @a port_protocol
	   as an already active subscription has no effect.
      
	   @param host_handle The @a host_handle that was passed to the UI's 
	   instantiate() function.
	   @param port_index The index of the port, as returned by port_index().
	   @param port_protocol The numeric ID of the PortProtocol, as
	   returned by port_protocol_id().
	*/
	void (*add_port_subscription)(LV2_PUI_Host_Handle host_handle,
	                              uint32_t            port_index,
	                              uint32_t            port_protocol);

	/**
	   Unsubscribe to updates for a port.
	   This means that the host will cease calling calling port_event() when
	   the port value changes.
      
	   Calling this function with a @a port_index and @a port_protocol that
	   does not refer to an active port subscription has no effect.
     
	   @param host_handle The @a host_handle that was passed to the UI's 
	   instantiate() function.
	   @param port_index The index of the port, as returned by port_index().
	   @param port_protocol The numeric ID of the PortProtocol, as
	   returned by port_protocol_id().
	*/
	void (*remove_port_subscription)(LV2_PUI_Host_Handle host_handle,
	                                 uint32_t            port_index,
	                                 uint32_t            port_protocol);
  
} LV2_PUI_Host_Descriptor;

/**
   This struct contains the implementation of an UI. A pointer to an 
   object of this type is returned by the lv2ui_descriptor() function. 
*/
typedef struct _LV2_PUI_Descriptor {
  
	/**
	   The URI for this UI (not for the plugin it controls).
	*/
	char const* URI;
  
	/**
	   Create a new UI object and return a handle to it. This function works
	   similarly to the instantiate() member in LV2_Descriptor.
	   
	   @param descriptor The descriptor for the UI to instantiate.
	   
	   @param plugin_uri The URI of the plugin that this UI will control.
	   
	   @param bundle_path The path to the bundle containing the RDF data that
	   references this shared object file, with trailing separator (e.g. '/').
	   
	   @param host_descriptor A pointer to an object that contains function
	   pointers that the UI instance should use to control the plugin
	   instance. This pointer MUST be valid until cleanup() is called for this
	   UI instance.
	   
	   @param host_handle A handle that the host may use to reference internal
	   data. It MUST be passed as the first parameter to the function
	   pointers in @a host_descriptor, and MUST NOT be interpreted in any other
	   way by the UI.
	   
	   @param features A NULL-terminated array of LV2_Feature pointers. The
	   host must pass all feature URIs that it and the UI supports and any
	   additional data, just like in the LV2 plugin instantiate()
	   function. Note that UI features and plugin features are NOT necessarily
	   the same; @a features will probably not be the same array as the one the
	   plugin host passes to a plugin.
	*/
	LV2_PUI_Handle (*instantiate)(struct _LV2_PUI_Descriptor const* descriptor,
	                             char const*                        plugin_uri,
	                             char const*                        bundle_path,
	                             LV2_PUI_Host_Descriptor const*     host_descriptor,
	                             LV2_PUI_Host_Handle                host_handle,
	                             LV2_Feature const* const*          features);

	/**
	   Return the widget pointer for the UI object.
	   This MUST return the same value during the entire lifetime of the UI
	   object.
	*/
	LV2_PUI_Widget (*get_widget)(LV2_PUI_Handle ui);

	/**
	   Destroy the UI object and the associated widget.
	   The host must not try to access the widget after calling this function.
	*/
	void (*cleanup)(LV2_PUI_Handle ui);
  
	/**
	   Notify the UI that something has happened to a subscribed port.

	   This is called by the host when something happens at a plugin port that
	   has been subscribed to using
	   LV2_PUI_Host_Descriptor::add_port_subscription().
      
	   The @a buffer is only valid during the time of this function call, so if 
	   the UI wants to keep it for later use it has to copy the contents to an
	   internal buffer.
      
	   @param ui A handle for the UI object.
	   @param port_index The index of the port that has changed, as returned by
	   LV2_PUI_Host_Descriptor::port_index().
	   @param buffer_size The size of the data buffer in bytes.
	   @param port_protocol The format of the data buffer, as returned by
	   LV2_PUI_Host_Descriptor::port_protocol_id().
	   @param buffer A pointer to the data buffer.
	*/
	void (*port_event)(LV2_PUI_Handle ui,
	                   uint32_t       port_index,
	                   uint32_t       buffer_size,
	                   uint32_t       port_protocol,
	                   void const*    buffer);
  
	/**
	   Return a data structure associated with an extension URI.

	   This facility can be used by extensions to extend the LV2_PUI_Descriptor
	   API. This function adheres to the same rules as
	   LV2_Descriptor::extension_data, except it applies to UIs rather than
	   plugins.
	*/
	void const* (*extension_data)(char const*  uri);

} LV2_PUI_Descriptor;

/**
   Prototype for UI accessor function.

   This function follows the same rules as lv2_desciprotr(), except it applies
   to UIs rather than plugins.
*/
LV2_PUI_Descriptor const* lv2ui_descriptor(uint32_t index);

/**
   Type of the lv2ui_descriptor() function in a UI library.
*/
typedef LV2_PUI_Descriptor const* (*LV2_PUI_DescriptorFunction)(uint32_t index);

#ifdef __cplusplus
}
#endif

#endif /* LV2_PUI_H */
