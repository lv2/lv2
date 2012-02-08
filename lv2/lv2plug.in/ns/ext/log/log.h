/*
  Copyright 2012 David Robillard <http://drobilla.net>

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
   @file log.h C header for the LV2 Log extension
   <http://lv2plug.in/ns/ext/log>.
*/

#ifndef LV2_LOG_H
#define LV2_LOG_H

#define LV2_LOG_URI      "http://lv2plug.in/ns/ext/log"
#define LV2_LOG_LOG_URI   LV2_LOG_URI "#log"
#define LV2_LOG_ERROR_URI LV2_LOG_URI "#Error"
#define LV2_LOG_INFO_URI  LV2_LOG_URI "#Info"
#define LV2_LOG_WARN_URI  LV2_LOG_URI "#Warn"

#include <stdarg.h>

#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

#ifdef __cplusplus
extern "C" {
#endif

#ifdef __GNUC__
/** Allow type checking of printf-like functions. */
#    define LV2_LOG_FUNC(fmt, arg1) __attribute__((format(printf, fmt, arg1)))
#else
#    define LV2_LOG_FUNC
#endif

/**
   Opaque data to host data for LV2_Log_Log.
*/
typedef void* LV2_Log_Handle;

/**
   Log (http://lv2plug.in/ns/ext/log#log).
*/
typedef struct _LV2_Log {
	/**
	   Opaque pointer to host data.

	   This MUST be passed to methods in this struct henever they are called.
	   Otherwise, it must not be interpreted in any way.
	*/
	LV2_Log_Handle handle;

	/**
	   Log a message, passing format parameters directly.

	   The API of this function matches that of the standard C printf
	   function, except for the addition of the first two parameters.
	*/
	LV2_LOG_FUNC(3, 4)
	int (*printf)(LV2_Log_Handle handle,
	              LV2_URID       level,
	              const char*    fmt, ...);

	/**
	   Log a message, passing format parameters in a va_list.

	   The API of this function matches that of the standard C vprintf
	   function, except for the addition of the first two parameters.
	*/
	LV2_LOG_FUNC(3, 0)
	int (*vprintf)(LV2_Log_Handle handle,
	               LV2_URID       level,
	               const char*    fmt,
	               va_list        ap);
} LV2_Log_Log;

#ifdef __cplusplus
}  /* extern "C" */
#endif

#endif  /* LV2_LOG_H */
