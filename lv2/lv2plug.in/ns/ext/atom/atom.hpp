/*
  Copyright 2008-2014 David Robillard <http://drobilla.net>

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

#ifndef LV2_ATOM_ATOM_HPP
#define LV2_ATOM_ATOM_HPP

#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"

namespace lv2 {
namespace atom {

typedef LV2_Atom Atom;

struct Event : public LV2_Atom_Event
{
	inline uint32_t type() const { return body.type; }
	inline uint32_t size() const { return body.size; }

	/** Convenience accessor for byte-oriented payloads. */
	inline uint8_t operator[](size_t i) const {
		return *((const uint8_t*)(&body + 1) + i);
	}
};

class Sequence : public LV2_Atom_Sequence
{
public:
	class const_iterator {
	public:
		inline const_iterator(const Event* ev) : m_ev(ev) {}

		inline const_iterator operator++() {
			m_ev = (const Event*)(
				(const uint8_t*)m_ev +
				sizeof(LV2_Atom_Event) +
				lv2_atom_pad_size(m_ev->body.size));
			return *this;
		}

		inline bool operator==(const const_iterator& i) const {
			return m_ev == i.m_ev;
		}

		inline bool operator!=(const const_iterator& i) const {
			return m_ev != i.m_ev;
		}

		inline const Event& operator*()  const { return *m_ev; }
		inline const Event* operator->() const { return m_ev; }

	private:
		const Event* m_ev;
	};

	inline const_iterator begin() const {
		return const_iterator((const Event*)(&body + 1));
	}

	inline const_iterator end() const {
		return const_iterator((const Event*)((const char*)&body + atom.size));
	}
};

} // namespace atom
} // namespace lv2

#endif // LV2_ATOM_ATOM_HPP
