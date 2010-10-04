/* LV2 Resize Port Extension
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

#ifndef LV2_RESIZE_PORT_H
#define LV2_RESIZE_PORT_H

#include <stdint.h>
#include <stdbool.h>

#define LV2_RESIZE_PORT_URI "http://lv2plug.in/ns/ext/resize-port"

typedef void* LV2_Resize_Port_Feature_Data;

typedef struct {

	LV2_Resize_Port_Feature_Data data;

	/** Resize a port buffer to at least @a size bytes.
	 *
	 * This function MAY return false, in which case the port buffer was
	 * not resized and the port is still connected to the same location.
	 * Plugins MUST gracefully handle this situation.
	 */
	bool (*resize_port)(LV2_Resize_Port_Feature_Data data,
	                    uint32_t                     index,
	                    size_t                       size);

} LV2_Resize_Port_Feature;

#endif /* LV2_RESIZE_PORT_H */

