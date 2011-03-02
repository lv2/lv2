/*
  Copyright 2011 David Robillard <http://drobilla.net>

  Redistribution and use in source and binary forms, with or without
  modification, are permitted provided that the following conditions are met:

  1. Redistributions of source code must retain the above copyright notice,
     this list of conditions and the following disclaimer.

  2. Redistributions in binary form must reproduce the above copyright
     notice, this list of conditions and the following disclaimer in the
     documentation and/or other materials provided with the distribution.

  THIS SOFTWARE IS PROVIDED ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES,
  INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY
  AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE
  AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY,
  OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF
  SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS; OR BUSINESS
  INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN
  CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE)
  ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF ADVISED OF
  THE POSSIBILITY OF SUCH DAMAGE.
*/

#ifndef SERD_INTERNAL_H
#define SERD_INTERNAL_H

#include <assert.h>
#include <stdlib.h>

#include "serd/serd.h"

/** A dynamic stack in memory. */
typedef struct {
	uint8_t* buf;       ///< Stack memory
	size_t   buf_size;  ///< Allocated size of buf (>= size)
	size_t   size;      ///< Conceptual size of stack in buf
} SerdStack;

/** An offset to start the stack at. Note 0 is reserved for NULL. */
#define SERD_STACK_BOTTOM sizeof(void*)

static inline SerdStack
serd_stack_new(size_t size)
{
	SerdStack stack;
	stack.buf       = malloc(size);
	stack.buf_size  = size;
	stack.size      = SERD_STACK_BOTTOM;
	return stack;
}

static inline bool
serd_stack_is_empty(SerdStack* stack)
{
	return stack->size <= SERD_STACK_BOTTOM;
}

static inline void
serd_stack_free(SerdStack* stack)
{
	free(stack->buf);
	stack->buf      = NULL;
	stack->buf_size = 0;
	stack->size     = 0;
}

static inline uint8_t*
serd_stack_push(SerdStack* stack, size_t n_bytes)
{
	const size_t new_size = stack->size + n_bytes;
	if (stack->buf_size < new_size) {
		stack->buf_size *= 2;
		stack->buf = realloc(stack->buf, stack->buf_size);
	}
	uint8_t* const ret = (stack->buf + stack->size);
	stack->size = new_size;
	return ret;
}

static inline void
serd_stack_pop(SerdStack* stack, size_t n_bytes)
{
	assert(stack->size >= n_bytes);
	stack->size -= n_bytes;
}

/** Return true if @a c lies within [min...max] (inclusive) */
static inline bool
in_range(const uint8_t c, const uint8_t min, const uint8_t max)
{
	return (c >= min && c <= max);
}

/** RFC2234: ALPHA := %x41-5A / %x61-7A  ; A-Z / a-z */
static inline bool
is_alpha(const uint8_t c)
{
	return in_range(c, 'A', 'Z') || in_range(c, 'a', 'z');
}

/** RFC2234: DIGIT ::= %x30-39  ; 0-9 */
static inline bool
is_digit(const uint8_t c)
{
	return in_range(c, '0', '9');
}

/** UTF-8 strlen.
 * @return Lengh of @a utf8 in characters.
 * @param utf8 A null-terminated UTF-8 string.
 * @param out_n_bytes (Output) Set to the size of @a utf8 in bytes.
 */
static inline size_t
serd_strlen(const uint8_t* utf8, size_t* out_n_bytes)
{
	size_t n_chars = 0;
	size_t i       = 0;
	for (; utf8[i]; ++i) {
		if ((utf8[i] & 0xC0) != 0x80) {
			// Does not start with `10', start of a new character
			++n_chars;
		}
	}
	if (out_n_bytes) {
		*out_n_bytes = i + 1;
	}
	return n_chars;
}

#endif  // SERD_INTERNAL_H

/**
 * @file env.c
 */

#include <assert.h>
#include <stdbool.h>
#include <stdlib.h>
#include <string.h>


typedef struct {
	SerdNode name;
	SerdNode uri;
} SerdPrefix;

struct SerdEnvImpl {
	SerdPrefix* prefixes;
	size_t      n_prefixes;
};

SERD_API
SerdEnv
serd_env_new()
{
	SerdEnv env = malloc(sizeof(struct SerdEnvImpl));
	env->prefixes   = NULL;
	env->n_prefixes = 0;
	return env;
}

SERD_API
void
serd_env_free(SerdEnv env)
{
	for (size_t i = 0; i < env->n_prefixes; ++i) {
		serd_node_free(&env->prefixes[i].name);
		serd_node_free(&env->prefixes[i].uri);
	}
	free(env->prefixes);
	free(env);
}

static inline SerdPrefix*
serd_env_find(SerdEnv        env,
              const uint8_t* name,
              size_t         name_len)
{
	for (size_t i = 0; i < env->n_prefixes; ++i) {
		const SerdNode* const prefix_name = &env->prefixes[i].name;
		if (prefix_name->n_bytes == name_len + 1) {
			if (!memcmp(prefix_name->buf, name, name_len)) {
				return &env->prefixes[i];
			}
		}
	}
	return NULL;
}

SERD_API
void
serd_env_add(SerdEnv         env,
             const SerdNode* name,
             const SerdNode* uri)
{
	assert(name && uri);
	SerdPrefix* const prefix = serd_env_find(env, name->buf, name->n_chars);
	if (prefix) {
		serd_node_free(&prefix->uri);
		prefix->uri = serd_node_copy(uri);
	} else {
		env->prefixes = realloc(env->prefixes,
		                        (++env->n_prefixes) * sizeof(SerdPrefix));
		env->prefixes[env->n_prefixes - 1].name = serd_node_copy(name);
		env->prefixes[env->n_prefixes - 1].uri  = serd_node_copy(uri);
	}
}

SERD_API
bool
serd_env_qualify(const SerdEnv   env,
                 const SerdNode* uri,
                 SerdNode*       prefix_name,
                 SerdChunk*      suffix)
{
	for (size_t i = 0; i < env->n_prefixes; ++i) {
		const SerdNode* const prefix_uri = &env->prefixes[i].uri;
		if (uri->n_bytes >= prefix_uri->n_bytes) {
			if (!strncmp((const char*)uri->buf,
			             (const char*)prefix_uri->buf,
			             prefix_uri->n_bytes - 1)) {
				*prefix_name = env->prefixes[i].name;
				suffix->buf = uri->buf + prefix_uri->n_bytes - 1;
				suffix->len = uri->n_bytes - prefix_uri->n_bytes;
				return true;
			}
		}
	}
	return false;
}

SERD_API
bool
serd_env_expand(const SerdEnv   env,
                const SerdNode* qname,
                SerdChunk*      uri_prefix,
                SerdChunk*      uri_suffix)
{
	const uint8_t* const colon = memchr(qname->buf, ':', qname->n_bytes);
	if (!colon) {
		return false;  // Illegal qname
	}

	const size_t            name_len = colon - qname->buf;
	const SerdPrefix* const prefix   = serd_env_find(env, qname->buf, name_len);
	if (prefix) {
		uri_prefix->buf = prefix->uri.buf;
		uri_prefix->len = prefix->uri.n_bytes - 1;
		uri_suffix->buf = colon + 1;
		uri_suffix->len = qname->n_bytes - (colon - qname->buf) - 2;
		return true;
	}
	return false;
}

SERD_API
void
serd_env_foreach(const SerdEnv  env,
                 SerdPrefixSink func,
                 void*          handle)
{
	for (size_t i = 0; i < env->n_prefixes; ++i) {
		func(handle,
		     &env->prefixes[i].name,
		     &env->prefixes[i].uri);
	}
}

/**
 * @file node.c
 */

#include <stdlib.h>
#include <string.h>


SERD_API
SerdNode
serd_node_from_string(SerdType type, const uint8_t* buf)
{
	size_t buf_n_bytes;
	const size_t buf_n_chars = serd_strlen(buf, &buf_n_bytes);
	SerdNode ret = { type, buf_n_bytes, buf_n_chars, buf };
	return ret;
}

SERD_API
SerdNode
serd_node_copy(const SerdNode* node)
{
	SerdNode copy = *node;
	uint8_t* buf  = malloc(copy.n_bytes);
	memcpy(buf, node->buf, copy.n_bytes);
	copy.buf = buf;
	return copy;
}

static size_t
serd_uri_string_length(const SerdURI* uri)
{
	size_t len = uri->path_base.len;

#define ADD_LEN(field, n_delims) \
	if ((field).len) { len += (field).len + (n_delims); }

	ADD_LEN(uri->path,      1);  // + possible leading `/'
	ADD_LEN(uri->scheme,    1);  // + trailing `:'
	ADD_LEN(uri->authority, 2);  // + leading `//'
	ADD_LEN(uri->query,     1);  // + leading `?'
	ADD_LEN(uri->fragment,  1);  // + leading `#'

	// Add 2 for authority // prefix (added even though authority.len = 0)
	return len + 2; // + 2 for authority //
}

static size_t
string_sink(const void* buf, size_t len, void* stream)
{
	uint8_t** ptr = (uint8_t**)stream;
	memcpy(*ptr, buf, len);
	*ptr += len;
	return len;
}

SERD_API
SerdNode
serd_node_new_uri_from_node(const SerdNode* uri_node,
                            const SerdURI*  base,
                            SerdURI*        out)
{
	return serd_node_new_uri_from_string(uri_node->buf, base, out);
}

SERD_API
SerdNode
serd_node_new_uri_from_string(const uint8_t* str,
                              const SerdURI* base,
                              SerdURI*       out)
{
	if (str[0] == '\0') {
		return serd_node_new_uri(base, NULL, out);  // Empty URI => Base URI
	} else {
		SerdURI uri;
		if (serd_uri_parse(str, &uri)) {
			return serd_node_new_uri(&uri, base, out);  // Resolve/Serialise
		}
	}
	return SERD_NODE_NULL;
}

