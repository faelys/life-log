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

#include "strlist.h"

#define PREFIX_LENGTH 32

#define KEY_EVENT_LOG		 100
#define KEY_EVENT_LAST_SEEN	 200
#define KEY_LONG_EVENT_RUNNING	 210
#define KEY_RECORD_TIME		 500
#define KEY_RECORD_TITLE	 510
#define KEY_BEGIN_PREFIX	 901
#define KEY_END_PREFIX		 902
#define KEY_EVENT_NAMES		1000

extern struct string_list event_names;
extern struct string_list event_begins;
extern struct string_list event_ends;
extern uint8_t long_event_id[STRLIST_MAX_SIZE];
extern uint8_t long_event_count;
extern char begin_prefix[PREFIX_LENGTH];
extern char end_prefix[PREFIX_LENGTH];

void
event_log_init(void);

void
push_main_menu(void);

void
push_log_menu(void);

void
record_event(uint8_t id);

bool
send_recorded_event(time_t time, const char *title);

void
update_main_menu(void);
