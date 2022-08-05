/*
  Copyright 2022 David Robillard <d@drobilla.net>

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

#if defined(__clang__)
_Pragma("clang diagnostic push")
_Pragma("clang diagnostic ignored \"-Wold-style-cast\"")
_Pragma("clang diagnostic ignored \"-Wzero-as-null-pointer-constant\"")
#elif defined(__GNUC__)
_Pragma("GCC diagnostic push")
_Pragma("GCC diagnostic ignored \"-Wsuggest-attribute=const\"")
#endif

#include "lv2/atom/atom.h"                       // IWYU pragma: keep
#include "lv2/atom/forge.h"                      // IWYU pragma: keep
#include "lv2/atom/util.h"                       // IWYU pragma: keep
#include "lv2/buf-size/buf-size.h"               // IWYU pragma: keep
#include "lv2/core/attributes.h"                 // IWYU pragma: keep
#include "lv2/core/lv2.h"                        // IWYU pragma: keep
#include "lv2/core/lv2_util.h"                   // IWYU pragma: keep
#include "lv2/data-access/data-access.h"         // IWYU pragma: keep
#include "lv2/dynmanifest/dynmanifest.h"         // IWYU pragma: keep
#include "lv2/event/event-helpers.h"             // IWYU pragma: keep
#include "lv2/event/event.h"                     // IWYU pragma: keep
#include "lv2/instance-access/instance-access.h" // IWYU pragma: keep
#include "lv2/log/log.h"                         // IWYU pragma: keep
#include "lv2/log/logger.h"                      // IWYU pragma: keep
#include "lv2/midi/midi.h"                       // IWYU pragma: keep
#include "lv2/morph/morph.h"                     // IWYU pragma: keep
#include "lv2/options/options.h"                 // IWYU pragma: keep
#include "lv2/parameters/parameters.h"           // IWYU pragma: keep
#include "lv2/patch/patch.h"                     // IWYU pragma: keep
#include "lv2/port-groups/port-groups.h"         // IWYU pragma: keep
#include "lv2/port-props/port-props.h"           // IWYU pragma: keep
#include "lv2/presets/presets.h"                 // IWYU pragma: keep
#include "lv2/resize-port/resize-port.h"         // IWYU pragma: keep
#include "lv2/state/state.h"                     // IWYU pragma: keep
#include "lv2/time/time.h"                       // IWYU pragma: keep
#include "lv2/ui/ui.h"                           // IWYU pragma: keep
#include "lv2/units/units.h"                     // IWYU pragma: keep
#include "lv2/uri-map/uri-map.h"                 // IWYU pragma: keep
#include "lv2/urid/urid.h"                       // IWYU pragma: keep
#include "lv2/worker/worker.h"                   // IWYU pragma: keep

int
main()
{
  return 0;
}

#if defined(__clang__)
_Pragma("clang diagnostic pop")
#elif defined(__GNUC__)
_Pragma("GCC diagnostic pop")
#endif