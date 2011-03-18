/*
  LV2 - An audio plugin interface specification.
  Copyright (C) 2006-2011 Steve Harris, David Robillard.

  Based on LADSPA:
  Copyright (C) 2000-2002 Richard W.E. Furse, Paul Barton-Davis,
                          Stefan Westerfeld.
 
  This header is free software; you can redistribute it and/or modify it
  under the terms of the GNU Lesser General Public License as published
  by the Free Software Foundation; either version 2.1 of the License,
  or (at your option) any later version.
 
  This header is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the GNU
  Lesser General Public License for more details.
 
  You should have received a copy of the GNU Lesser General Public
  License along with this library; if not, write to the Free Software
  Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA 02110-1301
  USA.
*/

/**
  @file lv2.h
  API for the LV2 specification <http://lv2plug.in/ns/lv2core>.
  Revision: 3.2
*/

#ifndef LV2_H_INCLUDED
#define LV2_H_INCLUDED

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/**
   Plugin Handle.
 
   This handle refers to a particular instance of a plugin. It is valid to
   compare to NULL (or 0 for C++) but otherwise the host MUST NOT attempt to
   interpret it. The plugin may use it to reference internal instance data.
*/
typedef void * LV2_Handle;

/**
   Feature.
 
   Features are used to provide an extensible interface to allow adding
   arbitrary functionality to implementations without breaking API
   compatibility. In particular, they may be passed by the host to the plugin's
   instantiate method to provide additional functionality. Extensions that
   describe a feature MUST specify the @a URI and format of @a data that needs
   to be used here. The core LV2 specification does not define any features;
   implementations are not required to use this facility.
*/
typedef struct _LV2_Feature {
	/**
	   A globally unique, case-sensitive identifier (URI) for this feature.

	   This MUST be a valid URI string as defined by RFC 3986.
	*/
	const char * URI;

	/**
	   Pointer to arbitrary data.

	   The format of this data is defined by the extension which describes the
	   feature with the given @a URI. The core LV2 specification makes no
	   restrictions on the format of this data. If no data is required, this
	   may be set to NULL if the relevant extension explicitly allows this.
	*/
	void * data;
} LV2_Feature;

