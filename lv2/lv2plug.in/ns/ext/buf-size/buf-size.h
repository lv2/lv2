/*
  Copyright 2007-2012 David Robillard <http://drobilla.net>

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

#ifndef LV2_BUF_SIZE_H
#define LV2_BUF_SIZE_H

#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define LV2_BUF_SIZE_URI    "http://lv2plug.in/ns/ext/buf-size"
#define LV2_BUF_SIZE_PREFIX LV2_BUF_SIZE_URI "#"

#define LV2_BUF_SIZE__access              LV2_BUF_SIZE_PREFIX "access"
#define LV2_BUF_SIZE__boundedBlockLength  LV2_BUF_SIZE_PREFIX "boundedBlockLength"
#define LV2_BUF_SIZE__fixedBlockLength    LV2_BUF_SIZE_PREFIX "fixedBlockLength"
#define LV2_BUF_SIZE__maxBlockLength      LV2_BUF_SIZE_PREFIX "maxBlockLength"
#define LV2_BUF_SIZE__minBlockLength      LV2_BUF_SIZE_PREFIX "minBlockLength"
#define LV2_BUF_SIZE__powerOf2BlockLength LV2_BUF_SIZE_PREFIX "powerOf2BlockLength"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
	LV2_BUF_SIZE_SUCCESS      = 0,  /**< Completed successfully. */
	LV2_BUF_SIZE_ERR_UNKNOWN  = 1,  /**< Unknown error. */
	LV2_BUF_SIZE_ERR_BAD_TYPE = 2   /**< Failed due to unsupported type. */
} LV2_Buf_Size_Status;

typedef void* LV2_Buf_Size_Access_Handle;

/**
   The data for feature LV2_BUF_SIZE__access.
*/
typedef struct {
	/**
	   Opaque host data.
	*/
	LV2_Buf_Size_Access_Handle handle;

	/**
	   The size of this struct.

	   The host MUST set this to sizeof(LV2_Buf_Size_Feature).
	*/
	uint32_t size;

	/**
	   Get properties of the sample_count parameter of LV2_Descriptor::run().

	   @param handle The handle field of this struct.
	   @param min Set to the minimum block length.
	   @param max Set to the maximum block length, or 0 for unlimited.
	   @param multiple_of Set to a number the block length will always be a
	   multiple of, possibly 1 for arbitrary block lengths.
	   @param power_of Set to a number the block length will always be a power
	   of, or 0 if no such restriction applies.
	   @return 0 for success, otherwise an error code.
	*/
	LV2_Buf_Size_Status
	(*get_sample_count)(LV2_Buf_Size_Access_Handle handle,
	                    uint32_t*                  min,
	                    uint32_t*                  max,
	                    uint32_t*                  multiple_of,
	                    uint32_t*                  power_of);

	/**
	   Get the size for buffers of a given type.

	   This function is useful for plugins that need to allocate auxiliary
	   buffers for data types other than audio.  A typical use case is to first
	   get the maximum block length with get_sample_count(), then determine the
	   space required for a Sequence by calling this function with type @ref
	   LV2_ATOM__Sequence.

	   @param handle The handle field of this struct.
	   @param buf_size Set to the requested buffer size, or 0 if unknown.
	   @param type The type of buffer.
	   @param sample_count The duration of time the buffer must represent.
	   @return The space required for the buffer, or 0 if unknown.
	*/
	LV2_Buf_Size_Status
	(*get_buf_size)(LV2_Buf_Size_Access_Handle handle,
	                uint32_t*                  buf_size,
	                LV2_URID                   type,
	                uint32_t                   sample_count);
} LV2_Buf_Size_Access;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_BUF_SIZE_H */
