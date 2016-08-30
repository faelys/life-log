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
#include "strset.h"

#define EXTRA_ITEMS 1
#define SUBTITLE_FORMAT "%Y-%m-%d %H:%M:%S"
#define SUBTITLE_LENGTH 20

struct event_menu_context {
	SimpleMenuLayer *menu_layer;
	SimpleMenuSection section;
	SimpleMenuItem *items;
	char *subtitles;
	uint8_t *ids;
	unsigned extra_items;
	uint8_t filter_id;
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

static bool
check_callback_context(int index, struct event_menu_context *context) {
	if (!context) return false;

	if (index < 0 || (unsigned)index < context->extra_items) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "event_menu callback called with unexpected index %d"
		    " (extra_items being %u)",
		    index, context->extra_items);
		return false;
	}

	if (context->ids[(unsigned)index - context->extra_items] == 0) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "event_menu callback called without recorded id"
		    " (index %d, extra_items %u)",
		    index, context->extra_items);
		return false;
	}

	return true;
}

static void
do_record_short_event(int index, void *void_context) {
	struct event_menu_context *context = void_context;
	unsigned corrected_index;
	uint8_t id;
	char *subtitle;

	if (!check_callback_context(index, context)) return;

	corrected_index = (unsigned)index - context->extra_items;
	id = context->ids[corrected_index] - 1;
	subtitle = context->subtitles + corrected_index * SUBTITLE_LENGTH;

	record_event(id + 1);
	update_last_seen(id);
	set_subtitle(subtitle, id);

	if (context->menu_layer)
		layer_mark_dirty(simple_menu_layer_get_layer
		    (context->menu_layer));
}

static void
do_record_long_event(int index, void *void_context) {
	struct event_menu_context *context = void_context;
	unsigned corrected_index;
	uint8_t id;
	bool running, secondary;

	if (!check_callback_context(index, context)) return;

	corrected_index = (unsigned)index - context->extra_items;
	secondary = (context->ids[corrected_index] >= 128);
	id = context->ids[corrected_index] - (secondary ? 128 : 1);
	running = BITARRAY_TEST(long_event_running, id);

	record_event(id + (secondary == running ? 1 : 128));
	if (!secondary) toggle_long_event_running(id);
	update_last_seen(id);
	event_menu_rebuild(context);

	if (context->menu_layer)
		layer_mark_dirty(simple_menu_layer_get_layer
		    (context->menu_layer));
}

bool
event_menu_rebuild(struct event_menu_context *context) {
	SimpleMenuItem *items;
	char *subtitles;
	uint8_t *ids;
	uint16_t size = 0, num_items;
	const char *cur_prefix;
	unsigned cur_prefix_length;
	unsigned separator_length = strlen(directory_separator);
	const char *filter = STRLIST_ITEM(event_prefixes, context->filter_id);
	const unsigned filter_length = filter ? strlen(filter) : 0;

	cur_prefix = 0;
	for (uint16_t i = 0; i < event_names.count; i++) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);
		const char *title = name;
		const char *suffix;

		if (name[0] == '-') continue;
		if (name[0] == '+') title = name + 1;
		if (filter && strncmp(title, filter, filter_length) != 0) {
			continue;
		}
		if (cur_prefix
		    && strncmp(title, cur_prefix, cur_prefix_length) == 0) {
			continue;
		}

		size += 1;
		cur_prefix = 0;

		if (directory_separator[0]
		    && (suffix = strstr(title + filter_length,
		                        directory_separator)) != 0) {
			uint8_t id;
			cur_prefix_length = suffix + separator_length - title;
			id = strset_search(&event_prefixes,
			    title, cur_prefix_length);
			cur_prefix = STRLIST_ITEM(event_prefixes, id);
		}

		if (name[0] == '+') {
			size += 1;
		}
	}

	num_items = (size ? size : 1) + context->extra_items;

	if (context->section.num_items == num_items
	    && context->items) {
		items = context->items;
		subtitles = context->subtitles;
		ids = context->ids;
	} else {
		items = realloc(context->items, num_items * sizeof *items);
		if (!items) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Unable to realloc event menu items from %"
			    PRIu32 " to %" PRIu16,
			    context->section.num_items, num_items);
			return false;
		}

		subtitles = realloc(context->subtitles,
		    num_items * SUBTITLE_LENGTH);

		if (!subtitles) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Unable to realloc subtitles from %"
			    PRIu32 " to %" PRIu16,
			    context->section.num_items, num_items);
			free(items);
			return false;
		}

		ids = realloc(context->ids,
		    (num_items - context->extra_items) * sizeof *ids);

		if (!ids) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Unable to realloc ids from %"
			    PRIu32 " to %" PRIu16,
			    context->section.num_items, num_items);
			free(items);
			free(subtitles);
			return false;
		}

		context->items = items;
		context->subtitles = subtitles;
		context->ids = ids;
		context->section.num_items = num_items;
	}
	context->section.items = items;

	if (!size) {
		items[context->extra_items] = (SimpleMenuItem){
		    .title = no_event_message
		};
		return true;
	}

	cur_prefix = 0;
	for (uint16_t i = 0, j = context->extra_items;
	    i < event_names.count;
	    i++) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);
		const char *title = name;
		char *subtitle;
		char *suffix;

		if (name[0] == '-') {
			continue;
		}
		if (name[0] == '+') title = name + 1;
		if (filter && strncmp(title, filter, filter_length) != 0) {
			continue;
		}
		if (cur_prefix
		    && strncmp(title, cur_prefix, cur_prefix_length) == 0) {
			continue;
		}

		if (directory_separator[0]
		    && (suffix = strstr(title + filter_length,
		                        directory_separator)) != 0) {
			uint8_t id;
			cur_prefix_length = suffix + separator_length - title;
			id = strset_search(&event_prefixes,
			    title, cur_prefix_length);
			cur_prefix = STRLIST_ITEM(event_prefixes, id);
			if (cur_prefix) {
				items[j++] = (SimpleMenuItem){
				    .title = cur_prefix,
				};
				continue;
			}
		} else {
			cur_prefix = 0;
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

			ids[j - context->extra_items] = i + 1;
			ids[other_j - context->extra_items] = i + 128;

			items[j] = (SimpleMenuItem){
			    .callback = &do_record_long_event,
			    .title = STRLIST_UNSAFE_ITEM(running
			      ? event_ends : event_begins, long_id),
			    .subtitle = subtitle,
			};
			items[other_j] = (SimpleMenuItem){
			    .callback = &do_record_long_event,
			    .title = STRLIST_UNSAFE_ITEM(running
			      ? event_begins : event_ends, long_id),
			    .subtitle = subtitle,
			};
			j++;
		} else {
			ids[j - context->extra_items] = i + 1;
			items[j++] = (SimpleMenuItem){
			    .callback = &do_record_short_event,
			    .title = title + filter_length,
			    .subtitle = subtitle,
			};
		}
	}

	return true;
}


struct event_menu_context *
event_menu_build(Window *parent, unsigned extra_items,
    SimpleMenuItem *items, uint8_t filter_id) {
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
	context->ids = 0;
	context->filter_id = filter_id;

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
	free(context->ids);
	free(context);
}
