// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  maps.h - verictl keyword maps header
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */
#ifndef _VERICONFIG_MAP_H
#define _VERICONFIG_MAP_H

#include "warn.h"

typedef struct {
	unsigned long bit;
	char chr;
} char_map_t;

typedef struct {
	unsigned long bit;
	const char *str;
	size_t len;
} string_map_t;

extern char_map_t privs_map[];
extern char_map_t flags_map[];
extern string_map_t levels_map[];
extern string_map_t digests_map[];
extern string_map_t caps_map[];

/* Less efficient than a switch, but easier to keep up-to-date */

static inline int
map_char(const char *str, unsigned long *ret, char_map_t *map, int relaxed_p)
{
	char_map_t *iter;
	unsigned long tmp = 0;
	const char *ptr = str;

next:
	while (*ptr) {
		for (iter = map; iter->chr != 0; iter++) {
			if (*ptr == iter->chr) {
				tmp |= iter->bit;
				ptr++;
				goto next;
			}
		}
		if (!relaxed_p) {
			WARN("unknown character: %c, aborting", *ptr);
			return -EINVAL;
		} else {
			WARN("unknown character: %c, ignoring", *ptr);
			ptr++;
		}
	}
	*ret = tmp;
	return 0;
}

static inline void
print_char_map(char_map_t *map, char sep)
{
	char_map_t *iter;
	int first = 1;

	for (iter = map; iter->chr != 0; iter++) {
		if (first) {
			first = 0;
		} else {
			putchar(sep);
		}
		putchar(iter->chr);
	}
	putchar('\n');
}
		
static inline int
map_string(const char *str, size_t len, unsigned long *ret, string_map_t *map)
{
	string_map_t *iter;

	for (iter = map; iter->len != 0; iter++) {
		if (iter->len != len)
			continue;
		if (strncmp(iter->str, str, len))
			continue;

		*ret = iter->bit;
		return 0;
	}

	WARN("unknown keyword: %s", str);
	return -EINVAL;
}	
		
static inline void
print_string_map(string_map_t *map, const char *sep)
{
	string_map_t *iter;
	int first = 1;

	for (iter = map; iter->len != 0; iter++) {
		if (first) {
			first = 0;
		} else {
			fputs(sep, stdout);
		}
			
		fputs(iter->str, stdout);
	}
	putchar('\n');
}

#endif /*_VERICONFIG_MAP_H */
