/* Dynamic manifest specification for LV2
 * Revision 1
 *
 * Copyright (C) 2008, 2009 Stefano D'Angelo <zanga.mail@gmail.com>
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 * 1. Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 * 2. Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 * 3. The name of the author may not be used to endorse or promote products
 *    derived from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE AUTHOR ``AS IS'' AND ANY EXPRESS OR
 * IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO, THE IMPLIED WARRANTIES
 * OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.
 * IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR ANY DIRECT, INDIRECT,
 * INCIDENTAL, SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT
 * NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE,
 * DATA, OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY
 * THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT
 * (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF
 * THIS SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

#ifndef LV2_DYN_MANIFEST_H_INCLUDED
#define LV2_DYN_MANIFEST_H_INCLUDED

#include <stdio.h>
#include "lv2.h"

#ifdef __cplusplus
extern "C" {
#endif


/* ************************************************************************* */


/** @file
 * C header for the LV2 Dynamic Manifest extension
 * <http://lv2plug.in/ns/ext/dyn-manifest>.
 * Revision: 1
 *
 * == Overview ==
 *
 * The LV2 API, on its own, cannot be used to write plugin libraries where
 * data is dynamically generated at runtime (e.g. API wrappers), since LV2
 * requires needed information to be provided in one or more static data (RDF)
 * files. This API addresses this limitation by extending the LV2 API.
 *
 * A host implementing support for this API should first detect that the plugin
 * library implements a dynamic manifest generator by examining its static
 * manifest file, then fetch data from the shared object file by accessing it as
 * usual (dlopen() and family) and using this API.
 *
 * The host is allowed to request regeneration of the dynamic manifest multiple
 * times, and the plugin library is expected to provide updated data if/when
 * possible. All data and references provided via this API before the last
 * regeneration of the dynamic manifest is to be considered invalid by the
 * host, including plugin descriptors whose URIs were discovered using this API.
 *
 * This API is extensible in a similar fashion as the LV2 plugin API.
 *
 * == Threading rules ==
 *
 * This specification defines threading rule classes, similarly to the LV2
 * specification.
 *
 * The functions defined by this API belong to:
 *
 *  - Dynamic manifest open class:  lv2_dyn_manifest_open()
 *  - Dynamic manifest close class: lv2_dyn_manifest_close()
 *  - Dynamic manifest file class:  lv2_dyn_manifest_get_subjects(),
 *                                  lv2_dyn_manifest_get_data()
 *
 * The rules that hosts must follow are these:
 *
 *  - When a function from the Dynamic manifest open or the Dynamic manifest
 *    close class is running, no other functions in the same shared object file
 *    may run.
 *  - When a function from the Dynamic manifest file class is called, no other
 *    functions from the same class may run if they are given at least one
 *    FILE * argument with the same value.
 *  - A function from the Dynamic manifest open class may not run after a
 *    successful call to a function from the same class, in case a function from
 *    the Dynamic manifest close class was not successfully called in the
 *    meanwhile.
 *  - A function from the Dynamic manifest close class may only run after a
 *    successful call to a function from the Dynamic manifest open class.
 *  - A function from the Dynamic manifest file class may only run beetween a
 *    successful call to a function from the Dynamic manifest open class and the
 *    following successful call to a function from the Dynamic manifest close
 *    class.
 *
 * Extensions to this specification which add new functions MUST declare in
 * which of these classes the functions belong, or define new classes for them;
 * furthermore, classes defined by such extensions MUST only allow calls after
 * a successful call to a function from the Dynamic manifest open class and
 * before the following successful call to a function from the Dynamic manifest
 * close class.
 *
 * Any simultaneous calls that are not explicitly forbidden by these rules are
 * allowed.
 */


/* ************************************************************************* */


/** Dynamic manifest generator handle.
 *
 * This handle indicates a particular status of a dynamic manifest generator.
 * The host MUST NOT attempt to interpret it and, unlikely LV2_Handle, it is NOT
 * even valid to compare this to NULL. The dynamic manifest generator may use it
 * to reference internal data. */
typedef void * LV2_Dyn_Manifest_Handle;


/* ************************************************************************* */


