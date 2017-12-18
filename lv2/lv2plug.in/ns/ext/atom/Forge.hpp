/*
  Copyright 2017 David Robillard <http://drobilla.net>

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
   @file Forge.hpp A C++ wrapper for LV2_Atom_Forge.

   This file provides a wrapper for the atom forge that makes it simpler and
   safer to write atoms in C++.  It uses scoped handled for nested frames to
   enforce correct atom structure in a safer and more readable way than the C
   API.

   A "make" interface is also provided for making primitive atoms that are
   value types without requiring an output buffer.
*/

/**
   @defgroup forgepp C++ Forge
   @ingroup atompp
   @{
*/

#ifndef LV2_ATOM_FORGE_HPP
#define LV2_ATOM_FORGE_HPP

#include <string>

#include "lv2/lv2plug.in/ns/ext/atom/Atom.hpp"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"

namespace lv2 {
namespace atom {

namespace detail {

template<typename Sink>
static LV2_Atom_Forge_Ref
write(void* const sink, const void* const buf, const uint32_t size)
{
	return ((Sink*)sink)->write(buf, size);
}

template<typename Sink>
static LV2_Atom*
deref(void* const sink, const LV2_Atom_Forge_Ref ref)
{
	return ((Sink*)sink)->deref(ref);
}

};

/** A "forge" for creating atoms by appending to a buffer. */
class Forge : public LV2_Atom_Forge
{
public:
	using Frame = LV2_Atom_Forge_Frame;

	/** Type safe wrapper for LV2_Atom_Forge_Ref. */
	template<typename T>
	struct Ref
	{
		Ref(Forge& forge, const LV2_Atom_Forge_Ref ref)
			: forge(forge)
			, ref(ref)
		{}

		const T& operator*() const {
			return *(const T*)lv2_atom_forge_deref(&forge, ref);
		}

		const T* operator->() const {
			return (const T*)lv2_atom_forge_deref(&forge, ref);
		}

		T& operator*()  { return *(T*)lv2_atom_forge_deref(&forge, ref); }
		T* operator->() { return  (T*)lv2_atom_forge_deref(&forge, ref); }

		operator const T&() const { return **this; }
		operator T&()             { return **this; }

		Forge&             forge;
		LV2_Atom_Forge_Ref ref;
	};

	/**
	   @name Initialisation
	   @{
	*/

	/** Construct a forge with uninitialized output.
	 *
	 * Either set_buffer() or set_sink() must be called before writing.
	 */
	Forge(LV2_URID_Map* const map)
	{
		lv2_atom_forge_init(this, map);
	}

	/** Construct a forge for writing to a buffer. */
	Forge(LV2_URID_Map* const map, uint8_t* const buf, const size_t size)
	{
		lv2_atom_forge_init(this, map);
		set_buffer(buf, size);
	}

	/** Construct a forge for writing to a sink function. */
	template<typename Handle>
	Forge(LV2_URID_Map* const   map,
	      LV2_Atom_Forge_Ref  (*sink)(Handle, const void*, uint32_t),
	      LV2_Atom*           (*deref)(Handle, LV2_Atom_Forge_Ref),
	      Handle                handle)
	{
		lv2_atom_forge_init(this, map);
		set_sink(sink, deref, handle);
	}

	/** Construct a forge for writing to a sink object.
	 *
	 * The sink must have write(const void*, uint32_t) and
	 * deref(LV2_Atom_Forge_Ref) methods.
	 */
	template<typename Sink>
	Forge(LV2_URID_Map* const map, Sink& sink)
	{
		lv2_atom_forge_init(this, map);
		set_sink(sink);
	}

	/** Set the output buffer to write to. */
	void set_buffer(uint8_t* const buf, const size_t size)
	{
		lv2_atom_forge_set_buffer(this, buf, size);
	}

