/*
  Copyright 2012-2015 David Robillard <http://drobilla.net>

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

#include "lv2/core/lv2.h"
#include "lv2/urid/sid.h"

#include <serd/serd.h>

#include <assert.h>

int n_errors = 0;

static int
on_node(const SerdEnv* env, const SerdNode* node)
{
	const SerdNode* base_uri     = serd_env_get_base_uri(env);
	const char*     base_uri_str = serd_node_get_string(base_uri);
	const size_t    base_uri_len = serd_node_get_length(base_uri);

	if (serd_node_get_type(node) == SERD_URI ||
	    serd_node_get_type(node) == SERD_CURIE) {

		SerdNode* expanded = serd_env_expand(env, node);
		if (!expanded) {
			fprintf(stderr,
			        "error: Failed to expand <%s>\n",
			        serd_node_get_string(node));
			return 1;
		}

		assert(serd_node_get_type(expanded) == SERD_URI);

		const char* uri = serd_node_get_string(expanded);
		if (!strcmp(uri, base_uri_str)) {
			return 0; // Ignore ontology itself
		} else if (strncmp(uri, base_uri_str, base_uri_len)) {
			fprintf(stderr,
			        "warning: Subject <%s> not within prefix <%s>\n",
			        uri,
			        serd_node_get_string(base_uri));
			return 0;
		}

		const LV2_URID id  = lv2_urid_static_map(uri);
		const char*    out = lv2_urid_static_unmap(id);

		if (!id) {
			fprintf(stderr, "error: Failed to map <%s>\n", uri);
			return 1; // SERD_ERR_INTERNAL;
		}

		assert(out);
		if (strcmp(uri, out)) {
			fprintf(stderr,
			        "error: <%s> (%d) unmapped to <%s>\n",
			        uri,
			        id,
			        out);
			return 1; // SERD_ERR_INTERNAL;
		}

		printf("OK %s %s\n", uri, out);
		serd_node_free(expanded);
	}

	return 0;
}

static SerdStatus
on_statement(void*                handle,
             SerdStatementFlags   flags,
             const SerdStatement* statement)
{
	(void)flags;

	const SerdEnv* env = (const SerdEnv*)handle;

	n_errors += on_node(env, serd_statement_get_subject(statement));
	/* on_node(env, serd_statement_get_predicate(statement)); */
	/* on_node(env, serd_statement_get_object(statement)); */

	return SERD_SUCCESS;
}

int
main(int argc, char** argv)
{
	if (argc != 3) {
		fprintf(stderr, "Usage: %s SCHEMA SCHEMA_URI\n", argv[0]);
		return 1;
	}

	const char* const filename   = argv[1];
	const char* const schema_uri = argv[2];

	SerdWorld*  world    = serd_world_new();
	SerdNode*   base_uri = serd_new_uri(schema_uri);
	SerdEnv*    env      = serd_env_new(base_uri);
	SerdSink*   sink     = serd_sink_new(env, env);
	SerdReader* reader   = serd_reader_new(world, SERD_TURTLE, sink, 1 << 20);

	serd_env_set_base_uri(env, base_uri);
	serd_sink_set_statement_func(sink, on_statement);

	SerdStatus st = SERD_SUCCESS;
	if ((st = serd_reader_start_file(reader, filename, true))) {
		fprintf(stderr, "error: %s\n", serd_strerror(st));
		return st;
	} else if ((st = serd_reader_read_document(reader))) {
		fprintf(stderr, "error: %s\n", serd_strerror(st));
		return st;
	}

	serd_node_free(base_uri);
	serd_reader_free(reader);
	serd_sink_free(sink);
	serd_env_free(env);
	serd_node_free(base_uri);
	serd_world_free(world);

	return n_errors;
}
