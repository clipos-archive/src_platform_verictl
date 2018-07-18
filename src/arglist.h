// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  arglist.h - verictl internal list of LOAD/UNLOAD arguments
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */

#ifndef _VERICONFIG_ARGLIST_H
#define _VERICONFIG_ARGLIST_H

#include <linux/types.h>

/* Now, this, my friends, is what I call ugly :) */
typedef struct kernel_cap_struct {
	__u32 cap[2];
} kernel_cap_t;

#include <linux/capability.h>
#include <linux/veriexec.h>

#ifndef CAP_CONTEXT
#define CAP_CONTEXT	63
#endif

#define cap_raise(c, flag)  ((c).cap[CAP_TO_INDEX(flag)] |= CAP_TO_MASK(flag))
#define cap_lower(c, flag)  ((c).cap[CAP_TO_INDEX(flag)] &= ~CAP_TO_MASK(flag))
#define cap_raise_p(c, flag)  ((c)->cap[CAP_TO_INDEX(flag)] |= (unsigned int) CAP_TO_MASK(flag))
#define cap_lower_p(c, flag)  ((c)->cap[CAP_TO_INDEX(flag)] &= ~CAP_TO_MASK(flag))

/* doubly linked list of args */
struct arglist_node {
	struct veriexec_arg *arg;
	struct arglist_node *next, *prev;
};

#define FREE_ARG(arg) do {\
		if (arg->a_fname) free(arg->a_fname);\
		if (arg->a_fp) free(arg->a_fp);\
		free(arg);\
} while (0)

#define FREE_ARGLIST(head) do {\
		if (head->arg) FREE_ARG(head->arg);\
		free(head); \
} while (0)

/* Everything else comes from linux/list.h */
static inline void __list_add(struct arglist_node *new,
			      struct arglist_node *prev,
			      struct arglist_node *next)
{
	next->prev = new;
	new->next = next;
	new->prev = prev;
	prev->next = new;
}

static inline void push_back(struct arglist_node *new, struct arglist_node *head)
{
	__list_add(new, head, head->next);
}

static inline void push_front(struct arglist_node *new, struct arglist_node *head)
{
	__list_add(new, head->prev, head);
}

static inline void __list_del(struct arglist_node * prev, struct arglist_node * next)
{
	next->prev = prev;
	prev->next = next;
}

static inline void list_del(struct arglist_node *entry)
{
	__list_del(entry->prev, entry->next);
	entry->next = NULL;
	entry->prev = NULL;
}

#define list_init(ptr) do {\
		(ptr)->arg = NULL;\
		(ptr)->next = (ptr);\
		(ptr)->prev = (ptr);\
} while (0)

#define list_for_each(pos, head) for (pos = (head)->next; pos != (head); pos = pos->next)

#endif /*_VERICONFIG_ARGLIST_H*/