	/** Set a sink function to write to. */
	template<typename Handle>
	void set_sink(LV2_Atom_Forge_Ref (*sink)(Handle, const void*, uint32_t),
	              LV2_Atom*          (*deref)(Handle, LV2_Atom_Forge_Ref),
	              Handle               handle)
	{
		lv2_atom_forge_set_sink(this, sink, deref, handle);
	}

	/** Set a sink object to write to. */
	template<typename Sink>
	void set_sink(Sink& sink)
	{
		lv2_atom_forge_set_sink(
			this, detail::write<Sink>, detail::deref<Sink>, &sink);
	}

	/**
	   @}
	   @name Primitive Value Construction
	   @{
	*/

	atom::Int    make(const int32_t v) { return LV2_Atom_Int{sizeof(v), Int, v}; }
	atom::Long   make(const int64_t v) { return LV2_Atom_Long{sizeof(v), Long, v}; }
	atom::Float  make(const float v)   { return LV2_Atom_Float{sizeof(v), Float, v}; }
	atom::Double make(const double v)  { return LV2_Atom_Double{sizeof(v), Double, v}; }
	atom::Bool   make(const bool v)    { return LV2_Atom_Bool{sizeof(int32_t), Bool, v}; }

	/**
	   @}
	   @name Low Level Output
	   @{
	*/

	/** Write raw output. */
	LV2_Atom_Forge_Ref raw(const void* const data, const uint32_t size) {
		return lv2_atom_forge_raw(this, data, size);
	}

	/** Pad output accordingly so next write is 64-bit aligned. */
	void pad(const uint32_t written) {
		lv2_atom_forge_pad(this, written);
	}

	/** Write raw output, padding to 64-bits as necessary. */
	LV2_Atom_Forge_Ref write(const void* const data, const uint32_t size)
	{
		return lv2_atom_forge_write(this, data, size);
	}

	/** Write a null-terminated string body. */
	LV2_Atom_Forge_Ref string_body(const char* const str, const uint32_t len)
	{
		return lv2_atom_forge_string_body(this, str, len);
	}

	/**
	   @}
	   @name Atoms
	   @{
	*/

	/** Write an Atom header. */
	Ref<atom::Atom> atom(uint32_t size, uint32_t type)
	{
		return ref<atom::Atom>(lv2_atom_forge_atom(this, size, type));
	}

	/** Write an Atom. */
	Ref<atom::Atom> atom(uint32_t size, uint32_t type, const void* body)
	{
		Ref<atom::Atom> ref(atom(size, type));
		lv2_atom_forge_write(this, body, size);
		return ref;
	}

	/** Write a primitive (fixed-size) atom. */
	Ref<atom::Atom> primitive(const LV2_Atom* atom)
	{
		return ref<atom::Atom>(lv2_atom_forge_primitive(this, atom));
	}

	/**
	   @}
	   @name Primitives
	   @{
	*/

	/** Write an atom:Int. */
	Ref<atom::Int> write(const int32_t val)
	{
		return ref<atom::Int>(lv2_atom_forge_int(this, val));
	}

	/** Write an atom:Long. */
	Ref<atom::Long> write(const int64_t val)
	{
		return ref<atom::Long>(lv2_atom_forge_long(this, val));
	}

	/** Write an atom:Float. */
	Ref<atom::Float> write(const float val)
	{
		return ref<atom::Float>(lv2_atom_forge_float(this, val));
	}

	/** Write an atom:Double. */
	Ref<atom::Double> write(const double val)
	{
		return ref<atom::Double>(lv2_atom_forge_double(this, val));
	}

	/** Write an atom:Bool. */
	Ref<atom::Bool> write(const bool val)
	{
		return ref<atom::Bool>(lv2_atom_forge_bool(this, val));
	}

	/** Write an atom:URID. */
	Ref<atom::URID> urid(const LV2_URID id)
	{
		return ref<atom::URID>(lv2_atom_forge_urid(this, id));
	}

	/**
	   @}
	   @name Strings
	   @{
	*/

