/* lv2_message.h - C header file for the LV2 Message extension.
 * Copyright (C) 2010 David Robillard <http://drobilla.net>
 *
 * This header is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as published
 * by the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This header is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or
 * FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this header; if not, write to the Free Software Foundation,
 * Inc., 59 Temple Place, Suite 330, Boston, MA 01222-1307 USA
 */

/** @file
 * C header for the LV2 Message extension <http://lv2plug.in/ns/ext/message>.
 */

#ifndef LV2_MESSAGE_H
#define LV2_MESSAGE_H

#define LV2_MESSAGE_URI "http://lv2plug.in/ns/ext/message"

#include <stdint.h>
#include <stddef.h>

#include "lv2/http/lv2plug.in/ns/ext/atom/atom.h"

/** An LV2 Message.
 *
 * A "Message" is an Atom of type message:Message.  The payload of a Message
 * is a key/value dictionary with URI mapped integer keys (uint32_t), followed
 * by a key/value dictionary with URI mapped integer keys and Atom values
 * (atom:Blank, i.e. LV2_).
 */
typedef struct _LV2_Message_Message {
	uint32_t selector; /***< Selector URI mapped to integer */
	LV2_Atom triples; /***< Always an atom:Triples */
} LV2_Message_Message;

#endif /* LV2_MESSAGE_H */

