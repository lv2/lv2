// Copyright 2012-2018 David Robillard <d@drobilla.net>
// SPDX-License-Identifier: ISC

#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/log/log.h"
#include "lv2/urid/urid.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

static char**   uris   = NULL;
static uint32_t n_uris = 0;

static char*
copy_string(const char* str)
{
  const size_t len = strlen(str);
  char*        dup = (char*)malloc(len + 1);
  memcpy(dup, str, len + 1);
  return dup;
}

static LV2_URID
urid_map(LV2_URID_Map_Handle handle, const char* uri)
{
  for (uint32_t i = 0; i < n_uris; ++i) {
    if (!strcmp(uris[i], uri)) {
      return i + 1;
    }
  }

  uris             = (char**)realloc(uris, ++n_uris * sizeof(char*));
  uris[n_uris - 1] = copy_string(uri);
  return n_uris;
}

static void
free_urid_map(void)
{
  for (uint32_t i = 0; i < n_uris; ++i) {
    free(uris[i]);
  }

  free(uris);
}

LV2_LOG_FUNC(1, 2)
static int
test_fail(const char* fmt, ...)
{
  va_list args;
  va_start(args, fmt);
  fprintf(stderr, "error: ");
  vfprintf(stderr, fmt, args);
  va_end(args);
  return 1;
}