SERD_API
SerdNode
serd_node_new_uri(const SerdURI* uri, const SerdURI* base, SerdURI* out)
{
	SerdURI abs_uri = *uri;
	if (base) {
		serd_uri_resolve(uri, base, &abs_uri);
	}
		
	const size_t len = serd_uri_string_length(&abs_uri);
	uint8_t*     buf = malloc(len + 1);

	SerdNode node = { SERD_URI, len + 1, len, buf };  // FIXME: UTF-8

	uint8_t*     ptr        = buf;
	const size_t actual_len = serd_uri_serialise(&abs_uri, string_sink, &ptr);

	buf[actual_len] = '\0';
	node.n_bytes    = actual_len + 1;
	node.n_chars    = actual_len;

	// FIXME: double parse
	if (!serd_uri_parse(buf, out)) {
		fprintf(stderr, "error parsing URI\n");
		return SERD_NODE_NULL;
	}

	return node;
}

SERD_API
void
serd_node_free(SerdNode* node)
{
	free((uint8_t*)node->buf);
}

/**
 * @file reader.c
 */

#include <assert.h>
#include <stdarg.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NS_XSD "http://www.w3.org/2001/XMLSchema#"
#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"

#define TRY_THROW(exp) if (!(exp)) goto except;
#define TRY_RET(exp)   if (!(exp)) return 0;

#define STACK_PAGE_SIZE 4096
#define READ_BUF_LEN    4096

typedef struct {
	const uint8_t* filename;
	unsigned       line;
	unsigned       col;
} Cursor;

typedef uint32_t uchar;

typedef size_t Ref;

typedef struct {
	SerdType type;
	Ref      value;
	Ref      datatype;
	Ref      lang;
} Node;

typedef struct {
	const Node* graph;
	const Node* subject;
	const Node* predicate;
} ReadContext;

/** Measured UTF-8 string. */
typedef struct {
	size_t  n_bytes;  ///< Size in bytes including trailing null byte
	size_t  n_chars;  ///< Length in characters
	uint8_t buf[];    ///< Buffer
} SerdString;

static const Node INTERNAL_NODE_NULL = { 0, 0, 0, 0 };

struct SerdReaderImpl {
	void*             handle;
	SerdBaseSink      base_sink;
	SerdPrefixSink    prefix_sink;
	SerdStatementSink statement_sink;
	SerdEndSink       end_sink;
	Node              rdf_type;
	Node              rdf_first;
	Node              rdf_rest;
	Node              rdf_nil;
	FILE*             fd;
	SerdStack         stack;
	Cursor            cur;
	uint8_t*          buf;
	const uint8_t*    blank_prefix;
	unsigned          next_id;
	int               err;
	uint8_t*          read_buf;
	int32_t           read_head;    ///< Offset into read_buf
	bool              from_file;    ///< True iff reading from @ref fd
	bool              eof;
#ifdef SUIL_STACK_CHECK
	Ref*              alloc_stack;  ///< Stack of push offsets
	size_t            n_allocs;     ///< Number of stack pushes
#endif
};

struct SerdReadStateImpl {
	SerdEnv  env;
	SerdNode base_uri_node;
	SerdURI  base_uri;
};

typedef enum {
	SERD_SUCCESS = 0,  ///< Completed successfully
	SERD_FAILURE = 1,  ///< Non-fatal failure
	SERD_ERROR   = 2,  ///< Fatal error
} SerdStatus;

static inline int
error(SerdReader reader, const char* fmt, ...)
{
	va_list args;
	va_start(args, fmt);
	fprintf(stderr, "error: %s:%u:%u: ",
	        reader->cur.filename, reader->cur.line, reader->cur.col);
	vfprintf(stderr, fmt, args);
	return 0;
}

static Node
make_node(SerdType type, Ref value, Ref datatype, Ref lang)
{
	const Node ret = { type, value, datatype, lang };
	return ret;
}

static inline bool
page(SerdReader reader)
{
	assert(reader->from_file);
	reader->read_head = 0;
	const size_t n_read = fread(reader->read_buf, 1, READ_BUF_LEN, reader->fd);
	if (n_read == 0) {
		reader->read_buf[0] = '\0';
		reader->eof = true;
		return false;
	} else if (n_read < READ_BUF_LEN) {
		reader->read_buf[n_read] = '\0';
	}
	return true;
}

static inline bool
peek_string(SerdReader reader, uint8_t* pre, int n)
{
	uint8_t* ptr = reader->read_buf + reader->read_head;
	for (int i = 0; i < n; ++i) {
		if (reader->from_file && (reader->read_head + i >= READ_BUF_LEN)) {
			if (!page(reader)) {
				return false;
			}
			ptr = reader->read_buf;
			reader->read_head = -i;
			memcpy(reader->read_buf + reader->read_head, pre, i);
			assert(reader->read_buf[reader->read_head] == pre[0]);
		}
		if ((pre[i] = *ptr++) == '\0') {
			return false;
		}
	}
	return true;
}

static inline uint8_t
peek_byte(SerdReader reader)
{
	return reader->read_buf[reader->read_head];
}

static inline uint8_t
eat_byte(SerdReader reader, const uint8_t byte)
{
	const uint8_t c = peek_byte(reader);
	++reader->read_head;
	switch (c) {
	case '\n': ++reader->cur.line; reader->cur.col = 0; break;
	default:   ++reader->cur.col;
	}

	if (c != byte) {
		return error(reader, "expected `%c', not `%c'\n", byte, c);
	}
	if (reader->from_file && (reader->read_head == READ_BUF_LEN)) {
		TRY_RET(page(reader));
		assert(reader->read_head < READ_BUF_LEN);
	}
	if (reader->read_buf[reader->read_head] == '\0') {
		reader->eof = true;
	}
	return c;
}

static inline void
eat_string(SerdReader reader, const char* str, unsigned n)
{
	for (unsigned i = 0; i < n; ++i) {
		eat_byte(reader, ((const uint8_t*)str)[i]);
	}
}

#ifdef SUIL_STACK_CHECK
static inline bool
stack_is_top_string(SerdReader reader, Ref ref)
{
	return ref == reader->alloc_stack[reader->n_allocs - 1];
}
#endif

static inline intptr_t
pad_size(intptr_t size)
{
	return (size + 7) & (~7);
}

// Make a new string from a non-UTF-8 C string (internal use only)
static Ref
push_string(SerdReader reader, const char* c_str, size_t n_bytes)
{
	// Align strings to 64-bits (assuming malloc/realloc are aligned to 64-bits)
	const size_t stack_size = pad_size((intptr_t)reader->stack.size);
	const size_t pad        = stack_size - reader->stack.size;
	uint8_t*     mem        = serd_stack_push(
		&reader->stack, pad + sizeof(SerdString) + n_bytes) + pad;
	SerdString* const str = (SerdString*)mem;
	str->n_bytes = n_bytes;
	str->n_chars = n_bytes - 1;
	memcpy(str->buf, c_str, n_bytes);
#ifdef SUIL_STACK_CHECK
	reader->alloc_stack = realloc(reader->alloc_stack,
	                              sizeof(uint8_t*) * (++reader->n_allocs));
	reader->alloc_stack[reader->n_allocs - 1] = (mem - reader->stack.buf);
#endif
	return (uint8_t*)str - reader->stack.buf;
}

static inline SerdString*
deref(SerdReader reader, const Ref ref)
{
	if (ref) {
		return (SerdString*)(reader->stack.buf + ref);
	}
	return NULL;
}

static inline void
push_byte(SerdReader reader, Ref ref, const uint8_t c)
{
	#ifdef SUIL_STACK_CHECK
	assert(stack_is_top_string(reader, ref));
	#endif
	serd_stack_push(&reader->stack, 1);
	SerdString* const str = deref(reader, ref);
	++str->n_bytes;
	if ((c & 0xC0) != 0x80) {
		// Does not start with `10', start of a new character
		++str->n_chars;
	}
	assert(str->n_bytes > str->n_chars);
	str->buf[str->n_bytes - 2] = c;
	str->buf[str->n_bytes - 1] = '\0';
}

static void
pop_string(SerdReader reader, Ref ref)
{
	if (ref) {
		if (ref == reader->rdf_nil.value
		    || ref == reader->rdf_first.value
		    || ref == reader->rdf_rest.value) {
			return;
		}
		#ifdef SUIL_STACK_CHECK
		if (!stack_is_top_string(reader, ref)) {
			fprintf(stderr, "attempt to pop non-top string %s\n",
			        deref(reader, ref)->buf);
			fprintf(stderr, "top: %s\n",
			        deref(reader, reader->alloc_stack[reader->n_allocs - 1])->buf);
		}
		assert(stack_is_top_string(reader, ref));
		--reader->n_allocs;
		#endif
		serd_stack_pop(&reader->stack, deref(reader, ref)->n_bytes);
	}
}

static inline SerdNode
public_node_from_ref(SerdReader reader, SerdType type, Ref ref)
{
	if (!ref) {
		return SERD_NODE_NULL;
	}
	const SerdString* str    = deref(reader, ref);
	const SerdNode    public = { type, str->n_bytes, str->n_chars, str->buf };
	return public;
}

static inline SerdNode
public_node(SerdReader reader, const Node* private)
{
	return public_node_from_ref(reader, private->type, private->value);
}

	
static inline bool
emit_statement(SerdReader reader,
               const Node* g, const Node* s, const Node* p, const Node* o)
{
	assert(s->value && p->value && o->value);
	const SerdNode graph           = g ? public_node(reader, g) : SERD_NODE_NULL;
	const SerdNode subject         = public_node(reader, s);
	const SerdNode predicate       = public_node(reader, p);
	const SerdNode object          = public_node(reader, o);
	const SerdNode object_datatype = public_node_from_ref(reader, SERD_URI, o->datatype);
	const SerdNode object_lang     = public_node_from_ref(reader, SERD_LITERAL, o->lang);
	return reader->statement_sink(reader->handle,
	                              &graph,
	                              &subject,
	                              &predicate,
	                              &object,
	                              &object_datatype,
	                              &object_lang);
}

static bool read_collection(SerdReader reader, ReadContext ctx, Node* dest);
static bool read_predicateObjectList(SerdReader reader, ReadContext ctx);

// [40]	hex	::=	[#x30-#x39] | [#x41-#x46]
static inline uint8_t
read_hex(SerdReader reader)
{
	const uint8_t c = peek_byte(reader);
	if (in_range(c, 0x30, 0x39) || in_range(c, 0x41, 0x46)) {
		return eat_byte(reader, c);
	} else {
		return error(reader, "illegal hexadecimal digit `%c'\n", c);
	}
}

