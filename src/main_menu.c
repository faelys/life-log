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

#include <inttypes.h>
#include <pebble.h>

#include "global.h"
#include "strlist.h"

#define EXTRA_ITEMS 1
#define SUBTITLE_FORMAT "%Y-%m-%d %H:%M:%S"
#define SUBTITLE_LENGTH 20

static Window *window;
static SimpleMenuLayer *menu_layer;
static SimpleMenuSection menu_section;
static char *subtitles;
static const char *no_event_message = "No event configured.";
static const char *show_log_message = "Show Event Log";
static time_t event_last_seen[64];

static void
set_subtitle(uint16_t id, uint16_t index) {
	if (!event_last_seen[id]) {
		strncpy(subtitles + index * SUBTITLE_LENGTH, "unknown",
		    SUBTITLE_LENGTH);
		return;
	}

	struct tm *tm = localtime(&event_last_seen[id]);
	size_t result = strftime(subtitles + index * SUBTITLE_LENGTH,
	    SUBTITLE_LENGTH,
	    SUBTITLE_FORMAT,
	    tm);
	if (result != SUBTITLE_LENGTH - 1) {
		APP_LOG(APP_LOG_LEVEL_WARNING,
		    "Unexpected result %zu of subtitle strftime,"
		    " instead of %d",
		    result, SUBTITLE_LENGTH - 1);
	}
}

static void
update_last_seen(uint16_t id, uint16_t index) {
	event_last_seen[id] = time(0);
	persist_write_data(200, event_last_seen, sizeof event_last_seen);
	set_subtitle(id, index);
	if (menu_layer) {
		layer_mark_dirty(simple_menu_layer_get_layer(menu_layer));
	}
}

static void
do_record_event(int index, void *context) {
	(void)context;

	if (index < EXTRA_ITEMS) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "do_record_event called with unexpected index %d"
		    " (EXTRA_ITEMS being %d)",
		    index, EXTRA_ITEMS);
		return;
	}

	for (uint16_t id = 0, i = EXTRA_ITEMS;
	    id < event_names.count;
	    id += 1) {
		if (STRLIST_UNSAFE_ITEM(event_names, id)[0] == '-') {
			continue;
		}
		if (i == index) {
			record_event(id + 1);
			update_last_seen(id, index);
			return;
		}
		i += 1;
	}
	APP_LOG(APP_LOG_LEVEL_ERROR,
	    "Unexpected fallthrough from loop in do_record_event,"
	    " index = %d, event_names.count = %" PRIu8,
	    index, event_names.count);
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
	uint16_t size = 0;

	for (uint16_t i = 0; i < event_names.count; i++) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);
		if (name[0] == '+') {
			size += 2;
		} else if (name[0] != '-') {
			size += 1;
		}
	}

	section->title = 0;
	section->items = 0;
	section->num_items = (size ? size : 1) + EXTRA_ITEMS;

	items = calloc(section->num_items, sizeof *items);
	if (!items) {
		section->num_items = 0;
		return false;
	}

	subtitles = calloc(section->num_items, SUBTITLE_LENGTH);
	if (!subtitles) {
		section->num_items = 0;
		free(items);
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

	for (uint16_t i = 0, j = EXTRA_ITEMS; i < event_names.count; i++) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);
		if (name[0] == '-') {
			continue;
		}

		set_subtitle(i, j);

		if (name[0] == '+') {
			uint8_t long_id = long_event_id[i] - 1;
			uint8_t other_j = EXTRA_ITEMS + size
			    - long_event_count + long_id;

			if (long_event_id[i] == 0) {
				APP_LOG(APP_LOG_LEVEL_ERROR,
				    "long_event_id[%" PRIu16 "] is 0 "
				    "even though name starts with '+'",
				    i);
				continue;
			}

			items[j++] = (SimpleMenuItem){
			    .callback = &do_record_event,
			    .title = STRLIST_UNSAFE_ITEM(event_begins,
			      long_id),
			    .subtitle = subtitles + j * SUBTITLE_LENGTH,
			};
			items[other_j] = (SimpleMenuItem){
			    .callback = &do_record_event,
			    .title = STRLIST_UNSAFE_ITEM(event_ends, long_id),
			    .subtitle = subtitles + j * SUBTITLE_LENGTH,
			};
		} else {
			items[j++] = (SimpleMenuItem){
			    .callback = &do_record_event,
			    .title = name,
			    .subtitle = subtitles + j * SUBTITLE_LENGTH,
			};
		}
	}

	return true;
}

static void
window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);
	persist_read_data(200, event_last_seen, sizeof event_last_seen);

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
