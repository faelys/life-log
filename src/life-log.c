#include <inttypes.h>
#include <pebble.h>

#include "global.h"
#include "strlist.h"

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
		strlist_set_from_dict(&event_names, iterator,
		    1001, tuple->value->uint8);
		strlist_store(&event_names, 1000);
	} else if (tuple) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Unexpected type %d for event count",
		    (int)tuple->type);
	}

	push_main_menu();
}

static void
init(void) {
	strlist_load(&event_names, 1000);

	app_message_register_inbox_received(inbox_received_handler);
	app_message_open(8192, 0);

	push_main_menu();
}

static void
deinit(void) {
	app_message_deregister_callbacks();
}

int
main(void) {
	init();
	app_event_loop();
	deinit();
}
