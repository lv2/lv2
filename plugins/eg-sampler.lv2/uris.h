/*
  LV2 Sampler Example Plugin
  Copyright 2011 David Robillard <d@drobilla.net>

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

#define NS_ATOM "http://lv2plug.in/ns/ext/atom#"
#define NS_RDF  "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#define SAMPLER_URI       "http://lv2plug.in/plugins/eg-sampler"
#define MIDI_EVENT_URI    "http://lv2plug.in/ns/ext/midi#MidiEvent"
#define FILENAME_URI      SAMPLER_URI "#filename"
#define ATOM_BLANK_URI    NS_ATOM "Blank"
#define ATOM_RESOURCE_URI NS_ATOM "Resource"
