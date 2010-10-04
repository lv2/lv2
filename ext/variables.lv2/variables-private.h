/* LV2 Plugin Variables Extension (Private Implementation)
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

#include "lv2_variables.h"

/** An LV2 Plugin Variable (Private) */
struct _LV2Var_Variable {
	char* key;   /**< Lookup key of variable, full URI */
	char* type;  /**< Type of value, full URI, may be NULL */
	char* value; /**< Variable value (string literal or URI) */
};


static const char*
lv2var_variable_key(const LV2Var_Variable var)
{
	return var->key;
}


static const char*
lv2var_variable_type(const LV2Var_Variable var)
{
	return var->type;
}


static const char*
lv2var_variable_value(const LV2Var_Variable var)
{
	return var->value;
}

