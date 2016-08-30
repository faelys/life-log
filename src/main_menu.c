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
#include "strset.h"

static Window *window;
static struct event_menu_context *main_menu_context;
static const char *show_log_message = "Show Event Log";

static void
do_show_log(int index, void *context) {
	(void)index;
	(void)context;
	push_log_menu();
}

static void
window_load(Window *window) {
	SimpleMenuItem item = {
	    .callback = &do_show_log,
	    .title = show_log_message
	};

	main_menu_context = event_menu_build(window, 1, &item, INVALID_INDEX);
}

static void
window_unload(Window *window) {
	if (main_menu_context) {
		event_menu_destroy(main_menu_context);
		main_menu_context = 0;
	}
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
	SimpleMenuItem item = {
	    .callback = &do_show_log,
	    .title = show_log_message
	};

	if (!window || !main_menu_context) return;

	event_menu_destroy(main_menu_context);
	main_menu_context = event_menu_build(window, 1, &item, INVALID_INDEX);
}