/** Accessing data.
 *
 * Whenever a host wants to access data using this API, it could:
 *
 *  1. Call lv2_dyn_manifest_open();
 *  2. Create an empty resource identified by a FILE *;
 *  3. Get a "list" of exposed subject URIs using
 *     lv2_dyn_manifest_get_subjects();
 *  4. Call lv2_dyn_manifest_get_data() for each URI of interest, in order to
 *     get data related to that URI (either by calling the function subsequently
 *     with the same FILE * resource, or by creating more FILE * resources to
 *     perform parallel calls);
 *  5. Call lv2_dyn_manifest_close();
 *  6. Parse the content of the FILE * resource(s).
 *  7. Free/delete/unlink the FILE * resource(s).
 *
 * The content of the FILE * resources has to be interpreted by the host as a
 * regular file in Turtle syntax. This also means that each FILE * resource
 * should also contain needed prefix definitions, in case any are used.
 *
 * Each call to lv2_dyn_manifest_open() automatically implies the (re)generation
 * of the dynamic manifest on the library side.
 *
 * When such calls are made, data fetched from the involved library using this
 * API before such call is to be considered no more valid.
 *
 * In case the library uses this same API to access other dynamic manifests, it
 * MUST implement some mechanism to avoid potentially endless loops (such as A
 * loads B, B loads A, etc.) in functions from the Dynamic manifest open class
 * (the open-like operation MUST fail). For this purpose, use of a static
 * boolean flag is suggested.
 */

/** Function that (re)generates the dynamic manifest.
 *
 * handle is a pointer to an uninitialized dynamic manifest generator handle.
 *
 * features is a NULL terminated array of LV2_Feature structs which
 * represent the features the host supports. The dynamic manifest geenrator may
 * refuse to (re)generate the dynamic manifest if required features are not
 * found here (however hosts SHOULD NOT use this as a discovery mechanism,
 * instead of reading the static manifest file). This array must always exist;
 * if a host has no features, it MUST pass a single element array containing
 * NULL.
 *
 * This function MUST return 0 on success, otherwise a non-zero error code, and
 * the host SHOULD evaluate the result of the operation by examining the
 * returned value, rather than try to interpret the value of handle.
 */
int lv2_dyn_manifest_open(LV2_Dyn_Manifest_Handle *  handle,
                          const LV2_Feature *const * features);

/** Function that fetches a "list" of subject URIs exposed by the dynamic
 *  manifest generator.
 *
 * handle is the dynamic manifest generator handle.
 *
 * fp is the FILE * identifying the resource the host has to set up for the
 * dynamic manifest generator. The host MUST pass a writable, empty resource to
 * this function, and the dynamic manifest generator MUST ONLY perform write
 * operations on it at the end of the stream (e.g. use only fprintf(), fwrite()
 * and similar).
 *
 * The dynamic manifest generator has to fill the resource only with the needed
 * triples to make the host aware of the "objects" it wants to expose. For
 * example, if the library exposes a regular LV2 plugin, it should output only a
 * triple like the following:
 *
 *   <http://www.example.com/plugin/uri> a lv2:Plugin;
 *
 * This function MUST return 0 on success, otherwise a non-zero error code.
 */
int lv2_dyn_manifest_get_subjects(LV2_Dyn_Manifest_Handle handle,
                                  FILE *                  fp);

/** Function that fetches data related to a specific URI.
 *
 * handle is the dynamic manifest generator handle.
 *
 * fp is the FILE * identifying the resource the host has to set up for the
 * dynamic manifest generator. The host MUST pass a writable resource to this
 * function, and the dynamic manifest generator MUST ONLY perform write
 * operations on it at the current position of the stream (e.g. use only
 * fprintf(), fwrite() and similar).
 *
 * uri is the URI to get data about (in the "plain" form, a.k.a. without RDF
 * prefixes).
 *
 * The dynamic manifest generator has to fill the resource with data related to
 * the URI. For example, if the library exposes a regular LV2 plugin whose URI,
 * as retrieved by the host using lv2_dyn_manifest_get_subjects() is
 * http://www.example.com/plugin/uri, it should output something like:
 *
 *   <http://www.example.com/plugin/uri> a lv2:Plugin;
 *       lv2:binary <mylib.so>;
 *       doap:name "My Plugin";
 *       ... etc...
 *
 * This function MUST return 0 on success, otherwise a non-zero error code.
 */
int lv2_dyn_manifest_get_data(LV2_Dyn_Manifest_Handle handle,
                              FILE *                  fp,
                              const char *            uri);

/** Function that ends the operations on the dynamic manifest generator.
 *
 * handle is the dynamic manifest generator handle.
 *
 * This function should be used by the dynamic manifest generator to perform
 * cleanup operations, etc.
 */
void lv2_dyn_manifest_close(LV2_Dyn_Manifest_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* LV2_DYN_MANIFEST_H_INCLUDED */