static inline bool
read_hex_escape(SerdReader reader, unsigned length, Ref dest)
{
	uint8_t buf[9] = { 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (unsigned i = 0; i < length; ++i) {
		buf[i] = read_hex(reader);
	}

	uint32_t c;
	sscanf((const char*)buf, "%X", &c);

	unsigned size = 0;
	if (c < 0x00000080) {
		size = 1;
	} else if (c < 0x00000800) {
		size = 2;
	} else if (c < 0x00010000) {
		size = 3;
	} else if (c < 0x00200000) {
		size = 4;
	} else {
		return false;
	}

	// Build output in buf
	// (Note # of bytes = # of leading 1 bits in first byte)
	switch (size) {
	case 4:
		buf[3] = 0x80 | (uint8_t)(c & 0x3F);
		c >>= 6;
		c |= (16 << 12);  // set bit 4
	case 3:
		buf[2] = 0x80 | (uint8_t)(c & 0x3F);
		c >>= 6;
		c |= (32 << 6);  // set bit 5
	case 2:
		buf[1] = 0x80 | (uint8_t)(c & 0x3F);
		c >>= 6;
		c |= 0xC0;  // set bits 6 and 7
	case 1:
		buf[0] = (uint8_t)c;
	}

	for (unsigned i = 0; i < size; ++i) {
		push_byte(reader, dest, buf[i]);
	}
	return true;
}

static inline bool
read_character_escape(SerdReader reader, Ref dest)
{
	switch (peek_byte(reader)) {
	case '\\':
		push_byte(reader, dest, eat_byte(reader, '\\'));
		return true;
	case 'u':
		eat_byte(reader, 'u');
		return read_hex_escape(reader, 4, dest);
	case 'U':
		eat_byte(reader, 'U');
		return read_hex_escape(reader, 8, dest);
	default:
		return false;
	}
}

static inline bool
read_echaracter_escape(SerdReader reader, Ref dest)
{
	switch (peek_byte(reader)) {
	case 't':
		eat_byte(reader, 't');
		push_byte(reader, dest, '\t');
		return true;
	case 'n':
		eat_byte(reader, 'n');
		push_byte(reader, dest, '\n');
		return true;
	case 'r':
		eat_byte(reader, 'r');
		push_byte(reader, dest, '\r');
		return true;
	default:
		return read_character_escape(reader, dest);
	}
}

static inline bool
read_scharacter_escape(SerdReader reader, Ref dest)
{
	switch (peek_byte(reader)) {
	case '"':
		push_byte(reader, dest, eat_byte(reader, '"'));
		return true;
	default:
		return read_echaracter_escape(reader, dest);
	}
}

static inline bool
read_ucharacter_escape(SerdReader reader, Ref dest)
{
	switch (peek_byte(reader)) {
	case '>':
		push_byte(reader, dest, eat_byte(reader, '>'));
		return true;
	default:
		return read_echaracter_escape(reader, dest);
	}
}

// [38] character ::= '\u' hex hex hex hex
//    | '\U' hex hex hex hex hex hex hex hex
//    | '\\'
//    | [#x20-#x5B] | [#x5D-#x10FFFF]
static inline SerdStatus
read_character(SerdReader reader, Ref dest)
{
	const uint8_t c = peek_byte(reader);
	assert(c != '\\');  // Only called from methods that handle escapes first
	switch (c) {
	case '\0':
		error(reader, "unexpected end of file\n", peek_byte(reader));
		return SERD_ERROR;
	default:
		if (c < 0x20) {  // ASCII control character
			error(reader, "unexpected control character\n");
			return SERD_ERROR;
		} else if (c <= 0x7E) {  // Printable ASCII
			push_byte(reader, dest, eat_byte(reader, c));
			return SERD_SUCCESS;
		} else {  // Wide UTF-8 character
			unsigned size = 1;
			if ((c & 0xE0) == 0xC0) {  // Starts with `110'
				size = 2;
			} else if ((c & 0xF0) == 0xE0) {  // Starts with `1110'
				size = 3;
			} else if ((c & 0xF8) == 0xF0) {  // Starts with `11110'
				size = 4;
			} else {
				error(reader, "invalid character\n");
				return SERD_ERROR;
			}
			for (unsigned i = 0; i < size; ++i) {
				push_byte(reader, dest, eat_byte(reader, peek_byte(reader)));
			}
			return SERD_SUCCESS;
		}
	}
}

// [39] echaracter ::= character | '\t' | '\n' | '\r'
static inline SerdStatus
read_echaracter(SerdReader reader, Ref dest)
{
	uint8_t c = peek_byte(reader);
	switch (c) {
	case '\\':
		eat_byte(reader, '\\');
		if (read_echaracter_escape(reader, peek_byte(reader))) {
			return SERD_SUCCESS;
		} else {
			error(reader, "illegal escape `\\%c'\n", peek_byte(reader));
			return SERD_ERROR;
		}
	default:
		return read_character(reader, dest);
	}
}

// [43] lcharacter ::= echaracter | '\"' | #x9 | #xA | #xD
static inline SerdStatus
read_lcharacter(SerdReader reader, Ref dest)
{
	const uint8_t c = peek_byte(reader);
	uint8_t       pre[3];
	switch (c) {
	case '"':
		peek_string(reader, pre, 3);
		if (pre[1] == '\"' && pre[2] == '\"') {
			eat_byte(reader, '\"');
			eat_byte(reader, '\"');
			eat_byte(reader, '\"');
			return SERD_FAILURE;
		} else {
			push_byte(reader, dest, eat_byte(reader, '"'));
			return SERD_SUCCESS;
		}
	case '\\':
		eat_byte(reader, '\\');
		if (read_scharacter_escape(reader, dest)) {
			return SERD_SUCCESS;
		} else {
			error(reader, "illegal escape `\\%c'\n", peek_byte(reader));
			return SERD_ERROR;
		}
	case 0x9: case 0xA: case 0xD:
		push_byte(reader, dest, eat_byte(reader, c));
		return SERD_SUCCESS;
	default:
		return read_echaracter(reader, dest);
	}
}

// [42] scharacter ::= ( echaracter - #x22 ) | '\"'
static inline SerdStatus
read_scharacter(SerdReader reader, Ref dest)
{
	uint8_t c = peek_byte(reader);
	switch (c) {
	case '\\':
		eat_byte(reader, '\\');
		if (read_scharacter_escape(reader, dest)) {
			return SERD_SUCCESS;
		} else {
			error(reader, "illegal escape `\\%c'\n", peek_byte(reader));
			return SERD_ERROR;
		}
	case '\"':
		return SERD_FAILURE;
	default:
		return read_character(reader, dest);
	}
}

// Spec: [41] ucharacter ::= ( character - #x3E ) | '\>'
// Impl: [41] ucharacter ::= ( echaracter - #x3E ) | '\>'
static inline SerdStatus
read_ucharacter(SerdReader reader, Ref dest)
{
	const uint8_t c = peek_byte(reader);
	switch (c) {
	case '\\':
		eat_byte(reader, '\\');
		if (read_ucharacter_escape(reader, dest)) {
			return SERD_SUCCESS;
		} else {
			return error(reader, "illegal escape `\\%c'\n", peek_byte(reader));
		}
	case '>':
		return SERD_FAILURE;
	default:
		return read_character(reader, dest);
	}
}

// [10] comment ::= '#' ( [^#xA #xD] )*
static void
read_comment(SerdReader reader)
{
	eat_byte(reader, '#');
	uint8_t c;
	while (((c = peek_byte(reader)) != 0xA) && (c != 0xD)) {
		eat_byte(reader, c);
	}
}

// [24] ws ::= #x9 | #xA | #xD | #x20 | comment
static inline bool
read_ws(SerdReader reader)
{
	const uint8_t c = peek_byte(reader);
	switch (c) {
	case 0x9: case 0xA: case 0xD: case 0x20:
		eat_byte(reader, c);
		return true;
	case '#':
		read_comment(reader);
		return true;
	default:
		return false;
	}
}

static inline void
read_ws_star(SerdReader reader)
{
	while (read_ws(reader)) {}
}

static inline bool
read_ws_plus(SerdReader reader)
{
	TRY_RET(read_ws(reader));
	read_ws_star(reader);
	return true;
}

// [37] longSerdString ::= #x22 #x22 #x22 lcharacter* #x22 #x22 #x22
static Ref
read_longString(SerdReader reader)
{
	eat_string(reader, "\"\"\"", 3);
	Ref        str = push_string(reader, "", 1);
	SerdStatus st;
	while (!(st = read_lcharacter(reader, str))) {}
	if (st != SERD_ERROR) {
		return str;
	}
	pop_string(reader, str);
	return 0;
}

// [36] string ::= #x22 scharacter* #x22
static Ref
read_string(SerdReader reader)
{
	eat_byte(reader, '\"');
	Ref        str = push_string(reader, "", 1);
	SerdStatus st;
	while (!(st = read_scharacter(reader, str))) {}
	if (st != SERD_ERROR) {
		eat_byte(reader, '\"');
		return str;
	}
	pop_string(reader, str);
	return 0;
}

// [35] quotedString ::= string | longSerdString
static Ref
read_quotedString(SerdReader reader)
{
	uint8_t pre[3];
	peek_string(reader, pre, 3);
	assert(pre[0] == '\"');
	switch (pre[1]) {
	case '\"':
		if (pre[2] == '\"')
			return read_longString(reader);
		else
			return read_string(reader);
	default:
		return read_string(reader);
	}
}

// [34] relativeURI ::= ucharacter*
static inline Ref
read_relativeURI(SerdReader reader)
{
	Ref str = push_string(reader, "", 1);
	SerdStatus st;
	while (!(st = read_ucharacter(reader, str))) {}
	if (st != SERD_ERROR) {
		return str;
	}
	pop_string(reader, str);
	return 0;
}

// [30] nameStartChar ::= [A-Z] | "_" | [a-z]
//    | [#x00C0-#x00D6] | [#x00D8-#x00F6] | [#x00F8-#x02FF] | [#x0370-#x037D]
//    | [#x037F-#x1FFF] | [#x200C-#x200D] | [#x2070-#x218F] | [#x2C00-#x2FEF]
//    | [#x3001-#xD7FF] | [#xF900-#xFDCF] | [#xFDF0-#xFFFD] | [#x10000-#xEFFFF]
static inline uchar
read_nameStartChar(SerdReader reader, bool required)
{
	const uint8_t c = peek_byte(reader);
	if (c == '_' || is_alpha(c)) {
		return eat_byte(reader, c);
	} else {
		if (required) {
			error(reader, "illegal character `%c'\n", c);
		}
		return 0;
	}
}

// [31] nameChar ::= nameStartChar | '-' | [0-9]
//    | #x00B7 | [#x0300-#x036F] | [#x203F-#x2040]
static inline uchar
read_nameChar(SerdReader reader)
{
	uchar c = read_nameStartChar(reader, false);
	if (c)
		return c;

	switch ((c = peek_byte(reader))) {
	case '-': case 0xB7: case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		return eat_byte(reader, c);
	default:
		// TODO: 0x300-0x036F | 0x203F-0x2040
		return 0;
	}
	return 0;
}

// [33] prefixName ::= ( nameStartChar - '_' ) nameChar*
static Ref
read_prefixName(SerdReader reader)
{
	uint8_t c = peek_byte(reader);
	if (c == '_') {
		error(reader, "unexpected `_'\n");
		return 0;
	}
	TRY_RET(c = read_nameStartChar(reader, false));
	Ref str = push_string(reader, "", 1);
	push_byte(reader, str, c);
	while ((c = read_nameChar(reader)) != 0) {
		push_byte(reader, str, c);
	}
	return str;
}

// [32] name ::= nameStartChar nameChar*
static Ref
read_name(SerdReader reader, Ref dest, bool required)
{
	uchar c = read_nameStartChar(reader, required);
	if (!c) {
		if (required) {
			error(reader, "illegal character at start of name\n");
		}
		return 0;
	}
	do {
		push_byte(reader, dest, c);
	} while ((c = read_nameChar(reader)) != 0);
	return dest;
}

// [29] language ::= [a-z]+ ('-' [a-z0-9]+ )*
static Ref
read_language(SerdReader reader)
{
	const uint8_t start = peek_byte(reader);
	if (!in_range(start, 'a', 'z')) {
		error(reader, "unexpected `%c'\n", start);
		return 0;
	}
	Ref str = push_string(reader, "", 1);
	push_byte(reader, str, eat_byte(reader, start));
	uint8_t c;
	while ((c = peek_byte(reader)) && in_range(c, 'a', 'z')) {
		push_byte(reader, str, eat_byte(reader, c));
	}
	if (peek_byte(reader) == '-') {
		push_byte(reader, str, eat_byte(reader, '-'));
		while ((c = peek_byte(reader)) && (
			       in_range(c, 'a', 'z') || in_range(c, '0', '9'))) {
			push_byte(reader, str, eat_byte(reader, c));
		}
	}
	return str;
}

// [28] uriref ::= '<' relativeURI '>'
static Ref
read_uriref(SerdReader reader)
{
	TRY_RET(eat_byte(reader, '<'));
	Ref const str = read_relativeURI(reader);
	if (str && eat_byte(reader, '>')) {
		return str;
	}
	pop_string(reader, str);
	return 0;
}

// [27] qname ::= prefixName? ':' name?
static Ref
read_qname(SerdReader reader)
{
	Ref prefix = read_prefixName(reader);
	if (!prefix) {
		prefix = push_string(reader, "", 1);
	}
	TRY_THROW(eat_byte(reader, ':'));
	push_byte(reader, prefix, ':');
	Ref str = read_name(reader, prefix, false);
	return str ? str : prefix;
except:
	pop_string(reader, prefix);
	return 0;
}

static bool
read_0_9(SerdReader reader, Ref str, bool at_least_one)
{
	uint8_t c;
	if (at_least_one) {
		if (!is_digit((c = peek_byte(reader)))) {
			return error(reader, "expected digit\n");
		}
		push_byte(reader, str, eat_byte(reader, c));
	}
	while (is_digit((c = peek_byte(reader)))) {
		push_byte(reader, str, eat_byte(reader, c));
	}
	return true;
}

// [19] exponent ::= [eE] ('-' | '+')? [0-9]+
// [18] decimal ::= ( '-' | '+' )? ( [0-9]+ '.' [0-9]*
//                                  | '.' ([0-9])+
//                                  | ([0-9])+ )
// [17] double  ::= ( '-' | '+' )? ( [0-9]+ '.' [0-9]* exponent
//                                  | '.' ([0-9])+ exponent
//                                  | ([0-9])+ exponent )
// [16] integer ::= ( '-' | '+' ) ? [0-9]+
static bool
read_number(SerdReader reader, Node* dest)
{
	#define XSD_DECIMAL NS_XSD "decimal"
	#define XSD_DOUBLE  NS_XSD "double"
	#define XSD_INTEGER NS_XSD "integer"
	Ref     str         = push_string(reader, "", 1);
	uint8_t c           = peek_byte(reader);
	bool    has_decimal = false;
	Ref     datatype    = 0;
	if (c == '-' || c == '+') {
		push_byte(reader, str, eat_byte(reader, c));
	}
	if ((c = peek_byte(reader)) == '.') {
		has_decimal = true;
		// decimal case 2 (e.g. '.0' or `-.0' or `+.0')
		push_byte(reader, str, eat_byte(reader, c));
		TRY_THROW(read_0_9(reader, str, true));
	} else {
		// all other cases ::= ( '-' | '+' ) [0-9]+ ( . )? ( [0-9]+ )? ...
		TRY_THROW(read_0_9(reader, str, true));
		if ((c = peek_byte(reader)) == '.') {
			has_decimal = true;
			push_byte(reader, str, eat_byte(reader, c));
			TRY_THROW(read_0_9(reader, str, false));
		}
	}
	c = peek_byte(reader);
	if (c == 'e' || c == 'E') {
		// double
		push_byte(reader, str, eat_byte(reader, c));
		switch ((c = peek_byte(reader))) {
		case '+': case '-':
			push_byte(reader, str, eat_byte(reader, c));
		default: break;
		}
		read_0_9(reader, str, true);
		datatype = push_string(reader, XSD_DOUBLE, strlen(XSD_DOUBLE) + 1);
	} else if (has_decimal) {
		datatype = push_string(reader, XSD_DECIMAL, strlen(XSD_DECIMAL) + 1);
	} else {
		datatype = push_string(reader, XSD_INTEGER, strlen(XSD_INTEGER) + 1);
	}
	*dest = make_node(SERD_LITERAL, str, datatype, 0);
	assert(dest->value);
	return true;
except:
	pop_string(reader, datatype);
	pop_string(reader, str);
	return false;
}

// [25] resource ::= uriref | qname
static bool
read_resource(SerdReader reader, Node* dest)
{
	switch (peek_byte(reader)) {
	case '<':
		*dest = make_node(SERD_URI, read_uriref(reader), 0, 0);
		break;
	default:
		*dest = make_node(SERD_CURIE, read_qname(reader), 0, 0);
	}
	return (dest->value != 0);
}

// [14] literal ::= quotedString ( '@' language )? | datatypeSerdString
//    | integer | double | decimal | boolean
static bool
read_literal(SerdReader reader, Node* dest)
{
	Ref           str      = 0;
	Node          datatype = INTERNAL_NODE_NULL;
	const uint8_t c        = peek_byte(reader);
	if (c == '-' || c == '+' || c == '.' || is_digit(c)) {
		return read_number(reader, dest);
	} else if (c == '\"') {
		str = read_quotedString(reader);
		if (!str) {
			return false;
		}

		Ref lang = 0;
		switch (peek_byte(reader)) {
		case '^':
			eat_byte(reader, '^');
			eat_byte(reader, '^');
			TRY_THROW(read_resource(reader, &datatype));
			break;
		case '@':
			eat_byte(reader, '@');
			TRY_THROW(lang = read_language(reader));
		}
		*dest = make_node(SERD_LITERAL, str, datatype.value, lang);
	} else {
		return error(reader, "Unknown literal type\n");
	}
	return true;
except:
	pop_string(reader, str);
	return false;
}

// [12] predicate ::= resource
static bool
read_predicate(SerdReader reader, Node* dest)
{
	return read_resource(reader, dest);
}

// [9] verb ::= predicate | 'a'
static bool
read_verb(SerdReader reader, Node* dest)
{
	uint8_t pre[2];
	peek_string(reader, pre, 2);
	switch (pre[0]) {
	case 'a':
		switch (pre[1]) {
		case 0x9: case 0xA: case 0xD: case 0x20:
			eat_byte(reader, 'a');
			*dest = make_node(SERD_URI,
			                  push_string(reader, NS_RDF "type", 48), 0, 0);
			return true;
		default: break;  // fall through
		}
	default:
		return read_predicate(reader, dest);
	}
}

// [26] nodeID ::= '_:' name
static Ref
read_nodeID(SerdReader reader)
{
	eat_byte(reader, '_');
	eat_byte(reader, ':');
	Ref str = push_string(reader, "", 1);
	return read_name(reader, str, true);
}

static Ref
blank_id(SerdReader reader)
{
	const char* prefix = reader->blank_prefix
		? (const char*)reader->blank_prefix
		: "genid";
	char str[32];  // FIXME: ensure length of reader->blank_prefix is OK
	const int len = snprintf(str, sizeof(str), "%s%u",
	                         prefix, reader->next_id++);
	return push_string(reader, str, len + 1);
}

// Spec: [21] blank ::= nodeID | '[]'
//          | '[' predicateObjectList ']' | collection
// Impl: [21] blank ::= nodeID | '[ ws* ]'
//          | '[' ws* predicateObjectList ws* ']' | collection
static bool
read_blank(SerdReader reader, ReadContext ctx, Node* dest)
{
	switch (peek_byte(reader)) {
	case '_':
		*dest = make_node(SERD_BLANK_ID, read_nodeID(reader), 0, 0);
		return true;
	case '[':
		eat_byte(reader, '[');
		read_ws_star(reader);
		if (peek_byte(reader) == ']') {
			eat_byte(reader, ']');
			*dest = make_node(SERD_BLANK_ID, blank_id(reader), 0, 0);
			if (ctx.subject) {
				TRY_RET(emit_statement(reader, ctx.graph, ctx.subject, ctx.predicate, dest));
			}
			return true;
		}
		*dest = make_node(SERD_ANON_BEGIN, blank_id(reader), 0, 0);
		if (ctx.subject) {
			TRY_RET(emit_statement(reader, ctx.graph, ctx.subject, ctx.predicate, dest));
			dest->type = SERD_ANON;
		}
		ctx.subject = dest;
		read_predicateObjectList(reader, ctx);
		read_ws_star(reader);
		eat_byte(reader, ']');
		if (reader->end_sink) {
			const SerdNode end = public_node(reader, dest);
			reader->end_sink(reader->handle, &end);
		}
		return true;
	case '(':
		if (read_collection(reader, ctx, dest)) {
			if (ctx.subject) {
				TRY_RET(emit_statement(reader, ctx.graph, ctx.subject, ctx.predicate, dest));
			}
			return true;
		}
		return false;
	default:
		return error(reader, "illegal blank node\n");
	}
}

inline static bool
is_object_end(const uint8_t c)
{
	switch (c) {
	case 0x9: case 0xA: case 0xD: case 0x20: case '\0':
	case '#': case '.': case ';':
		return true;
	default:
		return false;
	}
}

// [13] object ::= resource | blank | literal
// Recurses, calling statement_sink for every statement encountered.
// Leaves stack in original calling state (i.e. pops everything it pushes).
static bool
read_object(SerdReader reader, ReadContext ctx)
{
	static const char* const XSD_BOOLEAN     = NS_XSD "boolean";
	static const size_t      XSD_BOOLEAN_LEN = 40;

	uint8_t       pre[6];
	bool          ret  = false;
	bool          emit = (ctx.subject != 0);
	Node          o    = INTERNAL_NODE_NULL;
	const uint8_t c    = peek_byte(reader);
	switch (c) {
	case '\0':
	case ')':
		return false;
	case '[': case '(':
		emit = false;
		// fall through
	case '_':
		TRY_THROW(ret = read_blank(reader, ctx, &o));
		break;
	case '<': case ':':
		TRY_THROW(ret = read_resource(reader, &o));
		break;
	case '\"': case '+': case '-':
	case '0': case '1': case '2': case '3': case '4':
	case '5': case '6': case '7': case '8': case '9':
		TRY_THROW(ret = read_literal(reader, &o));
		break;
	case '.':
		TRY_THROW(ret = read_literal(reader, &o));
		break;
	default:
		/* Either a boolean literal, or a qname.
		   Unfortunately there is no way to distinguish these without
		   readahead, since `true' or `false' could be the start of a qname.
		*/
		peek_string(reader, pre, 6);
		if (!memcmp(pre, "true", 4) && is_object_end(pre[4])) {
			eat_string(reader, "true", 4);
			const Ref value     = push_string(reader, "true", 5);
			const Ref datatype  = push_string(reader, XSD_BOOLEAN, XSD_BOOLEAN_LEN + 1);
			o = make_node(SERD_LITERAL, value, datatype, 0);
		} else if (!memcmp(pre, "false", 5) && is_object_end(pre[5])) {
			eat_string(reader, "false", 5);
			const Ref value     = push_string(reader, "false", 6);
			const Ref datatype  = push_string(reader, XSD_BOOLEAN, XSD_BOOLEAN_LEN + 1);
			o = make_node(SERD_LITERAL, value, datatype, 0);
		} else if (!is_object_end(c)) {
			o = make_node(SERD_CURIE, read_qname(reader), 0, 0);
		}
		ret = o.value;
	}

	if (ret && emit) {
		assert(o.value);
		ret = emit_statement(reader, ctx.graph, ctx.subject, ctx.predicate, &o);
	}

except:
	pop_string(reader, o.lang);
	pop_string(reader, o.datatype);
	pop_string(reader, o.value);
	return ret;
}

// Spec: [8] objectList ::= object ( ',' object )*
// Impl: [8] objectList ::= object ( ws* ',' ws* object )*
static bool
read_objectList(SerdReader reader, ReadContext ctx)
{
	TRY_RET(read_object(reader, ctx));
	read_ws_star(reader);
	while (peek_byte(reader) == ',') {
		eat_byte(reader, ',');
		read_ws_star(reader);
		TRY_RET(read_object(reader, ctx));
		read_ws_star(reader);
	}
	return true;
}

// Spec: [7] predicateObjectList ::= verb objectList
//                                   (';' verb objectList)* (';')?
// Impl: [7] predicateObjectList ::= verb ws+ objectList
//                                   (ws* ';' ws* verb ws+ objectList)* (';')?
static bool
read_predicateObjectList(SerdReader reader, ReadContext ctx)
{
	if (reader->eof) {
		return false;
	}
	Node predicate = INTERNAL_NODE_NULL;
	TRY_RET(read_verb(reader, &predicate));
	TRY_THROW(read_ws_plus(reader));
	ctx.predicate = &predicate;
	TRY_THROW(read_objectList(reader, ctx));
	pop_string(reader, predicate.value);
	predicate.value = 0;
	read_ws_star(reader);
	while (peek_byte(reader) == ';') {
		eat_byte(reader, ';');
		read_ws_star(reader);
		switch (peek_byte(reader)) {
		case '.': case ']':
			return true;
		default:
			TRY_THROW(read_verb(reader, &predicate));
			ctx.predicate = &predicate;
			TRY_THROW(read_ws_plus(reader));
			TRY_THROW(read_objectList(reader, ctx));
			pop_string(reader, predicate.value);
			predicate.value = 0;
			read_ws_star(reader);
		}
	}
	return true;
except:
	pop_string(reader, predicate.value);
	return false;
}

/** Recursive helper for read_collection. */
static bool
read_collection_rec(SerdReader reader, ReadContext ctx)
{
	read_ws_star(reader);
	if (peek_byte(reader) == ')') {
		eat_byte(reader, ')');
		TRY_RET(emit_statement(reader, NULL, ctx.subject,
		                       &reader->rdf_rest, &reader->rdf_nil));
		return false;
	} else {
		const Node rest = make_node(SERD_BLANK_ID, blank_id(reader), 0, 0);
		TRY_RET(emit_statement(reader, ctx.graph, ctx.subject, &reader->rdf_rest, &rest));
		ctx.subject = &rest;
		ctx.predicate = &reader->rdf_first;
		if (read_object(reader, ctx)) {
			read_collection_rec(reader, ctx);
			pop_string(reader, rest.value);
			return true;
		} else {
			pop_string(reader, rest.value);
			return false;
		}
	}
}

// [22] itemList   ::= object+
// [23] collection ::= '(' itemList? ')'
static bool
read_collection(SerdReader reader, ReadContext ctx, Node* dest)
{
	TRY_RET(eat_byte(reader, '('));
	read_ws_star(reader);
	if (peek_byte(reader) == ')') {  // Empty collection
		eat_byte(reader, ')');
		*dest = reader->rdf_nil;
		return true;
	}

	*dest = make_node(SERD_BLANK_ID, blank_id(reader), 0, 0);
	ctx.subject   = dest;
	ctx.predicate = &reader->rdf_first;
	if (!read_object(reader, ctx)) {
		return error(reader, "unexpected end of collection\n");
	}

	ctx.subject = dest;
	return read_collection_rec(reader, ctx);
}

// [11] subject ::= resource | blank
static Node
read_subject(SerdReader reader, ReadContext ctx)
{
	Node    subject = INTERNAL_NODE_NULL;
	switch (peek_byte(reader)) {
	case '[': case '(': case '_':
		read_blank(reader, ctx, &subject);
		break;
	default:
		read_resource(reader, &subject);
	}
	return subject;
}

// Spec: [6] triples ::= subject predicateObjectList
// Impl: [6] triples ::= subject ws+ predicateObjectList
static bool
read_triples(SerdReader reader, ReadContext ctx)
{
	const Node subject = read_subject(reader, ctx);
	bool       ret     = false;
	if (subject.value != 0) {
		ctx.subject = &subject;
		TRY_RET(read_ws_plus(reader));
		ret = read_predicateObjectList(reader, ctx);
		pop_string(reader, subject.value);
	}
	ctx.subject = ctx.predicate = 0;
	return ret;
}

// [5] base ::= '@base' ws+ uriref
static bool
read_base(SerdReader reader)
{
	// `@' is already eaten in read_directive
	eat_string(reader, "base", 4);
	TRY_RET(read_ws_plus(reader));
	Ref uri;
	TRY_RET(uri = read_uriref(reader));
	const SerdNode uri_node = public_node_from_ref(reader, SERD_URI, uri);
	reader->base_sink(reader->handle, &uri_node);
	pop_string(reader, uri);
	return true;
}

// Spec: [4] prefixID ::= '@prefix' ws+ prefixName? ':' uriref
// Impl: [4] prefixID ::= '@prefix' ws+ prefixName? ':' ws* uriref
static bool
read_prefixID(SerdReader reader)
{
	// `@' is already eaten in read_directive
	eat_string(reader, "prefix", 6);
	TRY_RET(read_ws_plus(reader));
	bool ret = false;
	Ref name = read_prefixName(reader);
	if (!name) {
		name = push_string(reader, "", 1);
	}
	TRY_THROW(eat_byte(reader, ':') == ':');
	read_ws_star(reader);
	Ref uri = 0;
	TRY_THROW(uri = read_uriref(reader));
	const SerdNode name_node = public_node_from_ref(reader, SERD_LITERAL, name);
	const SerdNode uri_node  = public_node_from_ref(reader, SERD_URI, uri);
	ret = reader->prefix_sink(reader->handle, &name_node, &uri_node);
	pop_string(reader, uri);
except:
	pop_string(reader, name);
	return ret;
}

// [3] directive ::= prefixID | base
static bool
read_directive(SerdReader reader)
{
	eat_byte(reader, '@');
	switch (peek_byte(reader)) {
	case 'b':
		return read_base(reader);
	case 'p':
		return read_prefixID(reader);
	default:
		return error(reader, "illegal directive\n");
	}
}

// Spec: [1] statement ::= directive '.' | triples '.' | ws+
// Impl: [1] statement ::= directive ws* '.' | triples ws* '.' | ws+
static bool
read_statement(SerdReader reader)
{
	ReadContext ctx = { 0, 0, 0 };
	read_ws_star(reader);
	if (reader->eof) {
		return true;
	}
	switch (peek_byte(reader)) {
	case '@':
		TRY_RET(read_directive(reader));
		break;
	default:
		TRY_RET(read_triples(reader, ctx));
		break;
	}
	read_ws_star(reader);
	return eat_byte(reader, '.');
}

// [1] turtleDoc ::= statement
static bool
read_turtleDoc(SerdReader reader)
{
	while (!reader->eof) {
		TRY_RET(read_statement(reader));
	}
	return true;
}

SERD_API
SerdReader
serd_reader_new(SerdSyntax        syntax,
                void*             handle,
                SerdBaseSink      base_sink,
                SerdPrefixSink    prefix_sink,
                SerdStatementSink statement_sink,
                SerdEndSink       end_sink)
{
	const Cursor cur = { NULL, 0, 0 };
	SerdReader   me  = malloc(sizeof(struct SerdReaderImpl));
	me->handle         = handle;
	me->base_sink      = base_sink;
	me->prefix_sink    = prefix_sink;
	me->statement_sink = statement_sink;
	me->end_sink       = end_sink;
	me->fd             = 0;
	me->stack          = serd_stack_new(STACK_PAGE_SIZE);
	me->cur            = cur;
	me->blank_prefix   = NULL;
	me->next_id        = 1;
	me->read_buf       = 0;
	me->read_head      = 0;
	me->eof            = false;
#ifdef SERD_STACK_CHECK
	me->alloc_stack    = 0;
	me->n_allocs       = 0;
#endif

#define RDF_FIRST NS_RDF "first"
#define RDF_REST  NS_RDF "rest"
#define RDF_NIL   NS_RDF "nil"
	me->rdf_first = make_node(SERD_URI, push_string(me, RDF_FIRST, 49), 0, 0);
	me->rdf_rest  = make_node(SERD_URI, push_string(me, RDF_REST, 48), 0, 0);
	me->rdf_nil   = make_node(SERD_URI, push_string(me, RDF_NIL, 47), 0, 0);

	return me;
}

SERD_API
void
serd_reader_free(SerdReader reader)
{
	SerdReader const me = (SerdReader)reader;
	pop_string(me, me->rdf_nil.value);
	pop_string(me, me->rdf_rest.value);
	pop_string(me, me->rdf_first.value);

#ifdef SERD_STACK_CHECK
	free(me->alloc_stack);
#endif
	free(me->stack.buf);
	free(me);
}

SERD_API
void
serd_reader_set_blank_prefix(SerdReader     reader,
                             const uint8_t* prefix)
{
	reader->blank_prefix = prefix;
}

SERD_API
bool
serd_reader_read_file(SerdReader me, FILE* file, const uint8_t* name)
{
	const Cursor cur = { name, 1, 1 };
	me->fd        = file;
	me->read_buf  = (uint8_t*)malloc(READ_BUF_LEN * 2);
	me->read_head = 0;
	me->cur       = cur;
	me->from_file = true;
	me->eof       = false;

	/* Read into the second page of the buffer. Occasionally peek_string
	   will move the read_head to before this point when readahead causes
	   a page fault.
	*/
	memset(me->read_buf, '\0', READ_BUF_LEN * 2);
	me->read_buf += READ_BUF_LEN;

	const bool ret = !page(me) || read_turtleDoc(me);

	free(me->read_buf - READ_BUF_LEN);
	me->fd       = 0;
	me->read_buf = NULL;
	return ret;
}

SERD_API
bool
serd_reader_read_string(SerdReader me, const uint8_t* utf8)
{
	const Cursor cur = { (const uint8_t*)"(string)", 1, 1 };

	me->read_buf  = (uint8_t*)utf8;
	me->read_head = 0;
	me->cur       = cur;
	me->from_file = false;

	const bool ret = read_turtleDoc(me);

	me->read_buf = NULL;
	return ret;
}

SERD_API
SerdReadState
serd_read_state_new(SerdEnv        env,
                    const uint8_t* base_uri_str)
{
	SerdReadState state         = malloc(sizeof(struct SerdReadStateImpl));
	SerdURI       base_base_uri = SERD_URI_NULL;
	state->env           = env;
	state->base_uri_node = serd_node_new_uri_from_string(
		base_uri_str, &base_base_uri, &state->base_uri);
	return state;
}

SERD_API
void
serd_read_state_free(SerdReadState state)
{
	serd_node_free(&state->base_uri_node);
	free(state);
}

SERD_API
SerdNode
serd_read_state_expand(SerdReadState   state,
                       const SerdNode* node)
{
	if (node->type == SERD_CURIE) {
		SerdChunk prefix;
		SerdChunk suffix;
		serd_env_expand(state->env, node, &prefix, &suffix);
		SerdNode ret = { SERD_URI,
		                 prefix.len + suffix.len + 1,
		                 prefix.len + suffix.len, // FIXME: UTF-8
		                 NULL };
		ret.buf = malloc(ret.n_bytes);
		snprintf((char*)ret.buf, ret.n_bytes, "%s%s", prefix.buf, suffix.buf);
		return ret;
	} else if (node->type == SERD_URI) {
		SerdURI ignored;
		return serd_node_new_uri_from_node(node, &state->base_uri, &ignored);
	} else {
		return SERD_NODE_NULL;
	}
}

SERD_API
SerdNode
serd_read_state_get_base_uri(SerdReadState state,
                             SerdURI*      out)
{
	*out = state->base_uri;
	return state->base_uri_node;
}

SERD_API
bool
serd_read_state_set_base_uri(SerdReadState   state,
                             const SerdNode* uri_node)
{
	// Resolve base URI and create a new node and URI for it
	SerdURI  base_uri;
	SerdNode base_uri_node = serd_node_new_uri_from_node(
		uri_node, &state->base_uri, &base_uri);

	if (base_uri_node.buf) {
		// Replace the current base URI
		serd_node_free(&state->base_uri_node);
		state->base_uri_node = base_uri_node;
		state->base_uri      = base_uri;
		return true;
	}
	return false;
}

SERD_API
bool
serd_read_state_set_prefix(SerdReadState   state,
                           const SerdNode* name,
                           const SerdNode* uri_node)
{
	if (serd_uri_string_has_scheme(uri_node->buf)) {
		// Set prefix to absolute URI
		serd_env_add(state->env, name, uri_node);
		return true;
	} else {
		// Resolve relative URI and create a new node and URI for it
		SerdURI  abs_uri;
		SerdNode abs_uri_node = serd_node_new_uri_from_node(
			uri_node, &state->base_uri, &abs_uri);

		if (!abs_uri_node.buf) {
			return false;
		}

		// Set prefix to resolved (absolute) URI
		serd_env_add(state->env, name, &abs_uri_node);
		serd_node_free(&abs_uri_node);
		return true;
	}
	return false;
}


/**
 * @file uri.c
 */

/** @file uri.c */

#include <assert.h>
#include <stdlib.h>
#include <string.h>


// #define URI_DEBUG 1

SERD_API
bool
serd_uri_string_has_scheme(const uint8_t* utf8)
{
	// RFC3986: scheme ::= ALPHA *( ALPHA / DIGIT / "+" / "-" / "." )
	if (!is_alpha(utf8[0])) {
		return false;  // Invalid scheme initial character, URI is relative
	}
	for (uint8_t c = *++utf8; (c = *utf8) != '\0'; ++utf8) {
		switch (c) {
		case ':':
			return true;  // End of scheme
		case '+': case '-': case '.':
			break;  // Valid scheme character, continue
		default:
			if (!is_alpha(c) && !is_digit(c)) {
				return false;  // Invalid scheme character
			}
		}
	}

	return false;
}

#ifdef URI_DEBUG
static void
serd_uri_dump(const SerdURI* uri, FILE* file)
{
#define PRINT_PART(range, name) \
	if (range.buf) { \
		fprintf(stderr, "  " name " = "); \
		fwrite((range).buf, 1, (range).len, stderr); \
		fprintf(stderr, "\n"); \
	}

	PRINT_PART(uri->scheme,    "scheme");
	PRINT_PART(uri->authority, "authority");
	PRINT_PART(uri->path_base, "path_base");
	PRINT_PART(uri->path,      "path");
	PRINT_PART(uri->query,     "query");
	PRINT_PART(uri->fragment,  "fragment");
}
#endif

SERD_API
bool
serd_uri_parse(const uint8_t* utf8, SerdURI* uri)
{
	*uri = SERD_URI_NULL;
	assert(uri->path_base.buf == NULL);
	assert(uri->path_base.len == 0);
	assert(uri->authority.len == 0);

	const uint8_t* ptr = utf8;

	/* See http://tools.ietf.org/html/rfc3986#section-3
	   URI = scheme ":" hier-part [ "?" query ] [ "#" fragment ]
	*/

	/* S3.1: scheme ::= ALPHA *( ALPHA / DIGIT / "+" / "-" / "." ) */
	if (is_alpha(*ptr)) {
		for (uint8_t c = *++ptr; true; c = *++ptr) {
			switch (c) {
			case '\0': case '/': case '?': case '#':
				ptr = utf8;
				goto path;  // Relative URI (starts with path by definition)
			case ':':
				uri->scheme.buf = utf8;
				uri->scheme.len = (ptr++) - utf8;
				goto maybe_authority;  // URI with scheme
			case '+': case '-': case '.':
				continue;
			default:
				if (is_alpha(c) || is_digit(c)) {
					continue;
				}
			}
		}
	}

	/* S3.2: The authority component is preceded by a double slash ("//")
	   and is terminated by the next slash ("/"), question mark ("?"),
	   or number sign ("#") character, or by the end of the URI.
	*/
maybe_authority:
	if (*ptr == '/' && *(ptr + 1) == '/') {
		ptr += 2;
		uri->authority.buf = ptr;
		assert(uri->authority.len == 0);
		for (uint8_t c = *ptr; (c = *ptr) != '\0'; ++ptr) {
			switch (c) {
			case '/': goto path;
			case '?': goto query;
			case '#': goto fragment;
			default:
				++uri->authority.len;
			}
		}
	}

	/* RFC3986 S3.3: The path is terminated by the first question mark ("?")
	   or number sign ("#") character, or by the end of the URI.
	*/
path:
	switch (*ptr) {
	case '?':  goto query;
	case '#':  goto fragment;
	case '\0': goto end;
	default:  break;
	}
	uri->path.buf = ptr;
	uri->path.len = 0;
	for (uint8_t c = *ptr; (c = *ptr) != '\0'; ++ptr) {
		switch (c) {
		case '?': goto query;
		case '#': goto fragment;
		default:
			++uri->path.len;
		}
	}

	/* RFC3986 S3.4: The query component is indicated by the first question
	   mark ("?") character and terminated by a number sign ("#") character
	   or by the end of the URI.
	*/
query:
	if (*ptr == '?') {
		uri->query.buf = ++ptr;
		for (uint8_t c = *ptr; (c = *ptr) != '\0'; ++ptr) {
			switch (c) {
			case '#':
				goto fragment;
			default:
				++uri->query.len;
			}
		}
	}

	/* RFC3986 S3.5: A fragment identifier component is indicated by the
	   presence of a number sign ("#") character and terminated by the end
	   of the URI.
	*/
fragment:
	if (*ptr == '#') {
		uri->fragment.buf = ptr;
		while (*ptr++ != '\0') {
			++uri->fragment.len;
		}
	}

end:
	#ifdef URI_DEBUG
	fprintf(stderr, "PARSE URI <%s>\n", utf8);
	serd_uri_dump(uri, stderr);
	fprintf(stderr, "\n");
	#endif

	return true;
}

SERD_API
void
serd_uri_resolve(const SerdURI* r, const SerdURI* base, SerdURI* t)
{
	// See http://tools.ietf.org/html/rfc3986#section-5.2.2

	t->path_base.buf = NULL;
	t->path_base.len = 0;
	if (r->scheme.len) {
		*t = *r;
	} else {
		if (r->authority.len) {
			t->authority = r->authority;
			t->path      = r->path;
			t->query     = r->query;
		} else {
			t->path = r->path;
			if (!r->path.len) {
				t->path_base = base->path;
				if (r->query.len) {
					t->query = r->query;
				} else {
					t->query = base->query;
				}
			} else {
				if (r->path.buf[0] != '/') {
					t->path_base = base->path;
				}
				t->query = r->query;
			}
			t->authority = base->authority;
		}
		t->scheme   = base->scheme;
		t->fragment = r->fragment;
	}

	#ifdef URI_DEBUG
	fprintf(stderr, "RESOLVE URI\nBASE:\n");
	serd_uri_dump(base, stderr);
	fprintf(stderr, "URI:\n");
	serd_uri_dump(r, stderr);
	fprintf(stderr, "RESULT:\n");
	serd_uri_dump(t, stderr);
	fprintf(stderr, "\n");
	#endif
}

SERD_API
size_t
serd_uri_serialise(const SerdURI* uri, SerdSink sink, void* stream)
{
	// See http://tools.ietf.org/html/rfc3986#section-5.3

	size_t write_size = 0;
#define WRITE(buf, len) \
	write_size += len; \
	if (len) { \
		sink((const uint8_t*)buf, len, stream); \
	}
#define WRITE_CHAR(c) WRITE(&(c), 1)
#define WRITE_COMPONENT(prefix, field, suffix) \
	if ((field).len) { \
		for (const uint8_t* c = (const uint8_t*)prefix; *c != '\0'; ++c) { \
			WRITE(c, 1); \
		} \
		WRITE((field).buf, (field).len); \
		for (const uint8_t* c = (const uint8_t*)suffix; *c != '\0'; ++c) { \
			WRITE(c, 1); \
		} \
	}

	WRITE_COMPONENT("",   uri->scheme,    ":");
	if (uri->authority.buf) {
		WRITE("//", 2);
		WRITE(uri->authority.buf, uri->authority.len);
	}
	if (uri->path_base.len) {
		if (!uri->path.buf && (uri->fragment.buf || uri->query.buf)) {
			WRITE_COMPONENT("", uri->path_base, "");
		} else {
			/* Merge paths, removing dot components.
			   See http://tools.ietf.org/html/rfc3986#section-5.2.3
			*/
			const uint8_t* begin = uri->path.buf;
			const uint8_t* end   = begin;
			size_t         up        = 1;
			if (begin) {
				// Count and skip leading dot components
				end = uri->path.buf + uri->path.len;
				for (bool done = false; !done && (begin < end);) {
					switch (begin[0]) {
					case '.':
						switch (begin[1]) {
						case '/':
							begin += 2;  // Chop leading "./"
							break;
						case '.':
							++up;
							switch (begin[2]) {
							case '/':
								begin += 3;  // Chop lading "../"
								break;
							default:
								begin += 2;  // Chop leading ".."
							}
							break;
						default:
							++begin;  // Chop leading "."
						}
						break;
					case '/':
						if (begin[1] == '/') {
							++begin;  // Replace leading "//" with "/"
							break;
						}  // else fall through
					default:
						done = true;  // Finished chopping dot components
					}
				}

				if (uri->path.buf && uri->path_base.buf) {
					// Find the up'th last slash
					const uint8_t* base_last = uri->path_base.buf + uri->path_base.len - 1;
					do {
						if (*base_last == '/') {
							--up;
						}
					} while (up > 0 && (--base_last > uri->path_base.buf));

					// Write base URI prefix
					const size_t base_len = base_last - uri->path_base.buf + 1;
					WRITE(uri->path_base.buf, base_len);

				} else {
					// Relative path is just query or fragment, append it to full base URI
					WRITE_COMPONENT("", uri->path_base, "");
				}

				// Write URI suffix
				WRITE(begin, end - begin);
			}
		}
	} else {
		WRITE_COMPONENT("", uri->path, "");
	}
	WRITE_COMPONENT("?", uri->query, "");
	if (uri->fragment.buf) {
		// Note uri->fragment.buf includes the leading `#'
		WRITE_COMPONENT("", uri->fragment, "");
	}
	return write_size;
}

/**
 * @file writer.c
 */

#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>


#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_XSD "http://www.w3.org/2001/XMLSchema#"

typedef struct {
	SerdNode graph;
	SerdNode subject;
	SerdNode predicate;
} WriteContext;

static const WriteContext WRITE_CONTEXT_NULL = {
	{ 0, 0, 0, 0}, {0, 0, 0, 0}, {0, 0, 0, 0}
};

struct SerdWriterImpl {
	SerdSyntax   syntax;
	SerdStyle    style;
	SerdEnv      env;
	SerdURI      base_uri;
	SerdStack    anon_stack;
	SerdSink     sink;
	void*        stream;
	WriteContext context;
	unsigned     indent;
};

typedef enum {
	WRITE_NORMAL,
	WRITE_URI,
	WRITE_STRING
} TextContext;

static inline WriteContext*
anon_stack_top(SerdWriter writer)
{
	assert(!serd_stack_is_empty(&writer->anon_stack));
	return (WriteContext*)(writer->anon_stack.buf
	                       + writer->anon_stack.size - sizeof(WriteContext));
}

static bool
write_text(SerdWriter writer, TextContext ctx,
           const uint8_t* utf8, size_t n_bytes, uint8_t terminator)
{
	char escape[10] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
	for (size_t i = 0; i < n_bytes;) {
		uint8_t in = utf8[i++];
		switch (in) {
		case '\\': writer->sink("\\\\", 2, writer->stream); continue;
		case '\n': writer->sink("\\n", 2, writer->stream);  continue;
		case '\r': writer->sink("\\r", 2, writer->stream);  continue;
		case '\t': writer->sink("\\t", 2, writer->stream);  continue;
		case '"':
			if (terminator == '"') {
				writer->sink("\\\"", 2, writer->stream);
				continue;
			}  // else fall-through
		default: break;
		}

		if (in == terminator) {
			snprintf(escape, 7, "\\u%04X", terminator);
			writer->sink(escape, 6, writer->stream);
			continue;
		}

		uint32_t c    = 0;
		size_t   size = 0;
		if ((in & 0x80) == 0) {  // Starts with `0'
			size = 1;
			c = in & 0x7F;
			if (in_range(in, 0x20, 0x7E)) {  // Printable ASCII
				writer->sink(&in, 1, writer->stream);
				continue;
			}
		} else if ((in & 0xE0) == 0xC0) {  // Starts with `110'
			size = 2;
			c = in & 0x1F;
		} else if ((in & 0xF0) == 0xE0) {  // Starts with `1110'
			size = 3;
			c = in & 0x0F;
		} else if ((in & 0xF8) == 0xF0) {  // Starts with `11110'
			size = 4;
			c = in & 0x07;
		} else {
			fprintf(stderr, "invalid UTF-8 at offset %zu: %X\n", i, in);
			return false;
		}

		if (ctx == WRITE_STRING && !(writer->style & SERD_STYLE_ASCII)) {
			// Write UTF-8 character directly to UTF-8 output
			// TODO: Scan to next escape and write entire range at once
			writer->sink(utf8 + i - 1, size, writer->stream);
			i += size - 1;
			continue;
		}

#define READ_BYTE() do { \
			assert(i < n_bytes); \
			in = utf8[i++] & 0x3f; \
			c <<= 6; \
			c |= in; \
		} while (0)

		switch (size) {
		case 4: READ_BYTE();
		case 3: READ_BYTE();
		case 2: READ_BYTE();
		}

		if (c < 0xFFFF) {
			snprintf(escape, 7, "\\u%04X", c);
			writer->sink(escape, 6, writer->stream);
		} else {
			snprintf(escape, 11, "\\U%08X", c);
			writer->sink(escape, 10, writer->stream);
		}
	}
	return true;
}

