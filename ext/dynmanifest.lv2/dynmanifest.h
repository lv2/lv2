/* Dynamic manifest specification for LV2
 * Revision 1.1
 *
 * Copyright (C) 2008-2011 Stefano D'Angelo <zanga.mail@gmail.com>
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

#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#define LV2_DYN_MANIFEST_URI "http://lv2plug.in/ns/ext/dynmanifest"

#ifdef __cplusplus
extern "C" {
#endif


/* ************************************************************************* */


/** @file
 * C header for the LV2 Dynamic Manifest extension
 * <http://lv2plug.in/ns/ext/dynmanifest>.
 * Revision: 1.1
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
 * usual (e.g., dlopen() and family) and using this API.
 *
 * The host is allowed to request regeneration of the dynamic manifest multiple
 * times, and the plugin library is expected to provide updated data if/when
 * possible. All data and references provided via this API before the last
 * regeneration of the dynamic manifest is to be considered invalid by the
 * host, including plugin descriptors whose URIs were discovered using this API.
 *
 * This API is extensible in a similar fashion as the LV2 API.
 *
 * == Accessing data ==
 *
 * Whenever a host wants to access data using this API, it could:
 *
 *  -# Call lv2_dyn_manifest_open();
 *  -# Create an empty resource identified by a FILE *;
 *  -# Get a "list" of exposed subject URIs using
 *     lv2_dyn_manifest_get_subjects();
 *  -# Call lv2_dyn_manifest_get_data() for each URI of interest, in order to
 *     get data related to that URI (either by calling the function subsequently
 *     with the same FILE * resource, or by creating more FILE * resources to
 *     perform parallel calls);
 *  -# Call lv2_dyn_manifest_close();
 *  -# Parse the content of the FILE * resource(s).
 *  -# Free/delete/unlink the FILE * resource(s).
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
 * In case the plugin library uses this same API to access other dynamic
 * manifests, it MUST implement some mechanism to avoid potentially endless
 * loops (such as A loads B, B loads A, etc.) and, in case such a loop is
 * detected, the operation MUST fail. For this purpose, use of a static boolean
 * flag is suggested.
 * 
 * == Threading rules ==
 *
 * All of the functions defined by this specification belong to the Discovery
 * class.
 *
 * Extensions to this specification which add new functions MUST declare in
 * which classes the functions belong, or define new classes for them.
 */


/* ************************************************************************* */


/** Dynamic manifest generator handle.
 *
 * This handle indicates a particular status of a dynamic manifest generator.
 * The host MUST NOT attempt to interpret it and, unlikely LV2_Handle, it is NOT
 * even valid to compare this to NULL. The dynamic manifest generator MAY use it
 * to reference internal data. */
typedef void * LV2_Dyn_Manifest_Handle;


/* ************************************************************************* */


/** Function that (re)generates the dynamic manifest.
 *
 * @param handle Pointer to an uninitialized dynamic manifest generator handle.
 *
 * @param features NULL terminated array of LV2_Feature structs which
 * represent the features the host supports. The dynamic manifest geenrator may
 * refuse to (re)generate the dynamic manifest if required features are not
 * found here (however hosts SHOULD NOT use this as a discovery mechanism,
 * instead of reading the static manifest file). This array must always exist;
 * if a host has no features, it MUST pass a single element array containing
 * NULL.
 *
 * @return 0 on success, otherwise a non-zero error code. The host SHOULD
 * evaluate the result of the operation by examining the returned value and MUST
 * NOT try to interpret the value of handle.
 */
int lv2_dyn_manifest_open(LV2_Dyn_Manifest_Handle *  handle,
                          const LV2_Feature *const * features);

/** Function that fetches a "list" of subject URIs exposed by the dynamic
 *  manifest generator.
 *
 * The dynamic manifest generator has to fill the resource only with the needed
 * triples to make the host aware of the "objects" it wants to expose. For
 * example, if the plugin library exposes a regular LV2 plugin, it should output
 * only a triple like the following:
 *
 *   <http://www.example.com/plugin/uri> a lv2:Plugin .
 *
 * The objects that are elegible for exposure are those that would need to be
 * represented by a subject node in a static manifest.
 *
 * @param handle Dynamic manifest generator handle.
 *
 * @param fp FILE * identifying the resource the host has to set up for the
 * dynamic manifest generator. The host MUST pass a writable, empty resource to
 * this function, and the dynamic manifest generator MUST ONLY perform write
 * operations on it at the end of the stream (e.g., using only fprintf(),
 * fwrite() and similar).
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int lv2_dyn_manifest_get_subjects(LV2_Dyn_Manifest_Handle handle,
                                  FILE *                  fp);

/** Function that fetches data related to a specific URI.
 *
 * The dynamic manifest generator has to fill the resource with data related to
 * object represented by the given URI. For example, if the library exposes a
 * regular LV2 plugin whose URI, as retrieved by the host using
 * lv2_dyn_manifest_get_subjects() is http://www.example.com/plugin/uri, it
 * should output something like:
 *
 *   <http://www.example.com/plugin/uri> a lv2:Plugin ;
 *       doap:name "My Plugin" ;
 *       lv2:binary <mylib.so> ;
 *       ... etc...
 *
 * @param handle Dynamic manifest generator handle.
 *
 * @param fp FILE * identifying the resource the host has to set up for the
 * dynamic manifest generator. The host MUST pass a writable resource to this
 * function, and the dynamic manifest generator MUST ONLY perform write
 * operations on it at the current position of the stream (e.g. using only
 * fprintf(), fwrite() and similar).
 *
 * @param uri URI to get data about (in the "plain" form, i.e., absolute URI
 * without Turtle prefixes).
 *
 * @return 0 on success, otherwise a non-zero error code.
 */
int lv2_dyn_manifest_get_data(LV2_Dyn_Manifest_Handle handle,
                              FILE *                  fp,
                              const char *            uri);

/** Function that ends the operations on the dynamic manifest generator.
 * 
 * This function SHOULD be used by the dynamic manifest generator to perform
 * cleanup operations, etc.
 *
 * Once this function is called, referring to handle will cause undefined
 * behavior.
 *
 * @param handle Dynamic manifest generator handle.
 */
void lv2_dyn_manifest_close(LV2_Dyn_Manifest_Handle handle);

#ifdef __cplusplus
}
#endif

#endif /* LV2_DYN_MANIFEST_H_INCLUDED */

