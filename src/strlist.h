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

#pragma once

#include <stdbool.h>
#include <stdint.h>
#include <pebble.h>

#define STRLIST_MAX_SIZE 83

struct string_list {
	char		*data;
	uint16_t	offsets[STRLIST_MAX_SIZE];
	uint16_t	size;
	uint8_t		count;
};

#define STRLIST_UNSAFE_ITEM(list, i) \
	((list).data + (list).offsets[i])

#define STRLIST_ITEM(list, i) \
	((i) < (list).count ? STRLIST_UNSAFE_ITEM(list, i) : ((char *)0))

bool
strlist_set_from_dict(struct string_list *list, DictionaryIterator *iterator,
    uint32_t first_key, uint8_t count);


bool
strlist_append(struct string_list *list, char *data);

bool
strlist_prepare(struct string_list *list);

bool
strlist_reset(struct string_list *list);

bool
strlist_load(struct string_list *list, uint32_t first_key);

bool
strlist_store(struct string_list *list, uint32_t first_key);