static void
serd_writer_write_delim(SerdWriter writer, const uint8_t delim)
{
	switch (delim) {
	case '\n':
		break;
	default:
		writer->sink(" ", 1, writer->stream);
	case '[':
		writer->sink(&delim, 1, writer->stream);
	}
	writer->sink("\n", 1, writer->stream);
	for (unsigned i = 0; i < writer->indent; ++i) {
		writer->sink("\t", 1, writer->stream);
	}
}

static bool
write_node(SerdWriter      writer,
           const SerdNode* node,
           const SerdNode* datatype,
           const SerdNode* lang)
{
	SerdChunk uri_prefix;
	SerdChunk uri_suffix;
	switch (node->type) {
	case SERD_NOTHING:
		return false;
	case SERD_ANON_BEGIN:
		if (writer->syntax != SERD_NTRIPLES) {
			++writer->indent;
			serd_writer_write_delim(writer, '[');
			WriteContext* ctx = (WriteContext*)serd_stack_push(
				&writer->anon_stack, sizeof(WriteContext));
			*ctx = writer->context;
			writer->context.subject   = *node;
			writer->context.predicate = SERD_NODE_NULL;
			break;
		}
	case SERD_ANON:
		if (writer->syntax != SERD_NTRIPLES) {
			break;
		}  // else fall through
	case SERD_BLANK_ID:
		writer->sink("_:", 2, writer->stream);
		writer->sink(node->buf, node->n_bytes - 1, writer->stream);
		break;
	case SERD_CURIE:
		switch (writer->syntax) {
		case SERD_NTRIPLES:
			if (!serd_env_expand(writer->env, node, &uri_prefix, &uri_suffix)) {
				fprintf(stderr, "error: undefined namespace prefix `%s'\n", node->buf);
				return false;
			}
			writer->sink("<", 1, writer->stream);
			write_text(writer, WRITE_URI, uri_prefix.buf, uri_prefix.len, '>');
			write_text(writer, WRITE_URI, uri_suffix.buf, uri_suffix.len, '>');
			writer->sink(">", 1, writer->stream);
			break;
		case SERD_TURTLE:
			writer->sink(node->buf, node->n_bytes - 1, writer->stream);
		}
		break;
	case SERD_LITERAL:
		if (writer->syntax == SERD_TURTLE && datatype && datatype->buf) {
			// TODO: compare against NS_XSD prefix once
			if (!strcmp((const char*)datatype->buf,    NS_XSD "boolean")
			    || !strcmp((const char*)datatype->buf, NS_XSD "decimal")
			    || !strcmp((const char*)datatype->buf, NS_XSD "integer")) {
				writer->sink(node->buf, node->n_bytes - 1, writer->stream);
				break;
			}
		}
		writer->sink("\"", 1, writer->stream);
		write_text(writer, WRITE_STRING, node->buf, node->n_bytes - 1, '"');
		writer->sink("\"", 1, writer->stream);
		if (lang && lang->buf) {
			writer->sink("@", 1, writer->stream);
			writer->sink(lang->buf, lang->n_bytes - 1, writer->stream);
		} else if (datatype && datatype->buf) {
			writer->sink("^^", 2, writer->stream);
			write_node(writer, datatype, NULL, NULL);
		}
		break;
	case SERD_URI:
		if ((writer->syntax == SERD_TURTLE)
		    && !strcmp((const char*)node->buf, NS_RDF "type")) {
			writer->sink("a", 1, writer->stream);
			return true;
		} else if ((writer->style & SERD_STYLE_CURIED)
		           && serd_uri_string_has_scheme(node->buf)) {
			SerdNode  prefix;
			SerdChunk suffix;
			if (serd_env_qualify(writer->env, node, &prefix, &suffix)) {
				write_text(writer, WRITE_URI, prefix.buf, prefix.n_bytes - 1, '>');
				writer->sink(":", 1, writer->stream);
				write_text(writer, WRITE_URI, suffix.buf, suffix.len, '>');
				return true;
			}
		} else if ((writer->style & SERD_STYLE_RESOLVED)
		           && !serd_uri_string_has_scheme(node->buf)) {
			SerdURI uri;
			if (serd_uri_parse(node->buf, &uri)) {
				SerdURI abs_uri;
				serd_uri_resolve(&uri, &writer->base_uri, &abs_uri);
				writer->sink("<", 1, writer->stream);
				serd_uri_serialise(&abs_uri, writer->sink, writer->stream);
				writer->sink(">", 1, writer->stream);
				return true;
			}
		}
		writer->sink("<", 1, writer->stream);
		write_text(writer, WRITE_URI, node->buf, node->n_bytes - 1, '>');
		writer->sink(">", 1, writer->stream);
		return true;
	}
	return true;
}

