/*
  Copyright 2010-2011 David Robillard <d@drobilla.net>

  Permission is hereby granted, free of charge, to any person obtaining a
  copy of this software and associated documentation files (the "Software"),
  to deal in the Software without restriction, including without limitation
  the rights to use, copy, modify, merge, publish, distribute, sublicense,
  and/or sell copies of the Software, and to permit persons to whom the
  Software is furnished to do so, subject to the following conditions:

  The above copyright notice and this permission notice shall be included
  in all copies or substantial portions of the Software.

  THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
  IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
  FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL
  THE AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR
  OTHER LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE,
  ARISING FROM, OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR
  OTHER DEALINGS IN THE SOFTWARE.
*/

/**
   @file files.h
   C API for the LV2 Files extension <http://lv2plug.in/ns/ext/files>.
*/

#ifndef LV2_FILES_H
#define LV2_FILES_H

#ifdef __cplusplus
extern "C" {
#endif

#define LV2_FILES_URI "http://lv2plug.in/ns/ext/files"

typedef void* LV2_Files_FileSupport_Data;

/**
   files:FileSupport feature struct.
   
   To support this feature, the host MUST pass an LV2_Feature struct with @a
   URI "http://lv2plug.in/ns/ext/files#fileSupport" and @ data pointed to an
   instance of this struct.
*/
typedef struct {

	/**
	   Opaque host data.
	*/
	LV2_Files_FileSupport_Data data;
	
	/**
	   Return the full path that should be used for a file owned by this
	   plugin called @a name. The plugin can assume @a name belongs to a
	   namespace dedicated to that plugin instance (i.e. hosts MUST ensure
	   this, e.g. by giving each plugin instance its own files directory).
	 
	   @param data MUST be the @a data member of this struct.
	   @param name The name of the file.
	   @return A newly allocated path which the plugin may use to create a new
	   file. The plugin is responsible for freeing the returned string.
	*/
	char* new_file_path(LV2_Files_FileSupport_Data data,
	                    const char*                name);
	
} LV2_Files_FileSupport;

#ifdef __cplusplus
} /* extern "C" */
#endif

#endif /* LV2_FILES_H */
