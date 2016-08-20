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

#include "bitarray.h"
#include "global.h"
#include "strlist.h"

#define EXTRA_ITEMS 1
#define SUBTITLE_FORMAT "%Y-%m-%d %H:%M:%S"
#define SUBTITLE_LENGTH 20

struct event_menu_context {
	SimpleMenuLayer *menu_layer;
	SimpleMenuSection section;
	SimpleMenuItem *items;
	char *subtitles;
	unsigned extra_items;
};

static const char *no_event_message = "No event configured.";
static time_t event_last_seen[64];
static BITARRAY_DECLARE(long_event_running, 128);

static void
set_subtitle(char *subtitle, uint16_t id) {
	if (!event_last_seen[id]) {
		strncpy(subtitle, "unknown", SUBTITLE_LENGTH);
		return;
	}

	struct tm *tm = localtime(&event_last_seen[id]);
	size_t result = strftime(subtitle,
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
update_last_seen(uint16_t id) {
	event_last_seen[id] = time(0);
	persist_write_data(KEY_EVENT_LAST_SEEN,
	    event_last_seen, sizeof event_last_seen);
}

static void
toggle_long_event_running(uint16_t id) {
	BITARRAY_TOGGLE(long_event_running, id);
	persist_write_data(KEY_LONG_EVENT_RUNNING,
	    long_event_running, sizeof long_event_running);
}

static void
do_record_event(int index, void *void_context) {
	struct event_menu_context *context = void_context;

	if (index < 0 || (unsigned)index < context->extra_items) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "do_record_event called with unexpected index %d"
		    " (EXTRA_ITEMS being %d)",
		    index, context->extra_items);
		return;
	}

	for (uint16_t id = 0, i = context->extra_items;
	    id < event_names.count;
	    id += 1) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, id);
		char *subtitle;
		if (name[0] == '-') {
			continue;
		}
		if (i == index) {
			subtitle = context->subtitles
			    + (i - context->extra_items) * SUBTITLE_LENGTH;
			if (name[0] == '+') {
				if (BITARRAY_TEST(long_event_running, id)) {
					record_event(id + 128);
				} else {
					record_event(id + 1);
				}
				toggle_long_event_running(id);
				update_last_seen(id);
				event_menu_rebuild(context);
			} else {
				record_event(id + 1);
				update_last_seen(id);
				set_subtitle(subtitle, id);
			}
			if (context->menu_layer)
				layer_mark_dirty(simple_menu_layer_get_layer
				    (context->menu_layer));
			return;
		} else if (long_event_id[id] > 0
		    && (unsigned)index == context->section.num_items
		      - long_event_count + long_event_id[id] - 1) {
			if (BITARRAY_TEST(long_event_running, id)) {
				record_event(id + 1);
			} else {
				record_event(id + 128);
			}
			update_last_seen(id);
			event_menu_rebuild(context);
			if (context->menu_layer)
				layer_mark_dirty(simple_menu_layer_get_layer
				    (context->menu_layer));
			return;
		}
		i += 1;
	}
	APP_LOG(APP_LOG_LEVEL_ERROR,
	    "Unexpected fallthrough from loop in do_record_event,"
	    " index = %d, event_names.count = %" PRIu8,
	    index, event_names.count);
}

bool
event_menu_rebuild(struct event_menu_context *context) {
	SimpleMenuItem *items;
	char *subtitles;
	uint16_t size = 0, num_items;

	for (uint16_t i = 0; i < event_names.count; i++) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);
		if (name[0] == '+') {
			size += 2;
		} else if (name[0] != '-') {
			size += 1;
		}
	}

	num_items = (size ? size : 1) + context->extra_items;

	if (context->section.num_items == num_items
	    && context->items) {
		items = context->items;
		subtitles = context->subtitles;
	} else {
		items = realloc(context->items, num_items * sizeof *items);
		if (!items) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Untable to realloc event menu items from %"
			    PRIu32 " to %" PRIu16,
			    context->section.num_items, num_items);
			return false;
		}

		subtitles = realloc(context->subtitles,
		    num_items * SUBTITLE_LENGTH);

		if (!subtitles) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Untable to realloc subtitles from %"
			    PRIu32 " to %" PRIu16,
			    context->section.num_items, num_items);
			free(items);
			return false;
		}

		context->items = items;
		context->subtitles = subtitles;
		context->section.num_items = num_items;
	}
	context->section.items = items;

	if (!size) {
		items[context->extra_items] = (SimpleMenuItem){
		    .title = no_event_message
		};
		return true;
	}

	for (uint16_t i = 0, j = context->extra_items;
	    i < event_names.count;
	    i++) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);
		char *subtitle;
		if (name[0] == '-') {
			continue;
		}

		subtitle = subtitles
		    + (j - context->extra_items) * SUBTITLE_LENGTH;
		set_subtitle(subtitle, i);

		if (name[0] == '+') {
			uint8_t long_id = long_event_id[i] - 1;
			uint8_t other_j = context->extra_items + size
			    - long_event_count + long_id;
			bool running = BITARRAY_TEST(long_event_running, i);

			if (long_event_id[i] == 0) {
				APP_LOG(APP_LOG_LEVEL_ERROR,
				    "long_event_id[%" PRIu16 "] is 0 "
				    "even though name starts with '+'",
				    i);
				continue;
			}

			items[j] = (SimpleMenuItem){
			    .callback = &do_record_event,
			    .title = STRLIST_UNSAFE_ITEM(running
			      ? event_ends : event_begins, long_id),
			    .subtitle = subtitle,
			};
			items[other_j] = (SimpleMenuItem){
			    .callback = &do_record_event,
			    .title = STRLIST_UNSAFE_ITEM(running
			      ? event_begins : event_ends, long_id),
			    .subtitle = subtitle,
			};
			j++;
		} else {
			items[j++] = (SimpleMenuItem){
			    .callback = &do_record_event,
			    .title = name,
			    .subtitle = subtitle,
			};
		}
	}

	return true;
}


struct event_menu_context *
event_menu_build(Window *parent, unsigned extra_items, SimpleMenuItem *items) {
	struct event_menu_context *context;

	context = calloc(1, sizeof *context);
	if (!context) return 0;
	if (!items) extra_items = 0;

	context->menu_layer = 0;
	context->section.title = 0;
	context->section.items = 0;
	context->section.num_items = 0;
	context->subtitles = 0;
	context->extra_items = extra_items;
	context->items = 0;

	if (!event_menu_rebuild(context)) {
		free(context);
		return 0;
	}
	if (extra_items > 0)
		memcpy(context->items, items, extra_items * sizeof *items);

	Layer *window_layer = window_get_root_layer(parent);
	GRect bounds = layer_get_bounds(window_layer);

	context->menu_layer = simple_menu_layer_create(bounds, parent,
	    &context->section, 1, context);
	layer_add_child(window_layer,
	    simple_menu_layer_get_layer(context->menu_layer));
	return context;
}

void
event_menu_destroy(struct event_menu_context *context) {
	simple_menu_layer_destroy(context->menu_layer);
	free((void *)context->section.items);
	context->section.items = 0;
	context->section.num_items = 0;
	free(context->subtitles);
	free(context);
}