SERD_API
bool
serd_writer_write_statement(SerdWriter      writer,
                            const SerdNode* graph,
                            const SerdNode* subject,
                            const SerdNode* predicate,
                            const SerdNode* object,
                            const SerdNode* object_datatype,
                            const SerdNode* object_lang)
{
	assert(subject && predicate && object);
	switch (writer->syntax) {
	case SERD_NTRIPLES:
		write_node(writer, subject, NULL, NULL);
		writer->sink(" ", 1, writer->stream);
		write_node(writer, predicate, NULL, NULL);
		writer->sink(" ", 1, writer->stream);
		if (!write_node(writer, object, object_datatype, object_lang)) {
			return false;
		}
		writer->sink(" .\n", 3, writer->stream);
		return true;
	case SERD_TURTLE:
		break;
	}
	if (subject->buf == writer->context.subject.buf) {
		if (predicate->buf == writer->context.predicate.buf) {  // Abbreviate S P
			++writer->indent;
			serd_writer_write_delim(writer, ',');
			write_node(writer, object, object_datatype, object_lang);
			--writer->indent;
		} else {  // Abbreviate S
			if (writer->context.predicate.buf) {
				serd_writer_write_delim(writer, ';');
			} else {
				++writer->indent;
				serd_writer_write_delim(writer, '\n');
			}
			write_node(writer, predicate, NULL, NULL);
			writer->context.predicate = *predicate;
			writer->sink(" ", 1, writer->stream);
			write_node(writer, object, object_datatype, object_lang);
		}
	} else {
		if (writer->context.subject.buf) {
			if (writer->indent > 0) {
				--writer->indent;
			}
			if (serd_stack_is_empty(&writer->anon_stack)) {
				serd_writer_write_delim(writer, '.');
				serd_writer_write_delim(writer, '\n');
			}
		}

		if (subject->type == SERD_ANON_BEGIN) {
			writer->sink("[ ", 2, writer->stream);
			++writer->indent;
			WriteContext* ctx = (WriteContext*)serd_stack_push(
				&writer->anon_stack, sizeof(WriteContext));
			*ctx = writer->context;
		} else {
			write_node(writer, subject, NULL, NULL);
			++writer->indent;
			if (subject->type != SERD_ANON_BEGIN && subject->type != SERD_ANON) {
				serd_writer_write_delim(writer, '\n');
			}
		}

		writer->context.subject   = *subject;
		writer->context.predicate = SERD_NODE_NULL;

		write_node(writer, predicate, NULL, NULL);
		writer->context.predicate = *predicate;
		writer->sink(" ", 1, writer->stream);

		write_node(writer, object, object_datatype, object_lang);
	}

	const WriteContext new_context = { graph ? *graph : SERD_NODE_NULL,
	                                   *subject,
	                                   *predicate };
	writer->context = new_context;
	return true;
}

