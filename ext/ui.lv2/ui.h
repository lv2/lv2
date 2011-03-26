/************************************************************************
 *
 * In-process UI extension for LV2
 *
 * Copyright (C) 2006-2010 Lars Luthman <mail@larsluthman.net>
 * 
 * Based on lv2.h, which was
 *
 * Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis, 
 *                         Stefan Westerfeld
 * Copyright (C) 2006 Steve Harris, Dave Robillard.
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

/** @file
    This file specifies a C API for communication between an LV2 host and an
    LV2 UI. The interface is similar to the one used for actual LV2 plugins.
    
    The entry point is the function lv2ui_descriptor().
*/

#ifndef LV2_UI_H
#define LV2_UI_H


#include <lv2.h>

/** The URI prefix for this extension. */
#define LV2_UI_URI "http://lv2plug.in/ns/ext/ui"

/** The numerical index returned by LV2_UI_Host_Descriptor::port_index() for
    invalid port symbols. */
#define LV2_UI_INVALID_PORT_INDEX ((uint32_t)-1)

/** The numerical ID returned by LV2_UI_Host_Descriptor::port_protocol_id() for
    invalid or unsupported PortProtocols. */
#define LV2_UI_INVALID_PORT_PROTOCOL_ID ((uint32_t)-1)

/** The full URI for the ui:floatControl PortProtocol. */
#define LV2_UI_FLOAT_CONTROL_URI "http://lv2plug.in/ns/ext/ui#floatControl"

