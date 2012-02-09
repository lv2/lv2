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

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

char** uris   = NULL;
size_t n_uris = 0;

char*
copy_string(const char* str)
{
	const size_t len = strlen(str);
	char*        dup = (char*)malloc(len + 1);
	memcpy(dup, str, len + 1);
	return dup;
}

LV2_URID
urid_map(LV2_URID_Map_Handle handle, const char* uri)
{
	for (size_t i = 0; i < n_uris; ++i) {
		if (!strcmp(uris[i], uri)) {
			return i + 1;
		}
	}

	uris = (char**)realloc(uris, ++n_uris * sizeof(char*));
	uris[n_uris - 1] = copy_string(uri);
	return n_uris;
}

int
test_fail(const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: ");
	vfprintf(stderr, fmt, args);
	va_end(args);
	return 1;
}

int
main()
{
	LV2_URID_Map   map = { NULL, urid_map };
	LV2_Atom_Forge forge;
	lv2_atom_forge_init(&forge, &map);

	LV2_URID eg_Object  = urid_map(NULL, "http://example.org/Object");
	LV2_URID eg_one     = urid_map(NULL, "http://example.org/one");
	LV2_URID eg_two     = urid_map(NULL, "http://example.org/two");
	LV2_URID eg_three   = urid_map(NULL, "http://example.org/three");
	LV2_URID eg_four    = urid_map(NULL, "http://example.org/four");
	LV2_URID eg_true    = urid_map(NULL, "http://example.org/true");
	LV2_URID eg_false   = urid_map(NULL, "http://example.org/false");
	LV2_URID eg_uri     = urid_map(NULL, "http://example.org/uri");
	LV2_URID eg_string  = urid_map(NULL, "http://example.org/string");
	LV2_URID eg_literal = urid_map(NULL, "http://example.org/literal");
	LV2_URID eg_tuple   = urid_map(NULL, "http://example.org/tuple");
	LV2_URID eg_vector  = urid_map(NULL, "http://example.org/vector");
	LV2_URID eg_seq     = urid_map(NULL, "http://example.org/seq");

#define BUF_SIZE  1024
#define NUM_PROPS 12

	uint8_t buf[BUF_SIZE];
	lv2_atom_forge_set_buffer(&forge, buf, BUF_SIZE);

	LV2_Atom* obj = (LV2_Atom*)lv2_atom_forge_resource(
		&forge, NULL, 0, eg_Object);

	// eg_one = (Int32)1
	lv2_atom_forge_property_head(&forge, obj, eg_one, 0);
	LV2_Atom_Int32* one = lv2_atom_forge_int32(&forge, obj, 1);
	if (one->value != 1) {
		return test_fail("%d != 1\n", one->value);
	}

	// eg_two = (Int64)2
	lv2_atom_forge_property_head(&forge, obj, eg_two, 0);
	LV2_Atom_Int64* two = lv2_atom_forge_int64(&forge, obj, 2);
	if (two->value != 2) {
		return test_fail("%ld != 2\n", two->value);
	}

	// eg_three = (Float)3.0
	lv2_atom_forge_property_head(&forge, obj, eg_three, 0);
	LV2_Atom_Float* three = lv2_atom_forge_float(&forge, obj, 3.0f);
	if (three->value != 3) {
		return test_fail("%f != 3\n", three->value);
	}

	// eg_four = (Double)4.0
	lv2_atom_forge_property_head(&forge, obj, eg_four, 0);
	LV2_Atom_Double* four = lv2_atom_forge_double(&forge, obj, 4.0);
	if (four->value != 4) {
		return test_fail("%ld != 4\n", four->value);
	}

	// eg_true = (Bool)1
	lv2_atom_forge_property_head(&forge, obj, eg_true, 0);
	LV2_Atom_Bool* t = lv2_atom_forge_bool(&forge, obj, true);
	if (t->value != 1) {
		return test_fail("%ld != 1 (true)\n", t->value);
	}

	// eg_false = (Bool)0
	lv2_atom_forge_property_head(&forge, obj, eg_false, 0);
	LV2_Atom_Bool* f = lv2_atom_forge_bool(&forge, obj, false);
	if (f->value != 0) {
		return test_fail("%ld != 0 (false)\n", f->value);
	}

	// eg_uri = (URID)"http://example.org/value"
	LV2_URID eg_value = urid_map(NULL, "http://example.org/value");
	lv2_atom_forge_property_head(&forge, obj, eg_uri, 0);
	LV2_Atom_URID* uri = lv2_atom_forge_urid(&forge, obj, eg_value);
	if (uri->id != eg_value) {
		return test_fail("%u != %u\n", uri->id, eg_value);
	}

	// eg_string = (String)"hello"
	lv2_atom_forge_property_head(&forge, obj, eg_string, 0);
	LV2_Atom_String* string = lv2_atom_forge_string(
		&forge, obj, (const uint8_t*)"hello", strlen("hello"));
	uint8_t* body = (uint8_t*)LV2_ATOM_BODY(string);
	if (strcmp((const char*)body, "hello")) {
		return test_fail("%s != \"hello\"\n", (const char*)body);
	}

	// eg_literal = (Literal)"hello"@fr
	lv2_atom_forge_property_head(&forge, obj, eg_literal, 0);
	LV2_Atom_Literal* literal = lv2_atom_forge_literal(
		&forge, obj, (const uint8_t*)"bonjour", strlen("bonjour"),
		0, urid_map(NULL, "http://lexvo.org/id/term/fr"));
	body = (uint8_t*)LV2_ATOM_CONTENTS(LV2_Atom_Literal, literal);
	if (strcmp((const char*)body, "bonjour")) {
		return test_fail("%s != \"bonjour\"\n", (const char*)body);
	}

	// eg_tuple = "foo",true
	lv2_atom_forge_property_head(&forge, obj, eg_tuple, 0);
	LV2_Atom_Tuple*  tuple = lv2_atom_forge_tuple(&forge, obj);
	LV2_Atom_String* tup0  = lv2_atom_forge_string(
		&forge, (LV2_Atom*)tuple, (const uint8_t*)"foo", strlen("foo"));
	LV2_Atom_Bool* tup1 = lv2_atom_forge_bool(&forge, (LV2_Atom*)tuple, true);
	obj->size += ((LV2_Atom*)tuple)->size;
	LV2_Atom_Tuple_Iter i = lv2_tuple_begin(tuple);
	if (lv2_tuple_is_end(tuple, i)) {
		return test_fail("Tuple iterator is empty\n");
	}
	LV2_Atom* tup0i = (LV2_Atom*)lv2_tuple_iter_get(i);
	if (!lv2_atom_equals((LV2_Atom*)tup0, tup0i)) {
		return test_fail("Corrupt tuple element 0\n");
	}
	i = lv2_tuple_iter_next(i);
	if (lv2_tuple_is_end(tuple, i)) {
		return test_fail("Premature end of tuple iterator\n");
	}
	LV2_Atom* tup1i = lv2_tuple_iter_get(i);
	if (!lv2_atom_equals((LV2_Atom*)tup1, tup1i)) {
		return test_fail("Corrupt tuple element 1\n");
	}
	i = lv2_tuple_iter_next(i);
	if (!lv2_tuple_is_end(tuple, i)) {
		return test_fail("Tuple iter is not at end\n");
	}

	// eg_vector = (Vector<Int32>)1,2,3,4
	lv2_atom_forge_property_head(&forge, obj, eg_vector, 0);
	int32_t elems[] = { 1, 2, 3, 4 };
	LV2_Atom_Vector* vector = lv2_atom_forge_vector(
		&forge, obj, 4, forge.Int32, sizeof(int32_t), elems);
	void* vec_body = LV2_ATOM_CONTENTS(LV2_Atom_Vector, vector);
	if (memcmp(elems, vec_body, sizeof(elems))) {
		return test_fail("Corrupt vector\n");
	}

	// eg_seq = (Sequence)1, 2
	lv2_atom_forge_property_head(&forge, obj, eg_seq, 0);
	LV2_Atom_Sequence* seq = lv2_atom_forge_sequence_head(&forge, obj, 0, 0);
	lv2_atom_forge_audio_time(&forge, (LV2_Atom*)seq, 0, 0);
	lv2_atom_forge_int32(&forge, (LV2_Atom*)seq, 1);
	lv2_atom_forge_audio_time(&forge, (LV2_Atom*)seq, 1, 0);
	lv2_atom_forge_int32(&forge, (LV2_Atom*)seq, 2);
	obj->size += seq->atom.size - sizeof(LV2_Atom);

	// Test equality
	LV2_Atom_Int32 itwo = { { forge.Int32, sizeof(int32_t) }, 2 };
	if (lv2_atom_equals((LV2_Atom*)one, (LV2_Atom*)two)) {
		return test_fail("1 == 2.0\n");
	} else if (lv2_atom_equals((LV2_Atom*)one, (LV2_Atom*)&itwo)) {
		return test_fail("1 == 2\n");
	}

	unsigned n_events = 0;
	LV2_SEQUENCE_FOREACH(seq, i) {
		LV2_Atom_Event* ev = lv2_sequence_iter_get(i);
		if (ev->time.audio.frames != n_events
		    || ev->time.audio.subframes != 0) {
			return test_fail("Corrupt event %u has bad time\n", n_events);
		} else if (ev->body.type != forge.Int32) {
			return test_fail("Corrupt event %u has bad type\n", n_events);
		} else if (((LV2_Atom_Int32*)&ev->body)->value != (int)n_events + 1) {
			return test_fail("Event %u != %d\n", n_events, n_events + 1);
		}
		++n_events;
	}
	
	unsigned n_props = 0;
	LV2_OBJECT_FOREACH((LV2_Atom_Object*)obj, i) {
		LV2_Atom_Property_Body* prop = lv2_object_iter_get(i);
		if (!prop->key) {
			return test_fail("Corrupt property %u has no key\n", n_props);
		} else if (prop->context) {
			return test_fail("Corrupt property %u has context\n", n_props);
		}
		++n_props;
	}

	if (n_props != NUM_PROPS) {
		return test_fail("Corrupt object has %u properties != %u\n",
		                 n_props, NUM_PROPS);
	}

	const LV2_Atom* one_match     = NULL;
	const LV2_Atom* two_match     = NULL;
	const LV2_Atom* three_match   = NULL;
	const LV2_Atom* four_match    = NULL;
	const LV2_Atom* true_match    = NULL;
	const LV2_Atom* false_match   = NULL;
	const LV2_Atom* uri_match     = NULL;
	const LV2_Atom* string_match  = NULL;
	const LV2_Atom* literal_match = NULL;
	const LV2_Atom* tuple_match   = NULL;
	const LV2_Atom* vector_match  = NULL;
	const LV2_Atom* seq_match     = NULL;
	LV2_Atom_Object_Query q[] = {
		{ eg_one,     &one_match },
		{ eg_two,     &two_match },
		{ eg_three,   &three_match },
		{ eg_four,    &four_match },
		{ eg_true,    &true_match },
		{ eg_false,   &false_match },
		{ eg_uri,     &uri_match },
		{ eg_string,  &string_match },
		{ eg_literal, &literal_match },
		{ eg_tuple,   &tuple_match },
		{ eg_vector,  &vector_match },
		{ eg_seq,     &seq_match },
		LV2_OBJECT_QUERY_END
	};

	unsigned matches = lv2_object_get((LV2_Atom_Object*)obj, q);
	if (matches != n_props) {
		return test_fail("Query failed, %u matches != %u\n", matches, n_props);
	} else if (!lv2_atom_equals((LV2_Atom*)one, one_match)) {
		return test_fail("Bad match one\n");
	} else if (!lv2_atom_equals((LV2_Atom*)two, two_match)) {
		return test_fail("Bad match two\n");
	} else if (!lv2_atom_equals((LV2_Atom*)three, three_match)) {
		return test_fail("Bad match three\n");
	} else if (!lv2_atom_equals((LV2_Atom*)four, four_match)) {
		return test_fail("Bad match four\n");
	} else if (!lv2_atom_equals((LV2_Atom*)t, true_match)) {
		return test_fail("Bad match true\n");
	} else if (!lv2_atom_equals((LV2_Atom*)f, false_match)) {
		return test_fail("Bad match false\n");
	} else if (!lv2_atom_equals((LV2_Atom*)uri, uri_match)) {
		return test_fail("Bad match URI\n");
	} else if (!lv2_atom_equals((LV2_Atom*)string, string_match)) {
		return test_fail("Bad match string\n");
	} else if (!lv2_atom_equals((LV2_Atom*)literal, literal_match)) {
		return test_fail("Bad match literal\n");
	} else if (!lv2_atom_equals((LV2_Atom*)tuple, tuple_match)) {
		return test_fail("Bad match tuple\n");
	} else if (!lv2_atom_equals((LV2_Atom*)vector, vector_match)) {
		return test_fail("Bad match vector\n");
	} else if (!lv2_atom_equals((LV2_Atom*)seq, seq_match)) {
		return test_fail("Bad match sequence\n");
	}

	printf("All tests passed.\n");
	return 0;
}
