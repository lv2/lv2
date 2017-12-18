/*
  Copyright 2012-2017 David Robillard <http://drobilla.net>

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

#include <cstdint>
#include <vector>

#include "lv2/lv2plug.in/ns/ext/atom/Atom.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/Forge.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/atom-test-utils.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

using namespace lv2::atom;

static int
test_primitives()
{
	LV2_URID_Map map{NULL, urid_map};
	Forge        forge{&map};

	if (forge.make(int32_t(2)) < forge.make(int32_t(1)) ||
	    forge.make(int64_t(4)) < forge.make(int64_t(3)) ||
	    forge.make(6.0f) < forge.make(4.0f) ||
	    forge.make(8.0) < forge.make(7.0) ||
	    forge.make(true) < forge.make(false)) {
		return test_fail("Primitive comparison failed\n");
	}

	return 0;
}

int
main(void)
{
	if (test_primitives()) {
		return 1;
	}

	LV2_URID_Map map = { NULL, urid_map };

	LV2_URID eg_Object  = urid_map(NULL, "http://example.org/Object");
	LV2_URID eg_one     = urid_map(NULL, "http://example.org/one");
	LV2_URID eg_two     = urid_map(NULL, "http://example.org/two");
	LV2_URID eg_three   = urid_map(NULL, "http://example.org/three");
	LV2_URID eg_four    = urid_map(NULL, "http://example.org/four");
	LV2_URID eg_true    = urid_map(NULL, "http://example.org/true");
	LV2_URID eg_false   = urid_map(NULL, "http://example.org/false");
	LV2_URID eg_path    = urid_map(NULL, "http://example.org/path");
	LV2_URID eg_uri     = urid_map(NULL, "http://example.org/uri");
	LV2_URID eg_urid    = urid_map(NULL, "http://example.org/urid");
	LV2_URID eg_string  = urid_map(NULL, "http://example.org/string");
	LV2_URID eg_literal = urid_map(NULL, "http://example.org/literal");
	LV2_URID eg_tuple   = urid_map(NULL, "http://example.org/tuple");
	LV2_URID eg_vector  = urid_map(NULL, "http://example.org/vector");
	LV2_URID eg_vector2 = urid_map(NULL, "http://example.org/vector2");
	LV2_URID eg_seq     = urid_map(NULL, "http://example.org/seq");

#define BUF_SIZE  1024
#define NUM_PROPS 15

	uint8_t buf[BUF_SIZE];
	Forge   forge(&map, buf, BUF_SIZE);

	Forge::ScopedObject obj(forge, 0, eg_Object);

	// eg_one = (Int)1
	obj.key(eg_one);
	const Int& one = forge.write(1);
	if (one != 1) {
		return test_fail("%d != 1\n", one);
	}

	// eg_two = (Long)2
	obj.key(eg_two);
	const Long& two = forge.write(2L);
	if (two != 2L) {
		return test_fail("%ld != 2\n", two);
	}

	// eg_three = (Float)3.0
	obj.key(eg_three);
	const Float& three = forge.write(3.0f);
	if (three != 3.0f) {
		return test_fail("%f != 3.0f\n", three);
	}

	// eg_four = (Double)4.0
	obj.key(eg_four);
	const Double& four = forge.write(4.0);
	if (four != 4) {
		return test_fail("%ld != 4.0\n", four);
	}

	// eg_true = (Bool)1
	obj.key(eg_true);
	const Bool& t = forge.write(true);
	if (!t) {
		return test_fail("%ld != 1 (true)\n", t);
	}

	// eg_false = (Bool)0
	obj.key(eg_false);
	const Bool& f = forge.write(false);
	if (f) {
		return test_fail("%ld != 0 (false)\n", f);
	}

	// eg_path = (Path)"/foo/bar"
	obj.key(eg_path);
	const String& path = forge.path("/foo/bar");
	if (path != "/foo/bar") {
		return test_fail("%s != \"/foo/bar\"\n", path.c_str());
	}

	// eg_uri = (URI)"http://example.org/value"
	obj.key(eg_uri);
	const String& uri = forge.uri("http://lv2plug.in/");
	if (uri != "http://lv2plug.in/") {
		return test_fail("%s != \"http://lv2plug.in/\"\n", uri.c_str());
	}

	// eg_urid = (URID)"http://example.org/value"
	LV2_URID eg_value = urid_map(NULL, "http://example.org/value");
	obj.key(eg_urid);
	LV2_Atom_URID* urid = &*forge.urid(eg_value);
	if (urid->body != eg_value) {
		return test_fail("%u != %u\n", urid->body, eg_value);
	}

	// eg_string = (String)"hello"
	obj.key(eg_string);
	const String& string = forge.string("hello", strlen("hello"));
	if (string != "hello") {
		return test_fail("%s != \"hello\"\n", string.c_str());
	}

	// eg_literal = (Literal)"hello"@fr
	obj.key(eg_literal);
	const Literal& literal = forge.literal(
		"bonjour", 0, urid_map(NULL, "http://lexvo.org/id/term/fr"));
	if (strcmp(literal.c_str(), "bonjour")) {
		return test_fail("%s != \"bonjour\"\n", literal.c_str());
	}

	// eg_tuple = "foo",true
	obj.key(eg_tuple);
	const Tuple*  tuple = NULL;
	const String* tup0  = NULL;
	const Bool*   tup1  = NULL;
	{
		Forge::ScopedTuple scoped_tuple(forge);
		tup0  = &*forge.string("foo", strlen("foo"));
		tup1  = &*forge.write(true);
		tuple = &*scoped_tuple;
	}
	std::vector<const Atom*> elements;
	for (const Atom& atom : *tuple) {
		elements.push_back(&atom);
	}
	if (elements.size() != 2) {
		return test_fail("Short tuple iteration\n");
	} else if (*elements[0] != *tup0) {
		return test_fail("Incorrect first tuple element\n");
	} else if (*elements[1] != *tup1) {
		return test_fail("Incorrect second tuple element\n");
	}

	// eg_vector = (Vector<Int>)1,2,3,4
	obj.key(eg_vector);
	const int32_t elems[]         = { 1, 2, 3, 4 };
	const Vector<int32_t>& vector = forge.vector(forge.Int, 4, elems);
	if (memcmp(elems, vector.data(), sizeof(elems))) {
		return test_fail("Corrupt vector\n");
	}
	int32_t sum = 0;
	for (const int32_t i : vector) {
		sum += i;
	}
	if (sum != 1 + 2 + 3 + 4) {
		return test_fail("Corrupt vector sum\n");
	}

	// eg_vector2 = (Vector<Int>)1,2,3,4
	obj.key(eg_vector2);
	const Vector<int32_t>* vector2 = NULL;
	{
		Forge::ScopedVector<int32_t> scoped_vector(forge, forge.Int);
		for (unsigned e = 0; e < sizeof(elems) / sizeof(int32_t); ++e) {
			forge.write(elems[e]);
		}
		vector2 = &*scoped_vector;
	}
	if (vector != *vector2) {
		return test_fail("Vector != Vector2\n");
	}

	// eg_seq = (Sequence)1, 2
	obj.key(eg_seq);
	const Sequence* seq = NULL;
	{
		Forge::ScopedSequence scoped_sequence(forge, 0);
		scoped_sequence.frame_time(0);
		forge.write(1);
		scoped_sequence.frame_time(1);
		forge.write(2);
		seq = &*scoped_sequence;
	}

	// Test equality
	if (one == two) {
		return test_fail("1 == 2.0\n");
	} else if (one != one) {
		return test_fail("1 != 1\n");
	}

	unsigned n_events = 0;
	for (const Event& ev : *seq) {
		if (ev.time.frames != n_events) {
			return test_fail("Corrupt event %u has bad time\n", n_events);
		} else if (ev.body.type != forge.Int) {
			return test_fail("Corrupt event %u has bad type\n", n_events);
		} else if (((LV2_Atom_Int*)&ev.body)->body != (int)n_events + 1) {
			return test_fail("Event %u != %d\n", n_events, n_events + 1);
		}
		++n_events;
	}

	unsigned n_props = 0;
	for (const Property::Body& prop : *obj) {
		if (!prop.key) {
			return test_fail("Corrupt property %u has no key\n", n_props);
		} else if (prop.context) {
			return test_fail("Corrupt property %u has context\n", n_props);
		}
		++n_props;
	}

	if (n_props != NUM_PROPS) {
		return test_fail("Corrupt object has %u properties != %u\n",
		                 n_props, NUM_PROPS);
	}

	struct {
		const LV2_Atom* one;
		const LV2_Atom* two;
		const LV2_Atom* three;
		const LV2_Atom* four;
		const LV2_Atom* affirmative;
		const LV2_Atom* negative;
		const LV2_Atom* path;
		const LV2_Atom* uri;
		const LV2_Atom* urid;
		const LV2_Atom* string;
		const LV2_Atom* literal;
		const LV2_Atom* tuple;
		const LV2_Atom* vector;
		const LV2_Atom* vector2;
		const LV2_Atom* seq;
	} matches;

	memset(&matches, 0, sizeof(matches));

	LV2_Atom_Object_Query q[] = {
		{ eg_one,     &matches.one },
		{ eg_two,     &matches.two },
		{ eg_three,   &matches.three },
		{ eg_four,    &matches.four },
		{ eg_true,    &matches.affirmative },
		{ eg_false,   &matches.negative },
		{ eg_path,    &matches.path },
		{ eg_uri,     &matches.uri },
		{ eg_urid,    &matches.urid },
		{ eg_string,  &matches.string },
		{ eg_literal, &matches.literal },
		{ eg_tuple,   &matches.tuple },
		{ eg_vector,  &matches.vector },
		{ eg_vector2, &matches.vector2 },
		{ eg_seq,     &matches.seq },
		LV2_ATOM_OBJECT_QUERY_END
	};

	unsigned n_matches = lv2_atom_object_query(&*obj, q);
	for (int n = 0; n < 2; ++n) {
		if (n_matches != n_props) {
			return test_fail("Query failed, %u matches != %u\n",
			                 n_matches, n_props);
		} else if (one != *matches.one) {
			return test_fail("Bad match one\n");
		} else if (two != *matches.two) {
			return test_fail("Bad match two\n");
		} else if (three != *matches.three) {
			return test_fail("Bad match three\n");
		} else if (four != *matches.four) {
			return test_fail("Bad match four\n");
		} else if (t != *matches.affirmative) {
			return test_fail("Bad match true\n");
		} else if (f != *matches.negative) {
			return test_fail("Bad match false\n");
		} else if (path != *matches.path) {
			return test_fail("Bad match path\n");
		} else if (uri != *matches.uri) {
			return test_fail("Bad match URI\n");
		} else if (string != *matches.string) {
			return test_fail("Bad match string\n");
		} else if (literal != *matches.literal) {
			return test_fail("Bad match literal\n");
		} else if (*tuple != *matches.tuple) {
			return test_fail("Bad match tuple\n");
		} else if (vector != *matches.vector) {
			return test_fail("Bad match vector\n");
		} else if (vector != *matches.vector2) {
			return test_fail("Bad match vector2\n");
		} else if (*seq != *matches.seq) {
			return test_fail("Bad match sequence\n");
		}
		memset(&matches, 0, sizeof(matches));
		n_matches = lv2_atom_object_get((LV2_Atom_Object*)&*obj,
		                                eg_one,     &matches.one,
		                                eg_two,     &matches.two,
		                                eg_three,   &matches.three,
		                                eg_four,    &matches.four,
		                                eg_true,    &matches.affirmative,
		                                eg_false,   &matches.negative,
		                                eg_path,    &matches.path,
		                                eg_uri,     &matches.uri,
		                                eg_urid,    &matches.urid,
		                                eg_string,  &matches.string,
		                                eg_literal, &matches.literal,
		                                eg_tuple,   &matches.tuple,
		                                eg_vector,  &matches.vector,
		                                eg_vector2, &matches.vector2,
		                                eg_seq,     &matches.seq,
		                                0);
	}

	return 0;
}
