#include <inttypes.h>
#include <pebble.h>

#include "global.h"
#include "strlist.h"

static Window *window;
static TextLayer *text_layer;

static void window_load(Window *window) {
	Layer *window_layer = window_get_root_layer(window);
	GRect bounds = layer_get_bounds(window_layer);

	text_layer = text_layer_create(GRect(0, 72, bounds.size.w, 20));
	text_layer_set_text(text_layer, "Waiting configuration");
	text_layer_set_text_alignment(text_layer, GTextAlignmentCenter);
	layer_add_child(window_layer, text_layer_get_layer(text_layer));
}

static void
window_unload(Window *window) {
	text_layer_destroy(text_layer);
}

static void
inbox_received_handler(DictionaryIterator *iterator, void *context) {
	Tuple *tuple;

	(void)context;

	for (tuple = dict_read_first(iterator);
	    tuple;
	    tuple = dict_read_next(iterator)) {
		switch (tuple->type) {
		    case TUPLE_CSTRING:
			APP_LOG(APP_LOG_LEVEL_INFO,
			    "got string %" PRIu32 ": \"%s\"",
			    tuple->key, tuple->value->cstring);
			break;
		    case TUPLE_INT:
			APP_LOG(APP_LOG_LEVEL_INFO,
			    "got signed %" PRId32 ": %d",
			    tuple->key, (int)tuple->value->int8);
			break;
		    case TUPLE_UINT:
			APP_LOG(APP_LOG_LEVEL_INFO,
			    "got unsigned %" PRIu32 ": %d",
			    tuple->key, (int)tuple->value->uint8);
			break;
		    default:
			APP_LOG(APP_LOG_LEVEL_INFO,
			    "got tuple %" PRIu32 " of unknown type %d",
			    tuple->key, (int)tuple->type);
			break;
		}
	}

	tuple = dict_find(iterator, 1000);
	if (tuple && (tuple->type == TUPLE_UINT || tuple->type == TUPLE_INT)) {
		strlist_set_from_dict(&short_event_names, iterator,
		    1001, tuple->value->uint8);
	} else if (tuple) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Unexpected type %d for short event count",
		    (int)tuple->type);
	}

	tuple = dict_find(iterator, 2000);
	if (tuple && (tuple->type == TUPLE_UINT || tuple->type == TUPLE_INT)) {
		strlist_set_from_dict(&long_event_names, iterator,
		    2001, tuple->value->uint8);
	} else if (tuple) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Unexpected type %d for long event count",
		    (int)tuple->type);
	}

	push_main_menu();
}

static void
init(void) {
	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(8192, 0);

	window = window_create();
	window_set_window_handlers(window, (WindowHandlers) {
	    .load = window_load,
	    .unload = window_unload,
	});

	window_stack_push(window, true);
}

static void
deinit(void) {
	app_message_deregister_callbacks();
	window_destroy(window);
}

int
main(void) {
	init();
	app_event_loop();
	deinit();
}
