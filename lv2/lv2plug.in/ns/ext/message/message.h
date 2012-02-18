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
   @file message.h C header for the LV2 Message extension
   <http://lv2plug.in/ns/ext/message>.

   The message extension is purely data, this header merely defines URIs
   for convenience.
*/

#ifndef LV2_MESSAGE_H
#define LV2_MESSAGE_H

#define LV2_MESSAGE_URI "http://lv2plug.in/ns/ext/message"

#define LV2_MESSAGE__Ack      LV2_MESSAGE_URI "#Ack"
#define LV2_MESSAGE__Delete   LV2_MESSAGE_URI "#Delete"
#define LV2_MESSAGE__Error    LV2_MESSAGE_URI "#Error"
#define LV2_MESSAGE__Get      LV2_MESSAGE_URI "#Get"
#define LV2_MESSAGE__Message  LV2_MESSAGE_URI "#Message"
#define LV2_MESSAGE__Move     LV2_MESSAGE_URI "#Move"
#define LV2_MESSAGE__Patch    LV2_MESSAGE_URI "#Patch"
#define LV2_MESSAGE__Post     LV2_MESSAGE_URI "#Post"
#define LV2_MESSAGE__Put      LV2_MESSAGE_URI "#Put"
#define LV2_MESSAGE__Request  LV2_MESSAGE_URI "#Request"
#define LV2_MESSAGE__Response LV2_MESSAGE_URI "#Response"
#define LV2_MESSAGE__Set      LV2_MESSAGE_URI "#Set"
#define LV2_MESSAGE__add      LV2_MESSAGE_URI "#add"
#define LV2_MESSAGE__body     LV2_MESSAGE_URI "#body"
#define LV2_MESSAGE__request  LV2_MESSAGE_URI "#request"

#endif  /* LV2_MESSAGE_H */