	/** Write an atom:String.  Note that `str` need not be NULL terminated. */
	Ref<atom::String> string(const char* const str, const uint32_t len)
	{
		return ref<atom::String>(lv2_atom_forge_string(this, str, len));
	}

	/** Write an atom:String. */
	Ref<atom::String> string(const char* const str)
	{
		return string(str, strlen(str));
	}

	/** Write an atom:String. */
	Ref<atom::String> write(const std::string& str)
	{
		return ref<atom::String>(
			lv2_atom_forge_string(this, str.c_str(), str.length()));
	}

	/** Write a URI string.  Note that `str` need not be NULL terminated. */
	Ref<atom::String> uri(const char* const uri, const uint32_t len)
	{
		return ref<atom::String>(lv2_atom_forge_uri(this, uri, len));
	}

	/** Write a URI string. */
	Ref<atom::String> uri(const char* const str)
	{
		return uri(str, strlen(str));
	}

	/** Write a URI string. */
	Ref<atom::String> uri(const std::string& str)
	{
		return ref<atom::String>(
			lv2_atom_forge_uri(this, str.c_str(), str.length()));
	}

	/** Write an atom:Path.  Note that `str` need not be NULL terminated. */
	Ref<atom::String> path(const char* const str, const uint32_t len)
	{
		return ref<atom::String>(lv2_atom_forge_path(this, str, len));
	}

	/** Write an atom:Path. */
	Ref<atom::String> path(const char* const str)
	{
		return path(str, strlen(str));
	}

	/** Write an atom:Path. */
	Ref<atom::String> path(const std::string& path)
	{
		return ref<atom::String>(
			lv2_atom_forge_path(this, path.c_str(), path.length()));
	}

	/** Write an atom:Literal.  Note that `str` need not be NULL terminated. */
	Ref<atom::Literal> literal(const char* const str,
	                           const uint32_t    len,
	                           const uint32_t    datatype,
	                           const uint32_t    lang)
	{
		return ref<atom::Literal>(
			lv2_atom_forge_literal(this, str, len, datatype, lang));
	}

	/** Write an atom:Literal. */
	Ref<atom::Literal> literal(const char* const str,
	                           const uint32_t    datatype,
	                           const uint32_t    lang)
	{
		return literal(str, strlen(str), datatype, lang);
	}

	/** Write an atom:Literal. */
	Ref<atom::Literal> literal(const std::string& str,
	                           const uint32_t     datatype,
	                           const uint32_t     lang)
	{
		return literal(str.c_str(), str.length(), datatype, lang);
	}

	/**
	   @}
	   @name Collections
	   @{
	*/

	/** Base for all scoped collection handles. */
	template<typename T>
	struct ScopedFrame
	{
	public:
		ScopedFrame(Forge& forge) : forge(forge) {}
		~ScopedFrame() { lv2_atom_forge_pop(&forge, &frame); }

		ScopedFrame(const ScopedFrame&) = delete;
		ScopedFrame& operator=(const ScopedFrame&) = delete;

		ScopedFrame(ScopedFrame&&) = default;
		ScopedFrame& operator=(ScopedFrame&&) = default;

		T& operator*() {
			return *(T*)lv2_atom_forge_deref(&forge, frame.ref);
		}

		const T& operator*() const {
			return *(const T*)lv2_atom_forge_deref(&forge, frame.ref);
		}

		T* operator->() {
			return (T*)lv2_atom_forge_deref(&forge, frame.ref);
		}

		const T* operator->() const {
			return (const T*)lv2_atom_forge_deref(&forge, frame.ref);
		}

		Forge& forge;
		Frame  frame;
	};

	/** A scoped handle for writing a Vector.

	    For example, to write the vector [1, 2, 3]:

	    @code
	    {
	    ScopedVector vector(forge, sizeof(int32_t), forge.Int);
	    forge.write(1);
	    forge.write(2);
	    forge.write(3);
	    } // Vector finished
	    @endcode
	*/
	template<typename T>
	struct ScopedVector : ScopedFrame<atom::Vector<T>>
	{
		ScopedVector(Forge& forge, const uint32_t child_type)
			: ScopedFrame<atom::Vector<T>>(forge)
		{
			lv2_atom_forge_vector_head(
				&forge, &this->frame, sizeof(T), child_type);
		}
	};