SERD_API
bool
serd_writer_end_anon(SerdWriter      writer,
                     const SerdNode* node)
{
	if (writer->syntax == SERD_NTRIPLES) {
		return true;
	}
	if (serd_stack_is_empty(&writer->anon_stack)) {
		fprintf(stderr, "unexpected end of anonymous node\n");
		return false;
	}
	assert(writer->indent > 0);
	--writer->indent;
	serd_writer_write_delim(writer, '\n');
	writer->sink("]", 1, writer->stream);
	writer->context = *anon_stack_top(writer);
	serd_stack_pop(&writer->anon_stack, sizeof(WriteContext));
	if (!writer->context.subject.buf) {  // End of anonymous subject
		writer->context.subject = *node;
	}
	return true;
}

SERD_API
void
serd_writer_finish(SerdWriter writer)
{
	if (writer->context.subject.buf) {
		writer->sink(" .\n", 3, writer->stream);
		writer->context.subject.buf = NULL;
	}
}

SERD_API
SerdWriter
serd_writer_new(SerdSyntax     syntax,
                SerdStyle      style,
                SerdEnv        env,
                const SerdURI* base_uri,
                SerdSink       sink,
                void*          stream)
{
	const WriteContext context = WRITE_CONTEXT_NULL;
	SerdWriter         writer  = malloc(sizeof(struct SerdWriterImpl));
	writer->syntax     = syntax;
	writer->style      = style;
	writer->env        = env;
	writer->base_uri   = base_uri ? *base_uri : SERD_URI_NULL;
	writer->anon_stack = serd_stack_new(sizeof(WriteContext));
	writer->sink       = sink;
	writer->stream     = stream;
	writer->context    = context;
	writer->indent     = 0;
	return writer;
}

