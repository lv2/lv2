/* LV2 OSC Messages Extension
 * Copyright (C) 2007-2009 David Robillard
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */

#include <errno.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include "lv2_osc.h"
#include "lv2_osc_print.h"

/*#ifndef BIG_ENDIAN
  #ifndef LITTLE_ENDIAN
    #warning This code requires BIG_ENDIAN or LITTLE_ENDIAN to be defined
    #warning Assuming little endian.  THIS MAY BREAK HORRIBLY!
  #endif
#endif*/

#define lv2_osc_swap32(x) \
({ \
    uint32_t __x = (x); \
    ((uint32_t)( \
    (((uint32_t)(__x) & (uint32_t)0x000000ffUL) << 24) | \
    (((uint32_t)(__x) & (uint32_t)0x0000ff00UL) <<  8) | \
    (((uint32_t)(__x) & (uint32_t)0x00ff0000UL) >>  8) | \
    (((uint32_t)(__x) & (uint32_t)0xff000000UL) >> 24) )); \
})

#define lv2_osc_swap64(x) \
({ \
    uint64_t __x = (x); \
    ((uint64_t)( \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000000000ffULL) << 56) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000000000ff00ULL) << 40) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000000000ff0000ULL) << 24) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00000000ff000000ULL) <<  8) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x000000ff00000000ULL) >>  8) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x0000ff0000000000ULL) >> 24) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0x00ff000000000000ULL) >> 40) | \
	(uint64_t)(((uint64_t)(__x) & (uint64_t)0xff00000000000000ULL) >> 56) )); \
})


/** Pad a size to a multiple of 32 bits */
inline static uint32_t
lv2_osc_pad_size(uint32_t size)
{
	return size + 3 - ((size-1) % 4);
}


inline static uint32_t
lv2_osc_string_size(const char *s)
{
    return lv2_osc_pad_size((uint32_t)strlen(s) + 1);
}


static inline uint32_t
lv2_osc_blob_size(const void* blob)
{
    return sizeof(uint32_t) + lv2_osc_pad_size(*((uint32_t*)blob));
}


uint32_t
lv2_osc_arg_size(char type, const LV2_OSC_Argument* arg)
{
    switch (type) {
	case 'c':
	case 'i':
	case 'f':
		return 4;

	case 'h':
	case 'd':
		return 8;

	case 's':
		return lv2_osc_string_size(&arg->s);

	/*case 'S':
		return lv2_osc_string_size(&arg->S);*/

	case 'b':
		return lv2_osc_blob_size(&arg->b);

	default:
		fprintf(stderr, "Warning: unknown OSC type '%c'.", type);
		return 0;
    }
}


void
lv2_osc_argument_swap_byte_order(char type, LV2_OSC_Argument* arg)
{
    switch (type) {
	case 'i':
	case 'f':
	case 'b':
	case 'c':
		*(int32_t*)arg = lv2_osc_swap32(*(int32_t*)arg);
		break;

	case 'h':
	case 'd':
		*(int64_t*)arg = lv2_osc_swap64(*(int64_t*)arg);
		break;
	}
}


/** Convert a message from network byte order to host byte order. */
void
lv2_osc_message_swap_byte_order(LV2_OSC_Event* msg)
{
	const char* const types = lv2_osc_get_types(msg);

	for (uint32_t i=0; i < msg->argument_count; ++i)
		lv2_osc_argument_swap_byte_order(types[i], lv2_osc_get_argument(msg, i));
}


/** Not realtime safe, returned value must be free()'d by caller. */
LV2_OSC_Event*
lv2_osc_message_new(const char* path, const char* types, ...)
{
	/* FIXME: path only */

	LV2_OSC_Event* result = malloc(sizeof(LV2_OSC_Event)
			+ 4 + lv2_osc_string_size(path));

	const uint32_t path_size = lv2_osc_string_size(path);
	result->data_size = path_size + 4; // 4 for types
	result->argument_count = 0;
	result->types_offset = lv2_osc_string_size(path) + 1;
	(&result->data)[result->types_offset - 1] = ',';
	(&result->data)[result->types_offset] = '\0';

	memcpy(&result->data, path, strlen(path) + 1);

	return result;
}


/** Create a new LV2_OSC_Event from a raw OSC message.
 *
 * If \a out_buf is NULL, new memory will be allocated.  Otherwise the returned
 * value will be equal to buf, unless there is insufficient space in which
 * case NULL is returned.
 */