	/** Write a complete atom:Vector. */
	template<typename T>
	Ref<atom::Vector<T>> vector(const uint32_t elem_type,
	                            const uint32_t n_elems,
	                            const T* const elems)
	{
		return ref<atom::Vector<T>>(
			lv2_atom_forge_vector(this, sizeof(T), elem_type, n_elems, elems));
	}

	/** A scoped handle for writing a Tuple.

	    For example, to write the tuple (1, 2.0):

	    @code
	    {
	    ScopedTuple tuple(forge);
	    forge.write(1);
	    forge.write(2.0f);
	    } // Tuple finished
	    @endcode
	*/
	struct ScopedTuple : ScopedFrame<atom::Tuple>
	{
		ScopedTuple(Forge& forge) : ScopedFrame<atom::Tuple>(forge)
		{
			lv2_atom_forge_tuple(&forge, &frame);
		}
	};

	/** A scoped handle for writing an Object.

	    The caller is responsible for correctly writing a sequence of
	    properties like (key, value, key, value, ...).  For example, to write
	    the object [ a eg:Cat; eg:name "Hobbes" ]:

	    @code
	    LV2_URID eg_Cat  = map("http://example.org/Cat");
	    LV2_URID eg_name = map("http://example.org/name");
	    {
	    ScopedObject object(forge, 0, eg_Cat);
	    object.key(eg_name);
	    forge.string("Hobbes", strlen("Hobbes"));
	    } // Object finished
	    @endcode
	*/
	struct ScopedObject : ScopedFrame<atom::Object>
	{
		ScopedObject(Forge& forge, LV2_URID id, LV2_URID otype)
			: ScopedFrame<atom::Object>(forge)
		{
			lv2_atom_forge_object(&forge, &frame, id, otype);
		}

		/** Write a property key, to be followed by the value. */
		Ref<atom::Property::Body> key(const LV2_URID key, const LV2_URID ctx = 0)
		{
			return forge.ref<atom::Property::Body>(
				lv2_atom_forge_property_head(&forge, key, ctx));
		}
	};

	/** A scoped handle for writing a Sequence.

	    The caller is responsible for correctly writing a sequence of events
	    like (time, value, time, value, ...).  For example:

	    @code
	    {
	    ScopedSequence sequence(forge, 0, eg_Cat);

	    sequence.frame_time(0);
	    forge.string("Bang", strlen("Bang"));

	    sequence.frame_time(8);
	    {
	    ScopedTuple tuple(forge);
	    forge.write(1);
	    forge.write(2.0f);
	    } // Tuple event body finished
	    } // Sequence finished
	    @endcode
	*/
	struct ScopedSequence : ScopedFrame<atom::Sequence>
	{
		ScopedSequence(Forge& forge, const uint32_t unit)
			: ScopedFrame<atom::Sequence>(forge)
		{
			lv2_atom_forge_sequence_head(&forge, &frame, unit);
		}

		/** Write a time stamp in frames, to be followed by the event body. */
		Ref<int64_t> frame_time(const int64_t frames)
		{
			return forge.ref<int64_t>(
				lv2_atom_forge_frame_time(&forge, frames));
		}

		/** Write a time stamp in beats, to be followed by the event body. */
		Ref<double> beat_time(const double beats)
		{
			return forge.ref<double>(
				lv2_atom_forge_beat_time(&forge, beats));
		}
	};

	/**
	   @}
	*/

private:
	template<typename T>
	Ref<T> ref(const LV2_Atom_Forge_Ref ref) { return Ref<T>(*this, ref); }
};

} // namespace atom
} // namespace lv2

/**
   @}
*/

#endif  /* LV2_ATOM_FORGE_HPP */
