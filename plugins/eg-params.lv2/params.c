/*
  LV2 Parameter Example Plugin
  Copyright 2014-2015 David Robillard <d@drobilla.net>

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

#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#ifndef __cplusplus
#    include <stdbool.h>
#endif

#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/atom/util.h"
#include "lv2/lv2plug.in/ns/ext/patch/patch.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/lv2core/lv2.h"

#include "lv2/lv2plug.in/ns/ext/log/logger.h"
#include "lv2/lv2plug.in/ns/ext/midi/midi.h"
#include "lv2/lv2plug.in/ns/ext/state/state.h"

#define MAX_STRING 1024

#define EG_PARAMS_URI    "http://lv2plug.in/plugins/eg-params"
#define EG_PARAMS__float EG_PARAMS_URI "#float"

typedef struct {
	LV2_URID plugin;
	LV2_URID atom_Path;
	LV2_URID atom_Sequence;
	LV2_URID atom_URID;
	LV2_URID atom_eventTransfer;
	LV2_URID eg_int;
	LV2_URID eg_long;
	LV2_URID eg_float;
	LV2_URID eg_double;
	LV2_URID eg_bool;
	LV2_URID eg_string;
	LV2_URID eg_path;
	LV2_URID eg_lfo;
	LV2_URID eg_spring;
	LV2_URID midi_Event;
	LV2_URID patch_Get;
	LV2_URID patch_Set;
	LV2_URID patch_Put;
	LV2_URID patch_body;
	LV2_URID patch_subject;
	LV2_URID patch_property;
	LV2_URID patch_value;
} URIs;

typedef struct {
	LV2_Atom_Int    aint;
	LV2_Atom_Long   along;
	LV2_Atom_Float  afloat;
	LV2_Atom_Double adouble;
	LV2_Atom_Bool   abool;
	LV2_Atom        astring;
	char            string[MAX_STRING];
	LV2_Atom        apath;
	char            path[MAX_STRING];
	LV2_Atom_Float  lfo;
	LV2_Atom_Float  spring;
} State;

static inline void
map_uris(LV2_URID_Map* map, URIs* uris)
{
	uris->plugin             = map->map(map->handle, EG_PARAMS_URI);

	uris->atom_Path          = map->map(map->handle, LV2_ATOM__Path);
	uris->atom_Sequence      = map->map(map->handle, LV2_ATOM__Sequence);
	uris->atom_URID          = map->map(map->handle, LV2_ATOM__URID);
	uris->atom_eventTransfer = map->map(map->handle, LV2_ATOM__eventTransfer);
	uris->eg_int             = map->map(map->handle, EG_PARAMS_URI "#int");
	uris->eg_long            = map->map(map->handle, EG_PARAMS_URI "#long");
	uris->eg_float           = map->map(map->handle, EG_PARAMS_URI "#float");
	uris->eg_double          = map->map(map->handle, EG_PARAMS_URI "#double");
	uris->eg_bool            = map->map(map->handle, EG_PARAMS_URI "#bool");
	uris->eg_string          = map->map(map->handle, EG_PARAMS_URI "#string");
	uris->eg_path            = map->map(map->handle, EG_PARAMS_URI "#path");
	uris->eg_lfo             = map->map(map->handle, EG_PARAMS_URI "#lfo");
	uris->eg_spring          = map->map(map->handle, EG_PARAMS_URI "#spring");
	uris->midi_Event         = map->map(map->handle, LV2_MIDI__MidiEvent);
	uris->patch_Get          = map->map(map->handle, LV2_PATCH__Get);
	uris->patch_Set          = map->map(map->handle, LV2_PATCH__Set);
	uris->patch_Put          = map->map(map->handle, LV2_PATCH__Put);
	uris->patch_body         = map->map(map->handle, LV2_PATCH__body);
	uris->patch_subject      = map->map(map->handle, LV2_PATCH__subject);
	uris->patch_property     = map->map(map->handle, LV2_PATCH__property);
	uris->patch_value        = map->map(map->handle, LV2_PATCH__value);
}

enum {
	PARAMS_IN  = 0,
	PARAMS_OUT = 1
};

typedef struct {
	// Features
	LV2_URID_Map*   map;
	LV2_URID_Unmap* unmap;
	LV2_Log_Log*    log;

	// Forge for creating atoms
	LV2_Atom_Forge forge;

	// Logger convenience API
	LV2_Log_Logger logger;

	// Ports
	const LV2_Atom_Sequence* in_port;
	LV2_Atom_Sequence*       out_port;

	// URIs
	URIs uris;

	// Plugin state
	State state;

	float spring;
	float lfo;
} Params;

static void
connect_port(LV2_Handle instance,
             uint32_t   port,
             void*      data)
{
	Params* self = (Params*)instance;
	switch (port) {
	case PARAMS_IN:
		self->in_port = (const LV2_Atom_Sequence*)data;
		break;
	case PARAMS_OUT:
		self->out_port = (LV2_Atom_Sequence*)data;
		break;
	default:
		break;
	}
}

static inline int
get_features(const LV2_Feature* const* features, ...)
{
	va_list args;
	va_start(args, features);

	const char* uri = NULL;
	while ((uri = va_arg(args, const char*))) {
		void** data     = va_arg(args, void**);
		bool   required = va_arg(args, int);
		bool   found    = false;

		for (int i = 0; features[i]; ++i) {
			if (!strcmp(features[i]->URI, uri)) {
				*data = features[i]->data;
				found = true;
				break;
			}
		}

		if (required && !found) {
			fprintf(stderr, "Missing required feature <%s>\n", uri);
			return 1;
		}
	}

	return 0;
}


#define INIT_PARAM(atype, param) { \
	(param)->atom.type = (atype); \
	(param)->atom.size = sizeof((param)->body); \
}

static LV2_Handle
instantiate(const LV2_Descriptor*     descriptor,
            double                    rate,
            const char*               path,
            const LV2_Feature* const* features)
{
	// Allocate instance
	Params* self = (Params*)calloc(1, sizeof(Params));
	if (!self) {
		return NULL;
	}

	// Get host features
	if (get_features(features,
	                 LV2_URID__map,   &self->map,   true,
	                 LV2_URID__unmap, &self->unmap, false,
	                 LV2_LOG__log,    &self->log,   false,
	                 NULL)) {
		free(self);
		return NULL;
	}

	// Map URIs and initialise forge/logger
	map_uris(self->map, &self->uris);
	lv2_atom_forge_init(&self->forge, self->map);
	lv2_log_logger_init(&self->logger, self->map, self->log);

	// Initialize state
	INIT_PARAM(self->forge.Int,    &self->state.aint);
	INIT_PARAM(self->forge.Long,   &self->state.along);
	INIT_PARAM(self->forge.Float,  &self->state.afloat);
	INIT_PARAM(self->forge.Double, &self->state.adouble);
	INIT_PARAM(self->forge.Bool,   &self->state.abool);
	INIT_PARAM(self->forge.Float,  &self->state.spring);
	INIT_PARAM(self->forge.Float,  &self->state.lfo);
	self->state.astring.type = self->forge.String;
	self->state.astring.size = 0;
	self->state.apath.type   = self->forge.Path;
	self->state.apath.size   = 0;

	return (LV2_Handle)self;
}

static void
cleanup(LV2_Handle instance)
{
	free(instance);
}

static LV2_State_Status
check_param(Params*  self,
            LV2_URID key,
            LV2_URID required_key,
            LV2_URID type,
            LV2_URID required_type)
{
	if (key == required_key) {
		if (type == required_type) {
			return LV2_STATE_SUCCESS;
		} else if (self->unmap) {
			lv2_log_trace(
				&self->logger, "Bad type <%s> for <%s> (needs <%s>)\n",
				self->unmap->unmap(self->unmap->handle, type),
				self->unmap->unmap(self->unmap->handle, key),
				self->unmap->unmap(self->unmap->handle, required_type));
		} else {
			lv2_log_trace(&self->logger, "Bad type for parameter %d\n", key);
		}
		return LV2_STATE_ERR_BAD_TYPE;
	}

	return LV2_STATE_ERR_NO_PROPERTY;
}

static LV2_State_Status
set_parameter(Params*     self,
              LV2_URID    key,
              uint32_t    size,
              LV2_URID    type,
              const void* body,
              bool        from_state)
{
	const URIs*           uris  = &self->uris;
	const LV2_Atom_Forge* forge = &self->forge;
	LV2_State_Status      st    = LV2_STATE_SUCCESS;

	if (!(st = check_param(self, key, uris->eg_int, type, forge->Int))) {
		self->state.aint.body = *(const int32_t*)body;
		lv2_log_trace(&self->logger, "Set int %d\n", self->state.aint.body);
	} else if (!(st = check_param(self, key, uris->eg_long, type, forge->Long))) {
		self->state.along.body = *(const int64_t*)body;
		lv2_log_trace(&self->logger, "Set long %ld\n", self->state.along.body);
	} else if (!(st = check_param(self, key, uris->eg_float, type, forge->Float))) {
		self->state.afloat.body = *(const float*)body;
		lv2_log_trace(&self->logger, "Set float %f\n", self->state.afloat.body);
	} else if (!(st = check_param(self, key, uris->eg_double, type, forge->Double))) {
		self->state.adouble.body = *(const double*)body;
		lv2_log_trace(&self->logger, "Set double %f\n", self->state.adouble.body);
	} else if (!(st = check_param(self, key, uris->eg_bool, type, forge->Bool))) {
		self->state.abool.body = *(const int32_t*)body;
		lv2_log_trace(&self->logger, "Set bool %d\n", self->state.abool.body);
	} else if (!(st = check_param(self, key, uris->eg_string, type, forge->String))) {
		if (size <= MAX_STRING) {
			memcpy(self->state.string, body, size);
			self->state.astring.size = size;
			lv2_log_trace(&self->logger, "Set string %s\n", self->state.string);
		} else {
			lv2_log_error(&self->logger, "Insufficient space for string\n");
			return LV2_STATE_ERR_NO_SPACE;
		}
	} else if (!(st = check_param(self, key, uris->eg_path, type, forge->Path))) {
		if (size <= MAX_STRING) {
			memcpy(self->state.path, body, size);
			self->state.apath.size = size;
			lv2_log_trace(&self->logger, "Set path %s\n", self->state.path);
		} else {
			lv2_log_error(&self->logger, "Insufficient space for path\n");
			return LV2_STATE_ERR_NO_SPACE;
		}
	} else if (!(st = check_param(self, key, uris->eg_spring, type, forge->Float))) {
		self->spring = *(const float*)body;
		lv2_log_trace(&self->logger, "Set spring %f\n", *(const float*)body);
	} else if (key == uris->eg_lfo) {
		if (!from_state) {
			st = LV2_STATE_ERR_UNKNOWN;
			lv2_log_error(&self->logger, "Attempt to set non-writable LFO\n");
		} else {
			self->state.lfo.body = *(const float*)body;
			lv2_log_trace(&self->logger, "Set LFO %f\n", self->state.lfo.body);
		}
	} else if (self->unmap) {
		st = LV2_STATE_ERR_NO_PROPERTY;
		lv2_log_trace(&self->logger, "Unknown parameter <%s>\n",
		              self->unmap->unmap(self->unmap->handle, key));
	} else {
		st = LV2_STATE_ERR_NO_PROPERTY;
		lv2_log_trace(&self->logger, "Unknown parameter %d\n", key);
	}

	return st;
}

static const LV2_Atom *
get_parameter(Params* self, LV2_URID key)
{
	const URIs* uris = &self->uris;

	if (key == uris->eg_int) {
		lv2_log_trace(&self->logger, "Get int %d\n", self->state.aint.body);
		return &self->state.aint.atom;
	} else if (key == uris->eg_long) {
		lv2_log_trace(&self->logger, "Get long %ld\n", self->state.along.body);
		return &self->state.along.atom;
	} else if (key == uris->eg_float) {
		lv2_log_trace(&self->logger, "Get float %f\n", self->state.afloat.body);
		return &self->state.afloat.atom;
	} else if (key == uris->eg_double) {
		lv2_log_trace(&self->logger, "Get double %f\n", self->state.adouble.body);
		return &self->state.adouble.atom;
	} else if (key == uris->eg_bool) {
		lv2_log_trace(&self->logger, "Get bool %d\n", self->state.abool.body);
		return &self->state.abool.atom;
	} else if (key == uris->eg_string) {
		lv2_log_trace(&self->logger, "Get string %s\n", self->state.string);
		return &self->state.astring;
	} else if (key == uris->eg_path) {
		lv2_log_trace(&self->logger, "Get path %s\n", self->state.path);
		return &self->state.apath;
	} else if (key == uris->eg_spring) {
		lv2_log_trace(&self->logger, "Get spring %f\n", self->state.spring.body);
		return &self->state.spring.atom;
	} else if (key == uris->eg_lfo) {
		lv2_log_trace(&self->logger, "Get LFO %f\n", self->state.lfo.body);
		return &self->state.lfo.atom;
	} else if (self->unmap) {
		lv2_log_trace(&self->logger, "Unknown parameter <%s>\n",
		              self->unmap->unmap(self->unmap->handle, key));
	} else {
		lv2_log_trace(&self->logger, "Unknown parameter %d\n", key);
	}

	return NULL;
}

static LV2_State_Status
write_param_to_forge(LV2_State_Handle handle,
                     uint32_t         key,
                     const void*      value,
                     size_t           size,
                     uint32_t         type,
                     uint32_t         flags)
{
	LV2_Atom_Forge* forge = handle;

	if (!lv2_atom_forge_key(forge, key) ||
	    !lv2_atom_forge_atom(forge, size, type) ||
	    !lv2_atom_forge_write(forge, value, size)) {
		return LV2_STATE_ERR_UNKNOWN;
	}

	return LV2_STATE_SUCCESS;
}

static void
store_param(Params*                  self,
            LV2_State_Status*        save_status,
            LV2_State_Store_Function store,
            LV2_State_Handle         handle,
            LV2_URID                 key,
            const LV2_Atom*          value)
{
	const LV2_State_Status st = store(handle,
	                                  key,
	                                  value + 1,
	                                  value->size,
	                                  value->type,
	                                  LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);

	if (!*save_status) {
		*save_status = st;
	}
}

static LV2_State_Status
store_state(Params*                  self,
            LV2_State_Store_Function store,
            LV2_State_Handle         handle,
            uint32_t                 flags,
            LV2_State_Map_Path*      map_path)
{
	const URIs*      uris  = &self->uris;
	const State*     state = &self->state;
	LV2_State_Status st    = LV2_STATE_SUCCESS;

	// Store simple properties
	store_param(self, &st, store, handle, uris->eg_int, &state->aint.atom);
	store_param(self, &st, store, handle, uris->eg_long, &state->along.atom);
	store_param(self, &st, store, handle, uris->eg_float, &state->afloat.atom);
	store_param(self, &st, store, handle, uris->eg_double, &state->adouble.atom);
	store_param(self, &st, store, handle, uris->eg_bool, &state->abool.atom);
	store_param(self, &st, store, handle, uris->eg_string, &state->astring);
	store_param(self, &st, store, handle, uris->eg_spring, &state->spring.atom);
	store_param(self, &st, store, handle, uris->eg_lfo, &state->lfo.atom);

	if (map_path) {
		// Map path to abstract path for portable storage
		char* apath = map_path->abstract_path(map_path->handle, state->path);
		st = store(handle,
		           self->uris.eg_path,
		           apath,
		           strlen(apath) + 1,
		           self->uris.atom_Path,
		           LV2_STATE_IS_POD | LV2_STATE_IS_PORTABLE);
		free(apath);
	} else {
		store_param(self, &st, store, handle, uris->eg_path, &state->apath);
	}

	return st;
}

static LV2_State_Status
save(LV2_Handle                instance,
     LV2_State_Store_Function  store,
     LV2_State_Handle          handle,
     uint32_t                  flags,
     const LV2_Feature* const* features)
{
	Params*             self     = (Params*)instance;
	LV2_State_Map_Path* map_path = NULL;
	get_features(features, LV2_STATE__mapPath, &map_path, true, NULL);
	return store_state(self, store, handle, flags, map_path);
}

static void
retrieve_param(Params*                     self,
               LV2_State_Status*           restore_status,
               LV2_State_Retrieve_Function retrieve,
               LV2_State_Handle            handle,
               LV2_URID                    key)
{
	// Retrieve value from saved state
	size_t      vsize;
	uint32_t    vtype;
	uint32_t    vflags;
	const void* value = retrieve(handle, key, &vsize, &vtype, &vflags);

	// Set plugin instance state
	const LV2_State_Status st = value
		? set_parameter(self, key, vsize, vtype, value, true)
		: LV2_STATE_ERR_NO_PROPERTY;

	if (!*restore_status) {
		*restore_status = st;  // Set status if there has been no error yet
	}
}

static LV2_State_Status
restore(LV2_Handle                  instance,
        LV2_State_Retrieve_Function retrieve,
        LV2_State_Handle            handle,
        uint32_t                    flags,
        const LV2_Feature* const*   features)
{
	Params*          self = (Params*)instance;
	LV2_State_Status st   = LV2_STATE_SUCCESS;

	// Retrieve simple properties
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_int);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_long);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_float);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_double);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_bool);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_string);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_path);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_spring);
	retrieve_param(self, &st, retrieve, handle, self->uris.eg_lfo);

	return st;
}

static inline bool
subject_is_plugin(Params* self, const LV2_Atom_URID* subject)
{
	return (subject && subject->atom.type == self->uris.atom_URID &&
	        subject->body != self->uris.plugin);
}

static void
run(LV2_Handle instance, uint32_t sample_count)
{
	Params* self = (Params*)instance;
	URIs*   uris = &self->uris;

	// Initially, self->out_port contains a Chunk with size set to capacity
	// Set up forge to write directly to output port
	const uint32_t out_capacity = self->out_port->atom.size;
	lv2_atom_forge_set_buffer(&self->forge,
	                          (uint8_t*)self->out_port,
	                          out_capacity);

	// Start a sequence in the output port.
	LV2_Atom_Forge_Frame out_frame;
	lv2_atom_forge_sequence_head(&self->forge, &out_frame, 0);

	// Read incoming events
	LV2_ATOM_SEQUENCE_FOREACH(self->in_port, ev) {
		const LV2_Atom_Object* obj = (const LV2_Atom_Object*)&ev->body;
		if (obj->body.otype == uris->patch_Set) {
			// Get the property and value of the set message
			const LV2_Atom_URID* subject  = NULL;
			const LV2_Atom_URID* property = NULL;
			const LV2_Atom*      value    = NULL;
			lv2_atom_object_get(obj,
			                    uris->patch_subject,  (const LV2_Atom**)&subject,
			                    uris->patch_property, (const LV2_Atom**)&property,
			                    uris->patch_value,    &value,
			                    0);
			if (!subject_is_plugin(self, subject)) {
				lv2_log_error(&self->logger,
				              "patch:Set message with unknown subject\n");
			} else if (!property) {
				lv2_log_error(&self->logger,
				              "patch:Set message with no property\n");
			} else if (property->atom.type != uris->atom_URID) {
				lv2_log_error(&self->logger,
				              "patch:Set property is not a URID\n");
			} else {
				const LV2_URID key = property->body;
				set_parameter(self, key, value->size, value->type, value + 1, false);
			}
		} else if (obj->body.otype == uris->patch_Get) {
			// Get the property and value of the get message
			const LV2_Atom_URID* subject  = NULL;
			const LV2_Atom_URID* property = NULL;
			lv2_atom_object_get(obj,
			                    uris->patch_subject,  (const LV2_Atom**)&subject,
			                    uris->patch_property, (const LV2_Atom**)&property,
			                    0);
			if (!subject_is_plugin(self, subject)) {
				lv2_log_error(&self->logger,
				              "patch:Get message with unknown subject\n");
			} else if (!property) {
				// Received a get message, emit our full state (probably to UI)
				lv2_atom_forge_frame_time(&self->forge, ev->time.frames);
				LV2_Atom_Forge_Frame pframe;
				lv2_atom_forge_object(&self->forge, &pframe, 0, uris->patch_Put);
				lv2_atom_forge_key(&self->forge, uris->patch_body);

				LV2_Atom_Forge_Frame bframe;
				lv2_atom_forge_object(&self->forge, &bframe, 0, 0);
				store_state(self, write_param_to_forge, &self->forge, 0, NULL);

				lv2_atom_forge_pop(&self->forge, &bframe);
				lv2_atom_forge_pop(&self->forge, &pframe);
			} else if (property->atom.type != uris->atom_URID) {
				lv2_log_error(&self->logger,
				              "patch:Get property is not a URID\n");
			} else {
				// Received a get message, emit single property state (probably to UI)
				const LV2_URID key = property->body;
				const LV2_Atom *atom = get_parameter(self, key);
				if(atom)
				{
					lv2_atom_forge_frame_time(&self->forge, ev->time.frames);
					LV2_State_Status st = LV2_STATE_SUCCESS;
					LV2_Atom_Forge_Frame frame;
					lv2_atom_forge_object(&self->forge, &frame, 0, uris->patch_Set);
					lv2_atom_forge_key(&self->forge, uris->patch_property);
					lv2_atom_forge_urid(&self->forge, property->body);
					store_param(self, &st, write_param_to_forge, &self->forge,
					            uris->patch_value, atom);
					lv2_atom_forge_pop(&self->forge, &frame);
				}
			}
		} else {
			if (self->unmap) {
				lv2_log_trace(&self->logger,
				              "Unknown object type <%s>\n",
				              self->unmap->unmap(self->unmap->handle, obj->body.otype));
			} else {
				lv2_log_trace(&self->logger,
				              "Unknown object type %d\n", obj->body.otype);
			}
		}
	}

	if (self->spring > 0.01f) {
		self->spring -= 0.001;
		lv2_atom_forge_frame_time(&self->forge, 0);
		LV2_Atom_Forge_Frame frame;
		lv2_atom_forge_object(&self->forge, &frame, 0, uris->patch_Set);

		lv2_atom_forge_key(&self->forge, uris->patch_property);
		lv2_atom_forge_urid(&self->forge, uris->eg_spring);
		lv2_atom_forge_key(&self->forge, uris->patch_value);
		lv2_atom_forge_float(&self->forge, self->spring);

		lv2_atom_forge_pop(&self->forge, &frame);
	}

	lv2_atom_forge_pop(&self->forge, &out_frame);
}

static const void*
extension_data(const char* uri)
{
	static const LV2_State_Interface state = { save, restore };
	if (!strcmp(uri, LV2_STATE__interface)) {
		return &state;
	}
	return NULL;
}

static const LV2_Descriptor descriptor = {
	EG_PARAMS_URI,
	instantiate,
	connect_port,
	NULL,  // activate,
	run,
	NULL,  // deactivate,
	cleanup,
	extension_data
};

LV2_SYMBOL_EXPORT const LV2_Descriptor*
lv2_descriptor(uint32_t index)
{
	return (index == 0) ? &descriptor : NULL;
}
