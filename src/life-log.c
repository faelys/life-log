#include <inttypes.h>
#include <pebble.h>

#include "dict_tools.h"
#include "global.h"
#include "strlist.h"

static void
update_long_event_id(void) {
	uint8_t long_event_count = 0;
	for (uint8_t i = 0; i < event_names.count; i += 1) {
		long_event_id[i]
		    = (STRLIST_UNSAFE_ITEM(event_names, i)[0] == '+')
		    ? ++long_event_count : 0;
	}
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
			    "got signed %" PRIu32 ": %" PRId32,
			    tuple->key, tuple_int(tuple));
			break;
		    case TUPLE_UINT:
			APP_LOG(APP_LOG_LEVEL_INFO,
			    "got unsigned %" PRIu32 ": %" PRIu32,
			    tuple->key, tuple_uint(tuple));
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
		update_long_event_id();
	} else if (tuple) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Unexpected type %d for event count",
		    (int)tuple->type);
	}

	update_main_menu();
}

static void
init(void) {
	strlist_load(&event_names, 1000);
	update_long_event_id();
	event_log_init();

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
