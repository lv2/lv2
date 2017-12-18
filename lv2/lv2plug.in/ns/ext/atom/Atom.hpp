/*
  Copyright 2008-2017 David Robillard <http://drobilla.net>

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
   @defgroup atompp Atom C++ bindings.

   See <http://lv2plug.in/ns/ext/atom> for details.

   @{
*/

#ifndef LV2_ATOM_HPP
#define LV2_ATOM_HPP

#include <cstring>
#include <string>

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"

namespace lv2 {
namespace atom {

struct Atom : LV2_Atom {};

template<typename AtomType>
struct AtomWithBody : AtomType
{
	using Body = decltype(AtomType::body);

	AtomWithBody(AtomType atom) : AtomType(atom) {}

	operator const LV2_Atom&() const { return this->atom; }
	explicit operator Body()   const { return AtomType::body; }

	bool operator==(const Body& body) const { return AtomType::body == body; }
	bool operator!=(const Body& body) const { return AtomType::body != body; }

	bool operator!() const { return !AtomType::body; }
};

template<typename AtomType>
struct Primitive : AtomWithBody<AtomType>
{
	Primitive(AtomType atom) : AtomWithBody<AtomType>(atom) {}

	bool operator<(const Primitive& rhs) const {
		return AtomType::body < rhs.body;
	}
};

using Int    = Primitive<LV2_Atom_Int>;
using Long   = Primitive<LV2_Atom_Long>;
using Float  = Primitive<LV2_Atom_Float>;
using Double = Primitive<LV2_Atom_Double>;
using URID   = Primitive<LV2_Atom_URID>;

struct Bool : Primitive<LV2_Atom_Bool>
{
	Bool(LV2_Atom_Bool atom) : Primitive<LV2_Atom_Bool>(atom) {}

	operator bool()  const { return LV2_Atom_Bool::body; }
	bool operator!() const { return !LV2_Atom_Bool::body; }
};

struct String : LV2_Atom_String
{
	operator const LV2_Atom&() const { return this->atom; }
	operator const Atom&()     const { return (const Atom&)*this; }

	const char* c_str() const { return (const char*)(this + 1); }
	char*       c_str()       { return (char*)(this + 1); }

	bool operator==(const char* s)        const { return !strcmp(c_str(), s); }
	bool operator!=(const char* s)        const { return  strcmp(c_str(), s); }
	bool operator==(const std::string& s) const { return s == c_str(); }
	bool operator!=(const std::string& s) const { return s != c_str(); }
};

struct Literal : AtomWithBody<LV2_Atom_Literal>
{
	const char* c_str() const { return (const char*)(this + 1); }
	char*       c_str()       { return (char*)(this + 1); }
};

template<typename CType, typename CppType, CType* Next(const CType*)>
struct Iterator
{
	using value_type = CppType;

	explicit Iterator(const CType* p)
		: ptr(static_cast<const value_type*>(p))
	{}

	Iterator& operator++() {
		ptr = static_cast<value_type*>(Next(ptr));
		return *this;
	}

	const value_type& operator*()  const { return *ptr; }
	const value_type* operator->() const { return ptr; }

	bool operator==(const Iterator& i) const { return ptr == i.ptr; }
	bool operator!=(const Iterator& i) const { return ptr != i.ptr; }

private:
	const value_type* ptr;
};

struct Tuple : LV2_Atom_Tuple
{
	operator const LV2_Atom&() const { return this->atom; }

	using const_iterator = Iterator<LV2_Atom, Atom, lv2_atom_tuple_next>;

	const_iterator begin() const {
		return const_iterator(lv2_atom_tuple_begin(this));
	}

	const_iterator end() const {
		return const_iterator(
			(const LV2_Atom*)(
				(const char*)(this + 1) + lv2_atom_pad_size(atom.size)));
	}
};

template<typename T>
struct Vector : AtomWithBody<LV2_Atom_Vector>
{
	operator const LV2_Atom&() const { return this->atom; }

	const T* data() const { return (const T*)(&body + 1); }
	T*       data()       { return (T*)(&body + 1); }

	const T* begin() const { return data(); }
	const T* end()   const { return (const T*)(const char*)&body + atom.size; }
	T*       begin()       { return data(); }
	T*       end()         { return (T*)(char*)&body + atom.size; }
};

struct Property : AtomWithBody<LV2_Atom_Property> {};

struct Object : AtomWithBody<LV2_Atom_Object>
{
	using const_iterator = Iterator<LV2_Atom_Property_Body,
	                                Property::Body,
	                                lv2_atom_object_next>;

	const_iterator begin() const {
		return const_iterator(lv2_atom_object_begin(&body));
	}

	const_iterator end() const {
		return const_iterator(
			(const LV2_Atom_Property_Body*)(
				(const char*)&body + lv2_atom_pad_size(atom.size)));
	}
};

struct Event : LV2_Atom_Event {};

struct Sequence : AtomWithBody<LV2_Atom_Sequence>
{
	using const_iterator = Iterator<LV2_Atom_Event, Event, lv2_atom_sequence_next>;

	const_iterator begin() const {
		return const_iterator(lv2_atom_sequence_begin(&body));
	}

	const_iterator end() const {
		return const_iterator(lv2_atom_sequence_end(&body, atom.size));
	}
};

inline bool
operator==(const LV2_Atom& lhs, const LV2_Atom& rhs)
{
	return lv2_atom_equals(&lhs, &rhs);
}

inline bool
operator!=(const LV2_Atom& lhs, const LV2_Atom& rhs)
{
	return !lv2_atom_equals(&lhs, &rhs);
}

}  // namespace atom
}  // namespace lv2

/**
   @}
*/

#endif /* LV2_ATOM_HPP */
