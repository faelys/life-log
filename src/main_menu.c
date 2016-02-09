/*
 * Copyright (c) 2016, Natacha Porté
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

#include <pebble.h>

#include "global.h"
#include "strlist.h"

static Window *window;
static SimpleMenuLayer *menu_layer;
static SimpleMenuSection menu_section;
static const char *no_event_message = "No event configured.";
static const char *show_log_message = "Show Event Log";

#define EXTRA_ITEMS 1

static void
do_record_event(int index, void *context) {
	(void)context;
	if (index > 0 && index <= 255)
		record_event(index);
}

static void
do_show_log(int index, void *context) {
	(void)index;
	(void)context;
	push_log_menu();
}

static bool
rebuild_menu(SimpleMenuSection *section) {
	SimpleMenuItem *items;
	uint16_t size = event_names.count;

	section->title = 0;
	section->items = 0;
	section->num_items = (size ? size : 1) + EXTRA_ITEMS;

	items = calloc(section->num_items, sizeof *items);
	if (!items) {
		section->num_items = 0;
		return false;
	}
	section->items = items;

	items[0] = (SimpleMenuItem){
	    .callback = &do_show_log,
	    .title = show_log_message
	};

	if (!size) {
		items[EXTRA_ITEMS] = (SimpleMenuItem){
		    .title = no_event_message
		};
		return true;
	}

	for (uint16_t i = 0; i < event_names.count; i++) {
		items[EXTRA_ITEMS + i] = (SimpleMenuItem){
		    .callback = &do_record_event,
		    .title = STRLIST_UNSAFE_ITEM(event_names, i)
		};
	}

	return true;
}

static void
window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	if (!rebuild_menu(&menu_section)) return; /* TODO: display error */

	menu_layer = simple_menu_layer_create(bounds, window,
	    &menu_section, 1, 0);
	layer_add_child(window_layer, simple_menu_layer_get_layer(menu_layer));
}

static void
window_unload(Window *window) {
	simple_menu_layer_destroy(menu_layer);
	free((void *)menu_section.items);
	menu_section.items = 0;
}

void
push_main_menu(void) {
	if (!window) {
		window = window_create();
		window_set_window_handlers(window, (WindowHandlers) {
		    .load = &window_load,
		    .unload = &window_unload,
		});
	}
	window_stack_push(window, true);
}

void
update_main_menu(void) {
	if (!window || !menu_layer) return;

	simple_menu_layer_destroy(menu_layer);
	menu_layer = 0;
	free((void *)menu_section.items);
	menu_section.items = 0;

	if (!rebuild_menu(&menu_section)) return;

	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	menu_layer = simple_menu_layer_create(bounds, window,
	    &menu_section, 1, 0);
	layer_add_child(window_layer, simple_menu_layer_get_layer(menu_layer));
}
