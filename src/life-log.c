#include <inttypes.h>
#include <pebble.h>

#include "dict_tools.h"
#include "global.h"
#include "strlist.h"

static void
preprocess_long_events(void) {
	char buffer[128];

	long_event_count = 0;
	strlist_reset(&event_begins);
	strlist_reset(&event_ends);

	for (uint8_t i = 0; i < event_names.count; i += 1) {
		const char *name = STRLIST_UNSAFE_ITEM(event_names, i);

		if (name[0] != '+') {
			long_event_id[i] = 0;
			continue;
		}

		long_event_id[i] = ++long_event_count;

		snprintf(buffer, sizeof buffer, "%s%s", begin_prefix, name + 1);
		strlist_append(&event_begins, buffer);

		snprintf(buffer, sizeof buffer, "%s%s", end_prefix, name + 1);
		strlist_append(&event_ends, buffer);
	}
}

static void
inbox_received_handler(DictionaryIterator *iterator, void *context) {
	Tuple *tuple;
	bool events_updated = false;

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

	tuple = dict_find(iterator, KEY_EVENT_NAMES);
	if (tuple && (tuple->type == TUPLE_UINT || tuple->type == TUPLE_INT)) {
		strlist_set_from_dict(&event_names, iterator,
		    KEY_EVENT_NAMES + 1, tuple->value->uint8);
		strlist_store(&event_names, KEY_EVENT_NAMES);
		events_updated = true;
	} else if (tuple) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Unexpected type %d for event count",
		    (int)tuple->type);
	}

	tuple = dict_find(iterator, KEY_BEGIN_PREFIX);
	if (tuple && tuple->type == TUPLE_CSTRING) {
		strncpy(begin_prefix, tuple->value->cstring,
		    sizeof begin_prefix);
		begin_prefix[sizeof begin_prefix - 1] = 0;
		persist_write_string(KEY_BEGIN_PREFIX, begin_prefix);
		events_updated = true;
	}

	tuple = dict_find(iterator, KEY_END_PREFIX);
	if (tuple && tuple->type == TUPLE_CSTRING) {
		strncpy(end_prefix, tuple->value->cstring,
		    sizeof end_prefix);
		end_prefix[sizeof end_prefix - 1] = 0;
		persist_write_string(KEY_END_PREFIX, end_prefix);
		events_updated = true;
	}

	if (events_updated)
		preprocess_long_events();

	update_main_menu();
}

static void
init(void) {
	persist_read_string(KEY_BEGIN_PREFIX,
	    begin_prefix, sizeof begin_prefix);
	begin_prefix[sizeof begin_prefix - 1] = 0;
	persist_read_string(KEY_END_PREFIX, end_prefix, sizeof end_prefix);
	end_prefix[sizeof end_prefix - 1] = 0;
	strlist_load(&event_names, KEY_EVENT_NAMES);
	preprocess_long_events();
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
