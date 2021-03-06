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

struct __attribute__((__packed__)) entry {
	time_t time;
	uint8_t id;
};

#define PAGE_LENGTH (PERSIST_DATA_MAX_LENGTH / sizeof(struct entry))

struct entry page[PAGE_LENGTH];
uint16_t next_index = 0;

void
event_log_init(void) {
	int ret = persist_read_data(KEY_EVENT_LOG, page, sizeof page);

	if (ret < 0) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Error %d while reading event log",
		    ret);
	} else if ((size_t)ret < sizeof page) {
		APP_LOG(APP_LOG_LEVEL_WARNING,
		    "Short read of event log (%d/%zu)",
		    ret, sizeof page);
	}
}

void
record_event(uint8_t id) {
	const time_t ev_time = time(0);
	const char *title = 0;

	if (!id) return;
	page[next_index].time = ev_time;
	page[next_index].id = id;
	next_index = (next_index + 1) % PAGE_LENGTH;

	if (id <= event_names.count) {
		uint8_t long_id = long_event_id[id - 1];
		title = long_id > 0
		    ? STRLIST_UNSAFE_ITEM(event_begins, long_id - 1)
		    : STRLIST_UNSAFE_ITEM(event_names, id - 1);
	} else if (id >= 128 && id - 128 < event_names.count) {
		uint8_t long_id = long_event_id[id - 128];
		title = long_id > 0
		    ? STRLIST_UNSAFE_ITEM(event_ends, long_id - 1)
		    : STRLIST_UNSAFE_ITEM(event_names, id - 128);
	}
	if (title) send_recorded_event(ev_time, id, title);

	int ret = persist_write_data(KEY_EVENT_LOG, page, sizeof page);
	if (ret < 0) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Error %d while writing event log",
		    ret);
	} else if ((size_t)ret < sizeof page) {
		APP_LOG(APP_LOG_LEVEL_WARNING,
		    "Short write of event log (%d/%zu)",
		    ret, sizeof page);
	}
}


static const char *no_event_message = "No event logged.";

static bool
rebuild_menu(SimpleMenuSection *section, struct string_list *subtitles) {
	SimpleMenuItem *items;
	char buffer[32];
	struct tm *tm;
	int ret;
	uint16_t first = 0, last = 0;

	strlist_reset(subtitles);

	for (uint16_t i = 0; i < PAGE_LENGTH; i += 1) {
		if (!page[i].time) break;
		tm = localtime(&page[i].time);
		ret = strftime(buffer, sizeof buffer, "%Y-%m-%d %H:%M:%S", tm);
		if (!ret) break;
		strlist_append(subtitles, buffer);

		if (i && first == 0) {
			if (page[i].time < page[i - 1].time) {
				last = i - 1;
				first = i;
			} else {
				last = i;
			}
		}
	}

	if (subtitles->count) {
		items = calloc(subtitles->count, sizeof *items);
		if (!items) return false;
	} else {
		items = calloc(1, sizeof *items);
		if (!items) return false;
		items[0] = (SimpleMenuItem) { .title = no_event_message };
	}

	free((void *)section->items);
	section->items = items;
	section->title = 0;
	if (!subtitles->count) {
		section->num_items = 1;
		return true;
	}
	section->num_items = 0;

	for (uint16_t j = last;; j = (j + PAGE_LENGTH - 1) % PAGE_LENGTH) {
		uint16_t i = section->num_items;
		section->num_items += 1;
		if (page[j].id && page[j].id <= event_names.count) {
			uint8_t long_id = long_event_id[page[j].id - 1];
			items[i] = (SimpleMenuItem) {
			    .title = long_id > 0
			    ? STRLIST_UNSAFE_ITEM(event_begins, long_id - 1)
			    : STRLIST_UNSAFE_ITEM(event_names, page[j].id - 1),
			    .subtitle = STRLIST_UNSAFE_ITEM(*subtitles, j)
			};
		} else if (page[j].id >= 128
		    && page[j].id - 128 < event_names.count) {
			uint8_t long_id = long_event_id[page[j].id - 128];
			items[i] = (SimpleMenuItem) {
			    .title = long_id > 0
			    ? STRLIST_UNSAFE_ITEM(event_ends, long_id - 1)
			    : STRLIST_UNSAFE_ITEM(event_names, page[j].id-128),
			    .subtitle = STRLIST_UNSAFE_ITEM(*subtitles, j)
			};
		} else {
			items[i] = (SimpleMenuItem) {
			    .title = STRLIST_UNSAFE_ITEM(*subtitles, j)
			};
		}
		if (j == first) break;
	}

	return true;
}

static Window *window;
static SimpleMenuLayer *menu_layer;
static SimpleMenuSection menu_section;
static struct string_list subtitles;

static void
window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	if (!rebuild_menu(&menu_section, &subtitles)) return;
		/* TODO: display error */

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
push_log_menu(void) {
	if (!window) {
		window = window_create();
		window_set_window_handlers(window, (WindowHandlers) {
		    .load = &window_load,
		    .unload = &window_unload,
		});
	}
	window_stack_push(window, true);
}
