/* LV2 Plugin Variables Extension
 * Copyright (C) 2007-2009 David Robillard <http://drobilla.net>
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU General Public License as published by the Free
 * Software Foundation; either version 2 of the License, or (at your option)
 * any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU General Public License
 * for more details.
 *
 * You should have received a copy of the GNU General Public License along
 * with this program; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin St, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#ifndef LV2_VARIABLES_H
#define LV2_VARIABLES_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif


/** @file
 * This is an LV2 extension allowing plugin instances to have a set of
 * dynamic named/typed variables (ie key/value metadata).
 *
 * Plugin variable values are always in string form (if numeric
 * variable values are requires in realtime run callbacks, it is assumed
 * the plugin will cache the converted value).
 *
 * Keys are strings, either free-form locally unique strings, or URIs.
 * Types are URIs (corresponding to some type defined somewhere, e.g. in an
 * XML namespace or RDF ontology).
 *
 * The goal is to provide a powerful key/value system for plugins, which is
 * useful for setting run-time values (analogous to DSSI's "configure" calls)
 * which is typed and ideal for serializing to RDF (which means variable
 * values can be stored in the same file as the plugin's definition) or
 * network transmission/control.
 */


/** An LV2 Plugin Variable */
typedef struct _LV2Var_Variable* LV2Var_Variable;

static const char* lv2var_variable_key(const LV2Var_Variable var);
static const char* lv2var_variable_type(const LV2Var_Variable var);
static const char* lv2var_variable_value(const LV2Var_Variable var);



/** Plugin extension data for plugin variables.
 *
 * The extension_data function on a plugin (which supports this extension)
 * will return a pointer to a struct of this type, when called with the URI
 * http://drobilla.net/ns/lv2/variables
 */
typedef struct _LV2Var_Descriptor {

	/** Get the value of a plugin variable (O(log(n), non-blocking).
	 *
	 * @param key_uri  Key of variable to look up
	 * @param type_uri Output, set to (shared) type of value (full URI, may be NULL)
	 * @param value    Output, set to (shared) value of variable
	 *
	 * @return 0 if variable was found and type, value have been set accordingly,
	 * otherwise non-zero.
	 */
	int32_t (*get_value)(const char*  key_uri,
	                     const char** type_uri,
	                     const char** value);


	/** Set a plugin variable to a typed literal value (O(log(n), allocates memory).
	 *
	 * Note that this function is NOT realtime safe.
	 *
	 * String parameters are copied.  The key is the sole unique identifier
	 * for variables; if a variable exists with the given key, it will be
	 * overwritten with the new type and value.
	 *
	 * To set a variable's value to a URI, use rdfs:Resource
	 * (http://www.w3.org/2000/01/rdf-schema#Resource) for the value type.
	 *
	 * @param key_uri  Key of variable to set (MUST be a full URI)
	 * @param type_uri Type of value (MUST be a full URI, may be NULL)
	 * @param value    Value of variable to set
	 */
	void (*set_value)(const char* key_uri,
	                  const char* type_uri,
	                  const char* value);


	/** Unset (erase) a variable (O(log(n), deallocates memory).
	 *
	 * Note that this function is NOT realtime safe.
	 *
	 * @param key Key of variable to erase
	 */
	void (*unset)(const char* key_uri);


	/** Clear (erase) all set variables (O(1), deallocates memory).
	 *
	 * Note that this function is NOT realtime safe.
	 */
	void (*clear)();


	/** Get all variables of a plugin (O(log(n), allocates memory).
	 *
	 * @param variables Output, set to a shared array of all set variables
	 *
	 * @return The number of variables found
	 */
	uint32_t (*get_all_variables)(const LV2Var_Variable** variables);


	/** Get the value of a plugin variable (O(log(n), non-blocking).
	 *
	 * @param key_uri  Key of variable to look up
	 * @param variable Output, set to point at (shared) variable
	 *
	 * @return 0 if variable was found and variable has been set accordingly,
	 * otherwise non-zero.
	 */
	int32_t (*get_variable)(const char*             key_uri,
	                        const LV2Var_Variable** variable);

} LV2Var_Descriptor;


#ifdef __cplusplus
}
#endif

#endif /* LV2_VARIABLES_H */

