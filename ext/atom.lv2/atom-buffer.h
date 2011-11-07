/*
  Copyright 2008-2011 David Robillard <http://drobilla.net>

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

/**
   @file atom-event-buffer.h Helper functions for atom:EventBuffer.

   Note that these functions are all static inline which basically means:
   do not take the address of these functions.
*/

#ifndef LV2_ATOM_EVENT_BUFFER_H
#define LV2_ATOM_EVENT_BUFFER_H

#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdlib.h>
#include <assert.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"

/**
   Pad a size to 64 bits.
*/
static inline uint32_t
lv2_atom_pad_size(uint32_t size)
{
	return (size + 7) & (~7);
}

/**
   Initialize an existing atom buffer.
   All fields of @c buf are reset, except capacity which is unmodified.
*/
static inline void
lv2_atom_buffer_reset(LV2_Atom_Buffer* buf)
{
	buf->event_count = 0;
	buf->size        = 0;
}

/**
   Allocate a new, empty atom buffer.
*/
static inline LV2_Atom_Buffer*
lv2_atom_buffer_new(uint32_t capacity)
{
	const uint32_t   size = sizeof(LV2_Atom_Buffer) + capacity;
	LV2_Atom_Buffer* buf  = (LV2_Atom_Buffer*)malloc(size);
	if (buf) {
		buf->data     = (uint8_t*)(buf + 1);
		buf->capacity = capacity;
		lv2_atom_buffer_reset(buf);
	}
	return buf;
}

/**
   Free an atom buffer allocated with lv2_atom_buffer_new().
*/
static inline void
lv2_atom_buffer_free(LV2_Atom_Buffer* buf)
{
	free(buf);
}

/**
   An iterator over an LV2_Atom_Buffer.
*/
typedef struct {
	LV2_Atom_Buffer* buf;
	uint32_t         offset;
} LV2_Atom_Buffer_Iterator;

/**
   Return an iterator to the beginning of @c buf.
*/
static inline LV2_Atom_Buffer_Iterator
lv2_atom_buffer_begin(LV2_Atom_Buffer* buf)
{
	const LV2_Atom_Buffer_Iterator i = { buf, 0 };
	return i;
}

/**
   Return true iff @c i points to a valid atom.
*/
static inline bool
lv2_atom_buffer_is_valid(LV2_Atom_Buffer_Iterator i)
{
	return i.offset < i.buf->size;
}

/**
   Return the iterator to the next element after @c i.
   @param i A valid iterator to an atom in a buffer.
*/
static inline LV2_Atom_Buffer_Iterator
lv2_atom_buffer_next(LV2_Atom_Buffer_Iterator i)
{
	assert(lv2_atom_buffer_is_valid(i));
	const LV2_Atom_Event* const ev = (LV2_Atom_Event*)(
		i.buf->data + i.offset);
	i.offset += lv2_atom_pad_size(sizeof(LV2_Atom_Event) + ev->body.size);
	return i;
}

/**
   Return a pointer to the atom currently pointed to by @c i.
*/
static inline LV2_Atom_Event*
lv2_atom_buffer_get(LV2_Atom_Buffer_Iterator i)
{
	assert(lv2_event_is_valid(i));
	return (LV2_Atom_Event*)(i.buf->data + i.offset);
}

/**
   Write an atom to a buffer.

   The atom will be written at the location pointed to by @c i, which will be
   incremented to point to the location where the next atom should be written
   (which is likely now garbage).  Thus, this function can be called repeatedly
   with a single @c i to write a sequence of atoms to the buffer.

   @return True if atom was written, otherwise false (buffer is full).
*/
static inline bool
lv2_atom_buffer_write(LV2_Atom_Buffer_Iterator* i,
                      uint32_t                  frames,
                      uint32_t                  subframes,
                      uint32_t                  type,
                      uint32_t                  size,
                      const uint8_t*            data)
{
	const uint32_t free_space = i->buf->capacity - i->buf->size;
	if (free_space < sizeof(LV2_Atom_Event) + size) {
		return false;
	}

	LV2_Atom_Event* const ev = (LV2_Atom_Event*)(i->buf->data + i->offset);

	ev->frames    = frames;
	ev->subframes = subframes;
	ev->body.type = type;
	ev->body.size = size;
	memcpy((uint8_t*)ev + sizeof(LV2_Atom_Event), data, size);
	++i->buf->event_count;

	size          = lv2_atom_pad_size(sizeof(LV2_Atom_Event) + size);
	i->buf->size += size;
	i->offset    += size;

	return true;
}

#endif /* LV2_ATOM_EVENT_BUFFER_H */
