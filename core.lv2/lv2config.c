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

#define _XOPEN_SOURCE 500

#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <dirent.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <unistd.h>
#include <wordexp.h>

#include "serd-0.1.0.h"

#include "lv2-config.h"

#define NS_RDF "http://www.w3.org/1999/02/22-rdf-syntax-ns#"
#define NS_LV2 "http://lv2plug.in/ns/lv2core#"

typedef struct _Spec {
	SerdNode      uri;
	SerdNode      manifest;
} Spec;

typedef struct {
	SerdReader     reader;
	SerdReadState  state;
	const uint8_t* current_file;
	Spec*          specs;
	size_t         n_specs;
} World;

/* Append a discovered specification to world->specs. */
void
add_spec(World*         world,
         SerdNode*      uri,
         const uint8_t* manifest)
{
	world->specs = realloc(world->specs, sizeof(Spec) * (world->n_specs + 1));
	world->specs[world->n_specs].uri = *uri;
	world->specs[world->n_specs].manifest = serd_node_from_string(
		SERD_URI, (const uint8_t*)strdup((const char*)manifest));
	++world->n_specs;
}

/** Free world->specs. */
void
free_specs(World* world)
{
	for (size_t i = 0; i < world->n_specs; ++i) {
		Spec* spec = &world->specs[i];
		serd_node_free(&spec->uri);
		serd_node_free(&spec->manifest);
	}
	free(world->specs);
	world->specs   = NULL;
	world->n_specs = 0;
}

/** Reader @base directive handler. */
bool
on_base(void*           handle,
        const SerdNode* uri_node)
{
	World* const world = (World*)handle;
	return serd_read_state_set_base_uri(world->state, uri_node);
}

/** Reader @prefix directive handler. */
static bool
on_prefix(void*           handle,
          const SerdNode* name,
          const SerdNode* uri_node)
{
	World* const world = (World*)handle;
	return serd_read_state_set_prefix(world->state, name, uri_node);
}

/** Reader statement handler. */
static bool
on_statement(void*           handle,
             const SerdNode* graph,
             const SerdNode* subject,
             const SerdNode* predicate,
             const SerdNode* object,
             const SerdNode* object_datatype,
             const SerdNode* object_lang)
{
	World*        world = (World*)handle;
	SerdReadState state = world->state;
	SerdNode      abs_s = serd_read_state_expand(state, subject);
	SerdNode      abs_p = serd_read_state_expand(state, predicate);
	SerdNode      abs_o = serd_read_state_expand(state, object);

	if (abs_s.buf && abs_p.buf && abs_o.buf
	    && !strcmp((const char*)abs_p.buf, NS_RDF "type")
	    && !strcmp((const char*)abs_o.buf, NS_LV2 "Specification")) {
		add_spec(world, &abs_s, world->current_file);
	} else {
		serd_node_free(&abs_s);
	}
	serd_node_free(&abs_p);
	serd_node_free(&abs_o);
	return true;
}

/** Add any specifications found in a manifest.ttl to world->specs. */
static void
scan_manifest(World* world, const char* uri)
{
	SerdEnv env = serd_env_new();

	world->state = serd_read_state_new(env, (const uint8_t*)uri);

	const char* const path = uri + strlen("file://");
	FILE*             fd   = fopen(path, "r");
	if (fd) {
		world->current_file = (const uint8_t*)uri;
		if (!serd_reader_read_file(world->reader, fd, (const uint8_t*)uri)) {
			fprintf(stderr, "lv2config: error reading <%s>\n", path);
		}
		world->current_file = NULL;
		fclose(fd);
	} else {
		fprintf(stderr, "lv2config: failed to open <%s>\n", path);
	}

	serd_read_state_free(world->state);
	serd_env_free(env);
	world->state = NULL;
}

/** Expand variables, ~, etc. in path. */
static char*
expand(const char* path)
{
	char*     ret = NULL;
	wordexp_t p;

	wordexp(path, &p, 0);
	if (p.we_wordc == 0) {
		/* Literal directory path (e.g. no variables or ~) */
		ret = strdup(path);
	} else if (p.we_wordc == 1) {
		/* Directory path expands (e.g. contains ~ or $FOO) */
		ret = strdup(p.we_wordv[0]);
	} else {
		/* Multiple expansions in a single directory path? */
		fprintf(stderr, "lv2config: malformed path `%s' ignored\n", path);
	}

	wordfree(&p);
	return ret;
}

/** Scan all bundles in path (i.e. scan all path/foo.lv2/manifest.ttl). */
void
scan_dir(World* world, const char* path)
{
	char* full_path = expand(path);
	if (!full_path) {
		return;
	}

	DIR* dir = opendir(full_path);
	if (!dir) {
		free(full_path);
		return;
	}

	struct dirent* file;
	while ((file = readdir(dir))) {
		if (!strcmp(file->d_name, ".") || !strcmp(file->d_name, "..")) {
			continue;
		}

		char* uri = malloc(strlen("file://")
		                   + strlen(full_path) + 1
		                   + strlen(file->d_name) + 1
		                   + strlen("manifest.ttl") + 1);

		sprintf(uri, "file://%s/%s/manifest.ttl",
		        full_path, file->d_name);

		scan_manifest(world, uri);
		free(uri);
	}

	closedir(dir);
	free(full_path);
}