#ifdef __cplusplus
extern "C" {
#endif


/** A pointer to a widget or other type of UI handle.
    The actual type is defined by the type of the UI defined in the RDF data.
    All the functionality provided by this extension is toolkit 
    independent, the host only needs to pass the necessary callbacks and 
    display the widget, if possible. Plugins may have several UIs, in various
    toolkits. */
typedef void* LV2_UI_Widget;


/** This handle indicates a particular instance of a UI.
    It is valid to compare this to NULL (0 for C++) but otherwise the 
    host MUST not attempt to interpret it. The UI plugin may use it to 
    reference internal instance data. */
typedef void* LV2_UI_Handle;


/** An object of this type is passed to the UI's instantiate() function,
    and the UI must in turn pass it as the first parameter to the callbacks
    in LV2_UI_Host_Descriptor. The host may use it to reference internal data,
    such as the plugin instance that the UI is associated with. The UI
    MUST NOT interpret the value of an LV2_UI_Host_Handle in any way. */
typedef void* LV2_UI_Host_Handle;


/** This struct contains pointers to the host-provided functions that the
    UI can use to control the plugin instance. A pointer to an object of this
    type is passed to the lv2ui_descriptor() function.

    The host must provide non-NULL values for all the function pointers.
*/
typedef struct _LV2_UI_Host_Descriptor {

  /** This is a function that the UI can use to send data to a plugin's
      input ports. The @c buffer parameter must point to a block of data,
      @c buffer_size bytes large. The contents of this buffer and what the
      host should do with it depends on the value of the @c port_protocol
      parameter.
    
      The @c port_protocol parameter should be a numeric ID for a 
      ui:PortProtocol. Numeric IDs for PortProtocols are retrieved using the 
      port_protocol_id() function.

      The @c buffer is only valid during the time of this function call, so if 
      the host wants to keep it for later use it has to copy the contents to an
      internal buffer.
    
      @param host_handle The LV2_UI_Host_Handle that was passed to the UI's 
                         instantiate() function.
      @param port_index The index of the port that the data should be written
                        to, as returned by port_index().
      @param buffer_size The size of the data pointer to by @c buffer, in
                         bytes.
      @param port_protocol The numeric ID for the Port Protocol to use,
                           as returned by port_protocol_id().
  */
  void (*write_port)(LV2_UI_Host_Handle host_handle,
		     uint32_t           port_index,
		     uint32_t           buffer_size,
		     uint32_t           port_protocol,
		     void const*        buffer);
  
  /** Returns a numerical index for a port. This index is used when writing
      data to ports using write_port() and whe receiving data using 
      port_event(). If @c port_symbol is not a valid port symbol for @c plugin
      the host it MUST return LV2_UI_INVALID_PORT_INDEX.
      
      @param host_handle The LV2_UI_Host_Handle that was passed to the UI's 
                         instantiate() function.
      @param port_symbol A port symbol, as defined in the RDF data for
                         the plugin.
  */
  uint32_t (*port_index)(LV2_UI_Host_Handle host_handle,
			 char const*        port_symbol);
  
  /** This function is used by the UI, typically at instantiation, to get
      the numeric IDs that are mapped to certain ui:PortProtocols (see
      ui.ttl for details). If the host does not support the given
      ui:PortProtocol it MUST return LV2_UI_INVALID_PORT_PROTOCOL_ID.

      As a special case, when @c port_protocol_uri is LV2_UI_FLOAT_CONTROL_URI
      and ui:floatControl is listed as a required Feature for the UI, this
      function MUST return 0. The UI may assume this and skip the call.
      
      @param host_handle The LV2_UI_Host_Handle that was passed to the UI's 
                         instantiate() function.
      @param port_protocol_uri The URI of the ui:PortProtocol.
  */
  uint32_t (*port_protocol_id)(LV2_UI_Host_Handle host_handle,
			       char const*        port_protocol_uri);
  
  /** Add a port subscription. This means that the host will call the UI's
      port_event() function when the port value changes (as defined by
      the PortProtocol).
      
      Calling this function with the same @c port_index and @c port_protocol
      as an already active subscription has no effect.
      
      @param host_handle   The LV2_UI_Host_Handle that was passed to the UI's 
                           instantiate() function.
      @param port_index    The index for the port, as returned by port_index().
      @param port_protocol The numeric ID for the PortProtocol, as
                           returned by port_protocol_id.
   */
  void (*add_port_subscription)(LV2_UI_Host_Handle host_handle,
				uint32_t           port_index,
				uint32_t           port_protocol);

  /** Remove a port subscription that has been added previously using
      add_port_subscription, i.e. tell the host to stop calling port_event()
      when the port value changes.
      
      Calling this function with a @c port_index and @c port_protocol that
      does not define an active port subscription has no effect.
     
      @param host_handle   The LV2_UI_Host_Handle that was passed to the UI's 
                           instantiate() function.
      @param port_index    The index for the port, as returned by port_index().
      @param port_protocol The numeric ID for the PortProtocol, as
                           returned by port_protocol_id.
   */
  void (*remove_port_subscription)(LV2_UI_Host_Handle host_handle,
				   uint32_t           port_index,
				   uint32_t           port_protocol);
  
} LV2_UI_Host_Descriptor;



/** This struct contains the implementation of an UI. A pointer to an 
    object of this type is returned by the lv2ui_descriptor() function. 
*/
typedef struct _LV2_UI_Descriptor {
  
  /** The URI for this UI (not for the plugin it controls). */
  char const* URI;
  
  /** Create a new UI object and return a handle to it. This function works
      similarly to the instantiate() member in LV2_Descriptor.
      
      @param descriptor The descriptor for the UI that you want to instantiate.
      @param plugin_uri The URI of the plugin that this UI will control.
      @param bundle_path The path to the bundle containing the RDF data file
                         that references this shared object file, including the
                         trailing '/'.
      @param host_descriptor A pointer to an object that contains function
                             pointers that the UI instance should use to
                             control the plugin instance. This pointer MUST
			     be valid until cleanup() is called for this UI
			     instance.
      @param host_handle A handle that the host may use to reference internal
                         data. It should be passed as the first parameter
                         to the function pointers in @c host_descriptor, and
			 MUST NOT be interpreted in any other way by the UI.
      @param features   A NULL-terminated array of LV2_Feature pointers. The
                        host must pass all feature URIs that it and the UI
                        supports and any additional data, just like in the
                        LV2 plugin instantiate() function. Note that UI
                        features and plugin features are NOT necessarily the
                        same, they just share the same data structure - this
                        will probably not be the same array as the one the
                        plugin host passes to a plugin.
  */
  LV2_UI_Handle (*instantiate)(struct _LV2_UI_Descriptor const* descriptor,
                              char const*                      plugin_uri,
                              char const*                      bundle_path,
			      LV2_UI_Host_Descriptor const*    host_descriptor,
                              LV2_UI_Host_Handle               host_handle,
                              LV2_Feature const* const*        features);

  /** Return the widget pointer for the UI object. This MUST return the
      same value during the entire lifetime of the UI object.
  */
  LV2_UI_Widget (*get_widget)(LV2_UI_Handle ui);

  /** Destroy the UI object and the associated widget. The host must not try
      to access the widget after calling this function.
   */
  void (*cleanup)(LV2_UI_Handle ui);
  
  /** This is called by the host when something happens at a plugin port that
      a subscription has been added for using
      LV2_UI_Host_Descriptor::add_port_subscription().
      
      The @c buffer is only valid during the time of this function call, so if 
      the UI wants to keep it for later use it has to copy the contents to an
      internal buffer.
      
      @param ui            A handle for the UI object.
      @param port_index    The index of the port for which something has 
                           happened as returned by 
                           LV2_UI_Host_Descriptor::port_index().
      @param buffer_size   The size of the data buffer in bytes.
      @param port_protocol The format of the data buffer, as returned by
                           LV2_UI_Host_Descriptor::port_protocol_id().
      @param buffer        A pointer to the data buffer.
  */
  void (*port_event)(LV2_UI_Handle ui,
                     uint32_t      port_index,
                     uint32_t      buffer_size,
                     uint32_t      port_protocol,
                     void const*   buffer);
  
  /** Returns a data structure associated with an extension URI, for example
      a struct containing additional function pointers. Avoid returning
      function pointers directly since standard C++ has no valid way of
      casting a void* to a function pointer. This member may be set to NULL
      if the UI is not interested in supporting any extensions. This is similar
      to the extension_data() member in LV2_Descriptor.
  */
  void const* (*extension_data)(char const*  uri);

} LV2_UI_Descriptor;



/** A plugin UI programmer must include a function called "lv2ui_descriptor"
    with the following function prototype within the shared object
    file. This function will have C-style linkage (if you are using
    C++ this is taken care of by the 'extern "C"' clause at the top of
    the file). This function will be accessed by the UI host using the 
    @c dlsym() function and called to get a LV2_UI_UIDescriptor for the
    wanted plugin.
    
    Just like lv2_descriptor(), this function takes an index parameter. The
    index should only be used for enumeration and not as any sort of ID number -
    the host should just iterate from 0 and upwards until the function returns
    NULL or a descriptor with an URI matching the one the host is looking for.
*/
LV2_UI_Descriptor const* lv2ui_descriptor(uint32_t index);


/** This is the type of the lv2ui_descriptor() function. */
typedef LV2_UI_Descriptor const* (*LV2_UI_DescriptorFunction)(uint32_t index);



#ifdef __cplusplus
}
#endif


#endif
