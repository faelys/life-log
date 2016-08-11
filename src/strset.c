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

#include <stdlib.h>
#include <string.h>

#include "strset.h"

#define assert(c) \
	do { \
		if (!(c)) \
			APP_LOG(APP_LOG_LEVEL_ERROR, \
			    "assert(%s) failed at %s:%d",\
			    #c, __FILE__, __LINE__); \
	} while (0)

/* find the smallest index larger than or equal to the key */
static bool
search(const struct string_list *set,
    const char *data, size_t size, uint8_t *idx) {
	int16_t low = -1, high = set->count, mid;
	int res;

	while (high - low > 1) {
		const char *mid_item;
		int16_t mid = low + (high - low) / 2;
		assert(low < mid && mid < high);

		mid_item = STRLIST_UNSAFE_ITEM(*set, mid);
		res = strncmp(data, mid_item, size);
		if (res == 0 && mid_item[size] != 0) res = -1;

		if (res == 0) {
			*idx = mid;
			return true;
		}

		if (res < 0) {
			high = mid;
		} else {
			low = mid;
		}
	}

	*idx = high;
	return false;
}


uint8_t
strset_include(struct string_list *set, const char *data, size_t size) {
	uint8_t idx;
	uint16_t offset;

	if (search (set, data, size, &idx)) {
		return idx;
	}

	char data_string[size + 1];
	memcpy(data_string, data, size);
	data_string[size] = 0;
	strlist_append(set, data_string);

	assert(set->count >= 1);
	if (idx < set->count - 1) {
		offset = set->offsets[set->count - 1];
		memmove(set->offsets + idx + 1, set->offsets + idx,
		    (set->count - 1 - idx) * sizeof *set->offsets);
		set->offsets[idx] = offset;
	}

	return idx;
}


uint8_t
strset_search(const struct string_list *set, const char *data, size_t size) {
	uint8_t idx;
	if (search (set, data, size, &idx)) {
		return idx;
	} else {
		return INVALID_INDEX;
	}
}
