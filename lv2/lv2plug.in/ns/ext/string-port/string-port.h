/* lv2_string_port.h - C header file for LV2 string port extension.
 * Draft Revision 3
 * Copyright (C) 2008 Krzysztof Foltman <wdev@foltman.com>
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

/** @file
 * C header for the LV2 String Port extension
 * <http://lv2plug.in/ns/ext/string-port#StringTransfer>.
 */

#ifndef LV2_STRING_PORT_H
#define LV2_STRING_PORT_H

#include <stdint.h>

/** URI for the string port transfer mechanism feature */
#define LV2_STRING_PORT_URI "http://lv2plug.in/ns/ext/string-port#StringTransfer"

/** Flag: port data has been updated; for input ports, this flag is set by
the host. For output ports, this flag is set by the plugin. */
#define LV2_STRING_DATA_CHANGED_FLAG 1

/** structure for string port data */
typedef struct
{
    /** Buffer for UTF-8 encoded zero-terminated string value; host-allocated */
    char *data;

    /** Length in bytes (not characters), not including zero byte */
    size_t len;

    /** Output ports: storage space in bytes; must be >= RDF-specified requirements */
    size_t storage;

    /** Flags defined above */
    uint32_t flags;

    /** Undefined (pad to 8 bytes) */
    uint32_t pad;

} LV2_String_Data;

#endif