SERD_API
void
serd_writer_set_base_uri(SerdWriter     writer,
                         const SerdURI* uri)
{
	writer->base_uri = *uri;
	if (writer->syntax != SERD_NTRIPLES) {
		if (writer->context.graph.buf || writer->context.subject.buf) {
			writer->sink(" .\n\n", 4, writer->stream);
			writer->context = WRITE_CONTEXT_NULL;
		}
		writer->sink("@base <", 7, writer->stream);
		serd_uri_serialise(uri, writer->sink, writer->stream);
		writer->sink("> .\n", 4, writer->stream);
	}
	writer->context = WRITE_CONTEXT_NULL;
}

SERD_API
bool
serd_writer_set_prefix(SerdWriter      writer,
                       const SerdNode* name,
                       const SerdNode* uri)
{
	if (writer->syntax != SERD_NTRIPLES) {
		if (writer->context.graph.buf || writer->context.subject.buf) {
			writer->sink(" .\n\n", 4, writer->stream);
			writer->context = WRITE_CONTEXT_NULL;
		}
		writer->sink("@prefix ", 8, writer->stream);
		writer->sink(name->buf, name->n_bytes - 1, writer->stream);
		writer->sink(": <", 3, writer->stream);
		write_text(writer, WRITE_URI, uri->buf, uri->n_bytes - 1, '>');
		writer->sink("> .\n", 4, writer->stream);
	}
	writer->context = WRITE_CONTEXT_NULL;
	return true;
}

SERD_API
void
serd_writer_free(SerdWriter writer)
{
	SerdWriter const me = (SerdWriter)writer;
	serd_writer_finish(me);
	serd_stack_free(&writer->anon_stack);
	free(me);
}