/** Create all parent directories of dir_path, but not dir_path itself. */
int
mkdir_parents(const char* dir_path)
{
	char*        path     = strdup(dir_path);
	const size_t path_len = strlen(path);
	size_t       last_sep = 0;
	for (size_t i = 1; i <= path_len; ++i) {
		if (path[i] == LV2CORE_DIR_SEP[0]) {
			path[i] = '\0';
			if (mkdir(path, 0755) && errno != EEXIST) {
				fprintf(stderr, "lv2config: Failed to create %s (%s)\n",
				        path, strerror(errno));
				free(path);
				return 1;
			}
			path[i] = LV2CORE_DIR_SEP[0];
			last_sep = i;
		}
	}

	free(path);
	return 0;
}

/** Return the output include dir based on path (prepend DESTDIR). */
char*
output_dir(const char* path)
{
	char* destdir = getenv("DESTDIR");
	if (destdir) {
		size_t len = strlen(destdir) + strlen(path);
		char*  ret = malloc(len + 1);
		snprintf(ret, len + 1, "%s%s", destdir, path);
		return ret;
	} else {
		return strdup(path);
	}
}

/** Build an LV2 include tree in dest for all bunles in lv2_path. */
void
build_tree(World* world, const char* lv2_path, const char* dest)
{
	free_specs(world);

	/* Call scan_dir for each component of lv2_path,
	   which will build world->specs (a linked list of struct Spec).
	*/
	while (lv2_path[0] != '\0') {
		const char* const sep = strchr(lv2_path, LV2CORE_PATH_SEP[0]);
		if (sep) {
			const size_t dir_len = sep - lv2_path;
			char* const  dir     = malloc(dir_len + 1);
			memcpy(dir, lv2_path, dir_len);
			dir[dir_len] = '\0';
			scan_dir(world, dir);
			free(dir);
			lv2_path += dir_len + 1;
		} else {
			scan_dir(world, lv2_path);
			lv2_path = "\0";
		}
	}

	/* TODO: Check revisions */

	/* Make a link in the include tree for each specification bundle. */
	for (size_t i = 0; i < world->n_specs; ++i) {
		Spec*       spec = &world->specs[i];
		const char* path = strchr((const char*)spec->uri.buf, ':');
		if (!path) {
			fprintf(stderr, "lv2config: Invalid URI <%s>\n", spec->uri.buf);
			continue;
		}
		for (++path; (path[0] == '/' && path[0] != '\0'); ++path) {}

		const char* bundle_uri  = (const char*)spec->manifest.buf;
		char*       bundle_path = strdup(bundle_uri + strlen("file://"));
		char*       last_sep    = strrchr(bundle_path, LV2CORE_DIR_SEP[0]);
		if (last_sep) {
			*(last_sep + 1) = '\0';
		}

		char*  full_dest    = output_dir(dest);
		size_t len          = strlen(full_dest) + 1 + strlen(path);
		char*  rel_inc_path = malloc(len + 1);
		snprintf(rel_inc_path, len + 1, "%s/%s", full_dest, path);
		free(full_dest);

		char* inc_path = expand(rel_inc_path);
		free(rel_inc_path);
		printf("%s => %s\n", inc_path, bundle_path);

		if (!mkdir_parents(inc_path)) {
			if (!access(inc_path, F_OK) && unlink(inc_path)) {
				fprintf(stderr, "lv2config: Failed to remove %s (%s)\n",
				        inc_path, strerror(errno));
				free(inc_path);
				free(bundle_path);
				continue;
			}

			if (symlink(bundle_path, inc_path)) {
				fprintf(stderr, "lv2config: Failed to create link (%s)\n",
				        strerror(errno));
			}

			free(inc_path);
			free(bundle_path);
		}
	}
}

int
usage(const char* name, bool error)
{
	FILE* out = (error ? stderr : stdout);
	fprintf(out, "Usage: %s\n", name);
	fprintf(out, "Build the default system LV2 include directories.\n\n");
	fprintf(out, "Usage: %s INCLUDE_DIR\n", name);
	fprintf(out, "Build an LV2 include directory tree at INCLUDE_DIR\n");
	fprintf(out, "for all extensions found in $LV2_PATH.\n\n");
	fprintf(out, "Usage: %s INCLUDE_DIR BUNDLES_DIR\n", name);
	fprintf(out, "Build an lv2 include directory tree at INCLUDE_DIR\n");
	fprintf(out, "for all extensions found in bundles under BUNDLES_DIR.\n");
	return (error ? EXIT_FAILURE : EXIT_SUCCESS);
}

int
main(int argc, char** argv)
{
	World world = { NULL, NULL, NULL, NULL, 0 };
	world.reader = serd_reader_new(
		SERD_TURTLE, &world, on_base, on_prefix, on_statement, NULL);

	if (argc == 1) {
		/* lv2_config */
		build_tree(&world, "/usr/local/lib/lv2", "/usr/local/include/lv2");
		build_tree(&world, "/usr/lib/lv2",       "/usr/include/lv2");
	} else if (argv[1][0] == '-') {
		return usage(argv[0], false);
	} else if (argc == 2) {
		/* lv2_config INCLUDE_DIR */
		const char* lv2_path = getenv("LV2_PATH");
		if (!lv2_path) {
			lv2_path = LV2CORE_DEFAULT_LV2_PATH;
		}
		build_tree(&world, lv2_path, argv[1]);
	} else if (argc == 3) {
		/* lv2_config INCLUDE_DIR BUNDLES_DIR */
		build_tree(&world, argv[2], argv[1]);
	} else {
		return usage(argv[0], true);
	}

	free_specs(&world);
	serd_reader_free(world.reader);

	return 0;
}
