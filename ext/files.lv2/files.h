/* lv2_files.h - C header file for the LV2 Files extension.
 * Copyright (C) 2010 Leonard Ritter <paniq@paniq.org>
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
 * C header for the LV2 Files extension <http://lv2plug.in/ns/ext/files>.
 */

#ifndef LV2_FILES_H
#define LV2_FILES_H

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_FILES_URI "http://lv2plug.in/ns/ext/files"

typedef void* LV2_Files_FileSupport_Data;

/** Feature structure passed by host to instantiate with feature URI
 * <http://lv2plug.in/ns/ext/files#fileSupport>.
 */
typedef struct {

	LV2_Files_FileSupport_Data data;
	
	/** Return the full path that should be used for a file owned by this
	 * plugin called @a name.  The plugin can assume @a name belongs to a
	 * namespace dedicated to that plugin instance (i.e. hosts MUST ensure
	 * this, e.g. by giving each plugin its own directory for files, or
	 * mangling filenames somehow).
	 *
	 * @param data MUST be the @a data member of this struct.
	 * @param name The name of the file.
	 * @return A newly allocated path which the plugin may use to create a new
	 *         file.  The plugin is responsible for freeing the returned string.
	 */
	char* new_file_path(LV2_Files_FileSupport_Data data,
	                    const char*                name);
	
} LV2_Files_FileSupport;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_FILES_H */
