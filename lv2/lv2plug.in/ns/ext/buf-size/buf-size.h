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

#include <stddef.h>
#include <stdint.h>

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#define LV2_BUF_SIZE_URI    "http://lv2plug.in/ns/ext/buf-size"
#define LV2_BUF_SIZE_PREFIX LV2_BUF_SIZE_URI "#"

#define LV2_BUF_SIZE__access              LV2_BUF_SIZE_PREFIX "access"
#define LV2_BUF_SIZE__boundedBlockLength  LV2_BUF_SIZE_PREFIX "boundedBlockLength"
#define LV2_BUF_SIZE__fixedBlockLength    LV2_BUF_SIZE_PREFIX "fixedBlockLength"
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
	size_t size;

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

	   @param handle The handle field of this struct.
	   @param type The type of buffer.  This is deliberately loosely defined
	   and may be a port type or some other type (e.g. an Atom type).
	   @param subtype Additional type parameter.  This may be needed for some
	   types, otherwise it may be set to zero.
	   @return The buffer size for the given type, in bytes.
	*/
	size_t
	(*get_buf_size)(LV2_Buf_Size_Access_Handle handle,
	                LV2_URID                   type,
	                LV2_URID                   subtype);
} LV2_Buf_Size_Access;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_BUF_SIZE_H */
