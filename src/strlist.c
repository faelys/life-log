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