LV2_OSC_Event*
lv2_osc_message_from_raw(uint32_t out_buf_size,
                         void*    out_buf,
                         uint32_t raw_msg_size,
                         void*    raw_msg)
{
	const uint32_t message_header_size = (sizeof(uint32_t) * 4);

	const uint32_t path_size  = lv2_osc_string_size((char*)raw_msg);
	const uint32_t types_len  = strlen((char*)(raw_msg + path_size + 1));
	uint32_t       index_size = types_len * sizeof(uint32_t);

	if (out_buf == NULL) {
		out_buf_size = message_header_size + index_size + raw_msg_size;
		out_buf = malloc((size_t)out_buf_size);
	} else if (out_buf && out_buf_size < message_header_size + raw_msg_size) {
		return NULL;
	}

	LV2_OSC_Event* write_loc = (LV2_OSC_Event*)(out_buf);
	write_loc->argument_count = types_len;
	write_loc->data_size = index_size + raw_msg_size;

	// Copy raw message
	memcpy(&write_loc->data + index_size, raw_msg, raw_msg_size);

	write_loc->types_offset = index_size + path_size + 1;
	const char* const types = lv2_osc_get_types(write_loc);

	// Calculate/Write index
	uint32_t args_base_offset = write_loc->types_offset + lv2_osc_string_size(types) - 1;
	uint32_t arg_offset = 0;

	for (uint32_t i=0; i < write_loc->argument_count; ++i) {
		((uint32_t*)&write_loc->data)[i] = args_base_offset + arg_offset;
		const LV2_OSC_Argument* const arg = (LV2_OSC_Argument*)(&write_loc->data + args_base_offset + arg_offset);
		// Special case because size is still big-endian
#ifndef BIG_ENDIAN
		if (types[i] == 'b') // special case because size is still big-endian
			arg_offset += lv2_osc_swap32(*((int32_t*)arg));
		else
#endif
			arg_offset += lv2_osc_arg_size(types[i], arg);
	}

	/*printf("Index:\n");
	for (uint32_t i=0; i < write_loc->argument_count; ++i) {
		printf("%u ", ((uint32_t*)&write_loc->data)[i]);
	}
	printf("\n");

	printf("Data:\n");
	for (uint32_t i=0; i < (write_loc->argument_count * 4) + size; ++i) {
		printf("%3u", i % 10);
	}
	printf("\n");
	for (uint32_t i=0; i < (write_loc->argument_count * 4) + size; ++i) {
		char c = *(((char*)&write_loc->data) + i);
		if (c >= 32 && c <= 126)
			printf("%3c", c);
		else
			printf("%3d", (int)c);
	}
	printf("\n");*/

	// Swap to host byte order if necessary
#ifndef BIG_ENDIAN
	lv2_osc_message_swap_byte_order(write_loc);
#endif

	printf("Created message:\n");
	lv2_osc_message_print(write_loc);

	return write_loc;
}


#if 0
/** Allocate a new LV2OSCBuffer.
 *
 * This function is NOT realtime safe.
 */
LV2_OSCBuffer*
lv2_osc_buffer_new(uint32_t capacity)
{
	LV2OSCBuffer* buf = (LV2OSCBuffer*)malloc((sizeof(uint32_t) * 3) + capacity);
	buf->capacity = capacity;
	buf->size = 0;
	buf->message_count = 0;
	memset(&buf->data, 0, capacity);
	return buf;
}


void
lv2_osc_buffer_clear(LV2OSCBuffer* buf)
{
	buf->size = 0;
	buf->message_count = 0;
}

int
lv2_osc_buffer_append_message(LV2OSCBuffer* buf, LV2_OSC_Event* msg)
{
	const uint32_t msg_size = lv2_message_get_size(msg);

	if (buf->capacity - buf->size - ((buf->message_count + 1) * sizeof(uint32_t)) < msg_size)
		return ENOBUFS;

	char* write_loc = &buf->data + buf->size;

	memcpy(write_loc, msg, msg_size);

	// Index is written backwards, starting at end of data
	uint32_t* index_end = (uint32_t*)(&buf->data + buf->capacity - sizeof(uint32_t));
	*(index_end - buf->message_count) = buf->size;

	++buf->message_count;

	buf->size += msg_size;

	return 0;
}

int
lv2_osc_buffer_append(LV2OSCBuffer* buf, double time, const char* path, const char* types, ...)
{
	// FIXME: crazy unsafe
	LV2_OSC_Event* write_msg = (LV2_OSC_Event*)(&buf->data + buf->size);

	write_msg->time = time;
	write_msg->data_size = 0;
	write_msg->argument_count = 0;
	write_msg->types_offset = strlen(path) + 1;

	memcpy(&write_msg->data, path, write_msg->types_offset);

	/*fprintf(stderr, "Append message:\n");
	lv2_osc_message_print(write_msg);
	fprintf(stderr, "\n");*/

	uint32_t msg_size = lv2_message_get_size(write_msg);
	buf->size += msg_size;
	buf->message_count++;

	return 0;
}
#endif