/**
   Descriptor for an LV2 Plugin.
 
   This structure is used to describe a plugin. It provides the functions
   necessary to instantiate and use a plugin.
*/
typedef struct _LV2_Descriptor {
	/**
	   A globally unique, case-sensitive identifier for this plugin.

	   This MUST be a valid URI string as defined by RFC 3986. All plugins with
	   the same URI are compatible to some degree; see lv2.ttl and the <a
	   href="http://lv2plug.in/ns/lv2core">LV2 Ontology Documentation</a> for
	   details.
	*/
	const char * URI;

	/**
	   Instantiate the plugin.

	   Note that instance initialisation should generally occur in activate()
	   rather than here. If a host calls instantiate(), it MUST call cleanup()
	   at some point in the future.

	   @param descriptor Descriptor of the plugin to instantiate.

	   @param sample_rate Sample rate, in Hz, for the new plugin instance.

	   @param bundle_path Path to the LV2 bundle which contains this plugin
	   binary. It MUST include the trailing directory separator (e.g. '/') so
	   that simply appending a filename will yield the path to that file in the
	   bundle.
	 
	   @param features A NULL terminated array of LV2_Feature structs which
	   represent the features the host supports. Plugins may refuse to
	   instantiate if required features are not found here. However, hosts MUST
	   NOT use this as a discovery mechanism: instead, use the RDF data to
	   determine which features are required and do not attempt to instantiate
	   unsupported plugins at all. This parameter MUST NOT be NULL, i.e. a host
	   that supports no features MUST pass a single element array containing
	   NULL.

	   @return A handle for the new plugin instance, or NULL if instantiation
	   has failed.	 
	*/
	LV2_Handle (*instantiate)(const struct _LV2_Descriptor * descriptor,
	                          double                         sample_rate,
	                          const char *                   bundle_path,
	                          const LV2_Feature *const *     features);

	/**
	   Connect a port on a plugin instance to a memory location.

	   Plugin writers should be aware that the host may elect to use the same
	   buffer for more than one port and even use the same buffer for both
	   input and output (see lv2:inPlaceBroken in lv2.ttl).
	 
	   If the plugin has the feature lv2:hardRTCapable then there are various
	   things that the plugin MUST NOT do within the connect_port() function;
	   see lv2.ttl for details.

	   connect_port() MUST be called at least once for each port before run()
	   is called, unless that port is lv2:connectionOptional. The plugin must
	   pay careful attention to the block size passed to run() since the block
	   allocated may only just be large enough to contain the data, and is not
	   guaranteed to remain constant between run() calls.

	   connect_port() may be called more than once for a plugin instance to
	   allow the host to change the buffers that the plugin is reading or
	   writing. These calls may be made before or after activate() or
	   deactivate() calls.

	   @param instance Plugin instance containing the port.

	   @param port Index of the port to connect. The host MUST NOT try to
	   connect a port index that is not defined in the plugin's RDF data. If
	   it does, the plugin's behaviour is undefined (a crash is likely).

	   @param data_location Pointer to data of the type defined by the port
	   type in the plugin's RDF data (e.g. an array of float for an
	   lv2:AudioPort). This pointer must be stored by the plugin instance and
	   used to read/write data when run() is called. Data present at the time
	   of the connect_port() call MUST NOT be considered meaningful.
	*/
	void (*connect_port)(LV2_Handle instance,
	                     uint32_t   port,
	                     void *     data_location);

	/**
	   Initialise a plugin instance and activate it for use.
	 
	   This is separated from instantiate() to aid real-time support and so
	   that hosts can reinitialise a plugin instance by calling deactivate()
	   and then activate(). In this case the plugin instance MUST reset all
	   state information dependent on the history of the plugin instance except
	   for any data locations provided by connect_port(). If there is nothing
	   for activate() to do then this field may be NULL.
	 
	   When present, hosts MUST call this function once before run() is called
	   for the first time. This call SHOULD be made as close to the run() call
	   as possible and indicates to real-time plugins that they are now live,
	   however plugins MUST NOT rely on a prompt call to run() after
	   activate().

	   The host MUST NOT call activate() again until deactivate() has been
	   called first. If a host calls activate(), it MUST call deactivate() at
	   some point in the future. Note that connect_port() may be called before
	   or after activate().
	*/
	void (*activate)(LV2_Handle instance);

	/**
	   Run a plugin instance for a block.

	   Note that if an activate() function exists then it must be called before
	   run(). If deactivate() is called for a plugin instance then run() may
	   not be called until activate() has been called again.
	 
	   If the plugin has the feature lv2:hardRTCapable then there are various
	   things that the plugin MUST NOT do within the run() function (see
	   lv2.ttl for details).

	   As a special case, when @a sample_count == 0, the plugin should update
	   any output ports that represent a single instant in time (e.g. control
	   ports, but not audio ports). This is particularly useful for latent
	   plugins, which should update their latency output port so hosts can
	   pre-roll plugins to compute latency. Plugins MUST NOT crash when
	   @a sample_count == 0.

	   @param instance Instance to be run.

	   @param sample_count The block size (in samples) for which the plugin
	   instance must run.
	*/
	void (*run)(LV2_Handle instance,
	            uint32_t   sample_count);

	/**
	   Deactivate a plugin instance (counterpart to activate()).

	   Hosts MUST deactivate all activated instances after they have been run()
	   for the last time. This call SHOULD be made as close to the last run()
	   call as possible and indicates to real-time plugins that they are no
	   longer live, however plugins MUST NOT rely on prompt deactivation. If
	   there is nothing for deactivate() to do then this field may be NULL

	   Deactivation is not similar to pausing since the plugin instance will be
	   reinitialised by activate(). However, deactivate() itself MUST NOT fully
	   reset plugin state. For example, the host may deactivate a plugin, then
	   store its state (using some extension to do so).

	   Hosts MUST NOT call deactivate() unless activate() was previously
	   called. Note that connect_port() may be called before or after
	   deactivate().
	*/
	void (*deactivate)(LV2_Handle instance);

	/**
	   Clean up a plugin instance (counterpart to instantiate()).
	   
	   Once an instance of a plugin has been finished with it must be deleted
	   using this function. The instance handle passed ceases to be valid after
	   this call.
	 
	   If activate() was called for a plugin instance then a corresponding call
	   to deactivate() MUST be made before cleanup() is called. Hosts MUST NOT
	   call cleanup() unless instantiate() was previously called.
	*/
	void (*cleanup)(LV2_Handle instance);

	/**
	   Return additional plugin data defined by some extenion.

	   A typical use of this facility is to return a struct containing function
	   pointers to extend the LV2_Descriptor API.
	 
	   The actual type and meaning of the returned object MUST be specified
	   precisely by the extension if it defines any extra data. This function
	   MUST return NULL for any unsupported URI. If a plugin does not support
	   any extension data, this field may be NULL.
	 
	   @param uri URI of the extension. The plugin MUST return NULL if it does
	   not support the extension, but hosts MUST NOT use this as a discovery
	   mechanism. Hosts should only call this function for extensions known to
	   be supported by the plugin, as described in the plugin's RDF data.
	 
	   The host is never responsible for freeing the returned value.
	 
	   Note this function SHOULD return a struct (likely containing function
	   pointers) and NOT a direct function pointer. Casting void* to a function
	   pointer type is not portable, and returning a struct is much safer
	   since it is extensible (fields can be added without breaking the ABI).
	*/
	const void * (*extension_data)(const char * uri);
} LV2_Descriptor;

/**
   Prototype for plugin accessor function.
 
   The exact mechanism by which plugin libraries are loaded is host and system
   dependent, however all hosts need to know is the URI of the plugin they wish
   to load. Plugins are discovered via RDF data (not by loading plugin
   libraries). Documentation on best practices for plugin discovery can be
   found at <http://lv2plug.in>, however it is expected that hosts use a
   library to provide this functionality.
 
   A plugin library MUST include a function called "lv2_descriptor" with this
   prototype. This function MUST have C-style linkage (if you are using C++
   this is taken care of by the 'extern "C"' clause at the top of this file).
 
   A host will find the plugin's library via RDF data, get the lv2_descriptor()
   function from it, and proceed from there.
 
   Plugins are accessed by index using values from 0 upwards. Out of range
   indices MUST result in this function returning NULL, so the host can
   enumerate plugins by increasing @a index until NULL is returned.

   Note that @a index has no meaning, hosts MUST NOT depend on it remaining
   constant in any way. In other words, @a index is NOT a plugin ID.  In
   particular, hosts MUST NOT refer to plugins by library path and index in
   persistent serialisations (e.g. save files).
*/
const LV2_Descriptor * lv2_descriptor(uint32_t index);

/**
   Type of the lv2_descriptor() function in a plugin library.
*/
typedef const LV2_Descriptor *
(*LV2_Descriptor_Function)(uint32_t index);

/**
  Put this (LV2_SYMBOL_EXPORT) before any functions that are to be loaded
  by the host as a symbol from the dynamic library.
*/
#ifdef WIN32
#define LV2_SYMBOL_EXPORT __declspec(dllexport)
#else
#define LV2_SYMBOL_EXPORT
#endif

#ifdef __cplusplus
}
#endif

#endif /* LV2_H_INCLUDED */
