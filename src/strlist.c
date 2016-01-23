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

#include "strlist.h"

#define REALLOC_FASTER_THAN_FIND

bool
strlist_append(struct string_list *list, char *data) {
	char *new_data;
	size_t length;

	if (!list || !data) return false;
	if (list->count + 1 >= STRLIST_MAX_SIZE) return false;

	if (!data[0]) {
		if (!list->data || !list->size) {
			list->data = malloc(1);
			if (!list->data) return false;
			list->size = 1;
			list->count = 0;
		}
		list->offsets[list->count] = 0;
		list->count += 1;
		return true;
	}

	length = strlen(data) + 1;

	if (!list->data || !list->size) {
		list->data = malloc(length + 1);
		if (!list->data) return false;
		list->size = length + 1;
		list->count = 1;
		list->offsets[0] = 1;
		list->data[0] = 0;
		memcpy(list->data + 1, data, length);
		return true;
	}

	new_data = realloc(list->data, list->size + length);
	if (!new_data) return false;
	memcpy(new_data + list->size, data, length);
	list->data = new_data;
	list->offsets[list->count] = list->size;
	list->size += length;
	list->count += 1;
	return true;
}


bool
strlist_load(struct string_list *list, uint32_t first_key) {
	uint8_t buffer[PERSIST_DATA_MAX_LENGTH];
	int ret;
	int32_t size;
	char *data;
	unsigned page_count;

	size = persist_read_int(first_key);
	if (size <= 0) {
		strlist_reset(list);
		return true;
	}

	data = malloc(size);
	if (!data) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Unable to allocate %" PRId32 " bytes for string list",
		    size);
		return false;
	}

	page_count = (size + PERSIST_DATA_MAX_LENGTH - 1)
	    / PERSIST_DATA_MAX_LENGTH;
	for (unsigned page = 0; page < page_count; page += 1) {
		ret = persist_read_data(first_key + 1 + page,
		    buffer, sizeof buffer);
		if (ret == E_DOES_NOT_EXIST || ret <= 0) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Missing page %u (key %" PRIu32 ") in string list",
			    page, first_key + 1 + page);
			free(data);
			return false;
		}

		memcpy(data + page * PERSIST_DATA_MAX_LENGTH,
		    buffer, ret);
	}

	if (data[0] || data[size - 1]) {
		APP_LOG(APP_LOG_LEVEL_ERROR,
		    "Inconsitent extreme values in string list buffer");
		free(data);
		return false;
	}

	free(list->data);
	list->data = data;
	list->size = size;
	list->count = 0;

	uint16_t first = 1;
	uint16_t last;
	char *p;
	while (first < size) {
		p = memchr(data + first, 0, size - first);
		last = p - data;
		list->offsets[list->count] = first;
		list->count += 1;
		first = last + 1;
	}

	return true;
}


bool
strlist_prepare(struct string_list *list) {
	if (!list) return false;
	if (list->data && list->size) return true;
	list->data = malloc(1);
	if (!list->data) return false;
	list->size = 1;
	return true;
}

bool
strlist_reset(struct string_list *list) {
	char *new_data;

	if (!list) return false;
	new_data = realloc(list->data, 1);
	if (!new_data) return false;
	new_data[0] = 0;
	list->data = new_data;
	list->size = 1;
	list->count = 0;
	return true;
}


bool
strlist_store(struct string_list *list, uint32_t first_key) {
	unsigned page_count = (list->size + PERSIST_DATA_MAX_LENGTH - 1)
	    / PERSIST_DATA_MAX_LENGTH;
	int ret;
	persist_write_int(first_key, list->size);

	for (unsigned page = 0; page < page_count; page += 1) {
		uint16_t chunk_size = (page == page_count - 1)
		    ? (list->size - 1) % PERSIST_DATA_MAX_LENGTH + 1
		    : PERSIST_DATA_MAX_LENGTH;
		ret = persist_write_data(first_key + 1 + page,
		    list->data + page * PERSIST_DATA_MAX_LENGTH,
		    chunk_size);
		if (ret <= 0 || (uint16_t)ret != chunk_size) {
			APP_LOG(APP_LOG_LEVEL_ERROR,
			    "Unexpected value %d returned by "
			    "persist_write_data for page %u "
			    "(requested %" PRIu16 ")",
			    ret, page, chunk_size);
			return false;
		}
	}

	return true;
}


#ifdef REALLOC_FASTER_THAN_FIND
bool
strlist_set_from_dict(struct string_list *list, DictionaryIterator *iterator,
    uint32_t first_key, uint8_t count) {
	if (!strlist_reset(list)) return false;

	if (count >= STRLIST_MAX_SIZE) {
		count = STRLIST_MAX_SIZE - 1;
	}

	for (uint32_t i = 0; i < count; i += 1) {
		Tuple *tuple = dict_find(iterator, first_key + i);
		if (!tuple || tuple->type != TUPLE_CSTRING) continue;

		if (!strlist_append(list, tuple->value->cstring))
			return false;
	}
	return true;
}
#endif
