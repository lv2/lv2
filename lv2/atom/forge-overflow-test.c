/*
  Copyright 2019 David Robillard <http://drobilla.net>

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

#include "lv2/atom/atom-test-utils.c"
#include "lv2/atom/atom.h"
#include "lv2/atom/forge.h"
#include "lv2/atom/util.h"
#include "lv2/urid/urid.h"

#include <stdint.h>
#include <stdlib.h>
#include <string.h>

static int
test_string_overflow(void)
{
#define MAX_CHARS 15

	static const size_t capacity  = sizeof(LV2_Atom_String) + MAX_CHARS + 1;
	static const char*  str       = "ABCDEFGHIJKLMNOPQRSTUVWXYZ";

	uint8_t*       buf = (uint8_t*)malloc(capacity);
	LV2_URID_Map   map = { NULL, urid_map };
	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, &map);

	// Check that writing increasingly long strings fails at the right point
	for (size_t count = 0; count < MAX_CHARS; ++count) {
		lv2_atom_forge_set_buffer(&forge, buf, capacity);

		const LV2_Atom_Forge_Ref ref =
		        lv2_atom_forge_string(&forge, str, count);
		if (!ref) {
			return test_fail("Failed to write %zu byte string\n", count);
		}
	}

	// Failure writing to an exactly full forge
	LV2_Atom_Forge_Ref ref = 0;
	if ((ref = lv2_atom_forge_string(&forge, str, MAX_CHARS + 1))) {
		return test_fail("Successfully wrote past end of buffer\n");
	}

	// Failure writing body after successfully writing header
	lv2_atom_forge_set_buffer(&forge, buf, sizeof(LV2_Atom) + 1);
	if ((ref = lv2_atom_forge_string(&forge, "AB", 2))) {
		return test_fail("Successfully wrote atom header past end\n");
	}

	free(buf);
	return 0;
}

static int
test_literal_overflow(void)
{
	static const size_t capacity = sizeof(LV2_Atom_Literal) + 2;

	uint8_t*           buf = (uint8_t*)malloc(capacity);
	LV2_URID_Map       map = { NULL, urid_map };
	LV2_Atom_Forge_Ref ref = 0;
	LV2_Atom_Forge     forge;
	lv2_atom_forge_init(&forge, &map);

	// Failure in atom header
	lv2_atom_forge_set_buffer(&forge, buf, 1);
	if ((ref = lv2_atom_forge_literal(&forge, "A", 1, 0, 0))) {
		return test_fail("Successfully wrote atom header past end\n");
	}

	// Failure in literal header
	lv2_atom_forge_set_buffer(&forge, buf, sizeof(LV2_Atom) + 1);
	if ((ref = lv2_atom_forge_literal(&forge, "A", 1, 0, 0))) {
		return test_fail("Successfully wrote literal header past end\n");
	}

	// Success (only room for one character + null terminator)
	lv2_atom_forge_set_buffer(&forge, buf, capacity);
	if (!(ref = lv2_atom_forge_literal(&forge, "A", 1, 0, 0))) {
		return test_fail("Failed to write small enough literal\n");
	}

	// Failure in body
	lv2_atom_forge_set_buffer(&forge, buf, capacity);
	if ((ref = lv2_atom_forge_literal(&forge, "AB", 2, 0, 0))) {
		return test_fail("Successfully wrote literal body past end\n");
	}

	free(buf);
	return 0;
}

int
main(void)
{
	return test_string_overflow() || test_literal_overflow();
}
