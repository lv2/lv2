/* LV2 OSC Messages Extension
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

#ifndef LV2_OSC_H
#define LV2_OSC_H

#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

/** @file
 * C header for the LV2 OSC extension <http://lv2plug.in/ns/ext/osc>.
 * This extension uses (raw) OSC messages
 * and a buffer format which contains a sequence of timestamped messages.
 * Additional (ie beyond raw OSC) indexing information is stored in the buffer
 * for performance, so that accessors for messages and arguments are very fast:
 * O(1) and realtime safe, unless otherwise noted.
 */


/** Argument (in a message).
 *
 * The name of the element in this union directly corresponds to the OSC
 * type tag character in LV2_Event::types.
 */
typedef union {
	/* Standard OSC types */
	int32_t i; /**< 32 bit signed integer */
	float   f; /**< 32 bit IEEE-754 floating point number ("float") */
	char    s; /**< Standard C, NULL terminated string */
	uint8_t b; /**< Blob (int32 size then size bytes padded to 32 bits) */

	/* "Nonstandard" OSC types (defined in the OSC standard) */
	int64_t h; /* 64 bit signed integer */
	// t       /* OSC-timetag */
	double  d; /* 64 bit IEEE 754 floating point number ("double") */
	// S       /* Symbol, represented as an OSC-string */
	int32_t c; /* Character, represented as a 32-bit integer */
	// r  /* 32 bit RGBA color */
	// m  /* 4 byte MIDI message. Bytes from MSB to LSB are: port id, status byte, data1, data2 */
	// T  /* True. No bytes are allocated in the argument data. */
	// F  /* False. No bytes are allocated in the argument data. */
	// N  /* Nil. No bytes are allocated in the argument data. */
	// I  /* Infinitum. No bytes are allocated in the argument data. */
	// [  /* The beginning of an array. */
	// ]  /* The end of an array. */
} LV2_OSC_Argument;



/** Message.
 *
 * This is an OSC message at heart, but with some additional cache information
 * to allow fast access to parameters.  This is the payload of an LV2_Event,
 * time stamp and size (being generic) are in the containing header.
 */
typedef struct {
	uint32_t data_size;      /**< Total size of data, in bytes */
	uint32_t argument_count; /**< Number of arguments in data */
	uint32_t types_offset;   /**< Offset of types string in data */

	/** Take the address of this member to get a pointer to the remaining data.
	 *
	 * Contents are an argument index:
	 * uint32_t argument_index[argument_count]
	 *
	 * followed by a standard OSC message:
	 * char     path[path_length]     (padded OSC string)
	 * char     types[argument_count] (padded OSC string)
	 * void     data[data_size]
	 */
	char data;

} LV2_OSC_Event;

LV2_OSC_Event* lv2_osc_event_new(const char* path, const char* types, ...);

LV2_OSC_Event* lv2_osc_event_from_raw(uint32_t out_buf_size, void* out_buf,
                                      uint32_t raw_msg_size, void* raw_msg);

static inline uint32_t lv2_osc_get_osc_message_size(const LV2_OSC_Event* msg)
	{ return (msg->argument_count * sizeof(char) + 1) + msg->data_size; }

static inline const void* lv2_osc_get_osc_message(const LV2_OSC_Event* msg)
	{ return (const void*)(&msg->data + (sizeof(uint32_t) * msg->argument_count)); }

static inline const char* lv2_osc_get_path(const LV2_OSC_Event* msg)
	{ return (const char*)(&msg->data + (sizeof(uint32_t) * msg->argument_count)); }

static inline const char* lv2_osc_get_types(const LV2_OSC_Event* msg)
	{ return (const char*)(&msg->data + msg->types_offset); }

static inline LV2_OSC_Argument* lv2_osc_get_argument(const LV2_OSC_Event* msg, uint32_t i)
	{ return (LV2_OSC_Argument*)(&msg->data + ((uint32_t*)&msg->data)[i]); }

/*
int lv2_osc_buffer_append_message(LV2_Event_Buffer* buf, LV2_Event* msg);
int lv2_osc_buffer_append(LV2_Event_Buffer* buf, double time, const char* path, const char* types, ...);
void lv2_osc_buffer_compact(LV2_Event_Buffer* buf);
*/

#ifdef __cplusplus
}
#endif

#endif /* LV2_OSC_H */
