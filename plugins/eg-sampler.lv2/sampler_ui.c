/*
  LV2 Sampler Example Plugin UI
  Copyright 2011 David Robillard <d@drobilla.net>

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
   @file ui.c Sampler Plugin UI
*/

#include <stdlib.h>

#include <gtk/gtk.h>

#include "lv2/lv2plug.in/ns/ext/atom/atom-helpers.h"
#include "lv2/lv2plug.in/ns/ext/atom/atom.h"
#include "lv2/lv2plug.in/ns/ext/atom/forge.h"
#include "lv2/lv2plug.in/ns/ext/urid/urid.h"
#include "lv2/lv2plug.in/ns/extensions/ui/ui.h"

#include "./uris.h"

#define SAMPLER_UI_URI "http://lv2plug.in/plugins/eg-sampler#ui"

typedef struct {
	LV2_Atom_Forge forge;

	LV2_URID_Map* map;

	LV2UI_Write_Function write;
	LV2UI_Controller     controller;

	GtkWidget*           button;
} SamplerUI;

static LV2_URID
uri_to_id(SamplerUI* ui, const char* uri)
{
	return ui->map->map(ui->map->handle, uri);
}

static void
on_load_clicked(GtkWidget* widget,
                void*      handle)
{
	SamplerUI* ui = (SamplerUI*)handle;

	GtkWidget* dialog = gtk_file_chooser_dialog_new(
		"Load Sample",
		NULL,
		GTK_FILE_CHOOSER_ACTION_OPEN,
		GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
		GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
		NULL);

	if (gtk_dialog_run(GTK_DIALOG(dialog)) != GTK_RESPONSE_ACCEPT) {
		gtk_widget_destroy(dialog);
		return;
	}

	char*        filename     = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(dialog));
	const size_t filename_len = strlen(filename);
	gtk_widget_destroy(dialog);

#define OBJ_BUF_SIZE 1024
	uint8_t obj_buf[OBJ_BUF_SIZE];
	lv2_atom_forge_set_buffer(&ui->forge, obj_buf, OBJ_BUF_SIZE);

	LV2_Atom* obj = (LV2_Atom*)lv2_atom_forge_object(
		&ui->forge, NULL, 0, uri_to_id(ui, SET_MESSAGE_URI));
	lv2_atom_forge_property_head(&ui->forge, obj,
	                             uri_to_id(ui, FILENAME_URI), 0);
	lv2_atom_forge_string(&ui->forge, obj, (uint8_t*)filename, filename_len);

	ui->write(ui->controller, 0, sizeof(LV2_Atom) + obj->size,
	          uri_to_id(ui, NS_ATOM "atomTransfer"),
	          obj);

	g_free(filename);
}

static LV2UI_Handle
instantiate(const LV2UI_Descriptor*   descriptor,
            const char*               plugin_uri,
            const char*               bundle_path,
            LV2UI_Write_Function      write_function,
            LV2UI_Controller          controller,
            LV2UI_Widget*             widget,
            const LV2_Feature* const* features)
{
	SamplerUI* ui = (SamplerUI*)malloc(sizeof(SamplerUI));
	ui->map        = NULL;
	ui->write      = write_function;
	ui->controller = controller;
	ui->button     = NULL;

	*widget = NULL;

	for (int i = 0; features[i]; ++i) {
		if (!strcmp(features[i]->URI, LV2_URID_URI "#map")) {
			ui->map = (LV2_URID_Map*)features[i]->data;
		}
	}

	if (!ui->map) {
		fprintf(stderr, "sampler_ui: Host does not support urid:Map\n");
		free(ui);
		return NULL;
	}

	lv2_atom_forge_init(&ui->forge, ui->map);

	ui->button = gtk_button_new_with_label("Load Sample");
	g_signal_connect(ui->button, "clicked",
	                 G_CALLBACK(on_load_clicked),
	                 ui);

	*widget = ui->button;

	return ui;
}

static void
cleanup(LV2UI_Handle handle)
{
	SamplerUI* ui = (SamplerUI*)handle;
	gtk_widget_destroy(ui->button);
	free(ui);
}

static void
port_event(LV2UI_Handle handle,
           uint32_t     port_index,
           uint32_t     buffer_size,
           uint32_t     format,
           const void*  buffer)
{
}

const void*
extension_data(const char* uri)
{
	return NULL;
}

static const LV2UI_Descriptor descriptor = {
	SAMPLER_UI_URI,
	instantiate,
	cleanup,
	port_event,
	extension_data
};

LV2_SYMBOL_EXPORT
const LV2UI_Descriptor*
lv2ui_descriptor(uint32_t index)
{
	switch (index) {
	case 0:
		return &descriptor;
	default:
		return NULL;
	}
}
