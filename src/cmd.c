// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  cmd.c - verictl commands
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */ 
#include <stdlib.h>
#include <stdio.h>
#include <errno.h>
#include <sys/ioctl.h>
#include <string.h>

#include "arglist.h"
#include "options.h"
#include "cmd.h"
#include "parse.h"

static const char *
cmd2name(unsigned int cmd)
{
	switch (cmd) {
		case VERIEXEC_IO_LOAD :
			return "veriexec_load";
		case VERIEXEC_IO_UNLOAD :
			return "veriexec_unload";
		case VERIEXEC_IO_SETLVL:
			return "veriexec_setlvl";
		case VERIEXEC_IO_GETLVL:
			return "veriexec_getlvl";
		case VERIEXEC_IO_ADDCTX :
			return "veriexec_add_ctx";
		case VERIEXEC_IO_DELCTX:
			return "veriexec_del_ctx";
		case VERIEXEC_IO_SETCTX:
			return "veriexec_set_ctx";
		case VERIEXEC_IO_SETUPDATE:
			return "veriexec_set_update";
		case VERIEXEC_IO_MEMCHK:
			return "veriexec_memchk";
		default:
			return "<invalid>";
	}
}

#define do_ioctl3(fd,cmd,arg,name) do {\
		if (g_opts.options & OPTION_DEBUG) { \
			printf("ioctl3: %s\n", cmd2name(cmd)); \
		} else { \
			retval = ioctl((fd), (cmd), (arg)); \
			if (retval) \
				WARN_ERRNO("ioctl error: %s (%s)", \
							cmd2name(cmd), name);\
		}\
} while(0)

#define set_request_entry() do {\
	switch (g_opts.action) {\
		case ActionLoad:			\
			request = VERIEXEC_IO_LOAD; 	\
			break;				\
		case ActionUnload:			\
			request = VERIEXEC_IO_UNLOAD; 	\
			break;				\
		default:				\
			WARN("Unsupported entry action:  %d", g_opts.action); \
			return -EINVAL; \
	} \
} while (0)

int 
do_line(char *line, int fd)
{
	int retval;
	unsigned int request;
	struct veriexec_arg *arg;

	set_request_entry();

	retval = parse_line(line, &arg);
	if (retval)
		return retval;

	do_ioctl3(fd, request, arg, arg->a_fname);

	FREE_ARG(arg);

	/* Do not return an error when unloading fails due to 
	 * ENOENT.
	 */
	if (g_opts.action == ActionUnload && retval && errno == ENOENT)
		retval = 0;

	return retval;
}

int 
do_config(const char *filename, int fd)
{
	int retval;
	unsigned int request;
	struct arglist_node *head, *cur, *tmp;

	set_request_entry();

	retval = parse_config(filename, &head);
	if (retval) 
		return retval;

	list_for_each(cur, head) {
		if (cur->arg) {
			do_ioctl3(fd, request, cur->arg, cur->arg->a_fname);
		} else {
			fprintf(stderr, "%s: empty arg node?\n", __FUNCTION__);
		}
	}

	cur = head;
	do {
		tmp = cur;
		cur = cur->next;
		FREE_ARGLIST(tmp);
	} while (cur != head);

	/* Do not return an error when unloading fails due to 
	 * ENOENT.
	 */
	if (g_opts.action == ActionUnload && retval && errno == ENOENT)
		retval = 0;

	return retval;
}

int
do_ctx(char *line, int fd)
{
	unsigned int request;
	char reqname[6];
	struct veriexec_xarg *xarg;

	int retval;

	retval = parse_xline(line, &xarg);
	if (retval)
		return retval;

	switch (g_opts.action) {
		case ActionAddCtx:
			request = VERIEXEC_IO_ADDCTX;
			break;
		case ActionDelCtx:
			request = VERIEXEC_IO_DELCTX;
			break;
		case ActionSetCtx:
			request = VERIEXEC_IO_SETCTX;
			break;
		default:
			WARN("Unsupported context action %d", g_opts.action);
			return -1;
	}

	(void) snprintf(reqname, 6, "%d", xarg->a_ctx);
	/* Needed because we don't deal with snprintf errors... */
	reqname[5] = '\0';
	do_ioctl3(fd, request, xarg, reqname);

	return retval;
}

int
do_setlvl(int fd)
{
	int retval = 0;
	char reqname[8];
	struct veriexec_larg larg = {
		.a_ctx = (unsigned int)-1
	};

	larg.a_lvl = g_opts.level;
	if (g_opts.ctx)
		larg.a_ctx = g_opts.ctx;

	(void)snprintf(reqname, 8, "0x%x", g_opts.level);
	reqname[7] = '\0';
	do_ioctl3(fd, VERIEXEC_IO_SETLVL, &larg, reqname);

	return retval;
}

int
do_printlvl(int fd) {
	int retval = 0;
	struct veriexec_larg larg = {
		.a_ctx = (unsigned int)-1
	};

	do_ioctl3(fd, VERIEXEC_IO_GETLVL, &larg, "getlvl");
	if (retval)
		return retval;

	fprintf(stdout, "Current veriexec level is %d\n", 
			larg.a_lvl);
	return 0;
}
	
int
do_setupdate(unsigned int ctx, int fd)
{
	char reqname[8];
	int retval = 0;
	
	(void)snprintf(reqname, 8, "0x%x", ctx);
	reqname[7] = '\0';
	do_ioctl3(fd, VERIEXEC_IO_SETUPDATE, &ctx, reqname);

	return retval;
}

int
do_memchk(int fd) {
	int retval = 0;
	unsigned int count;

	do_ioctl3(fd, VERIEXEC_IO_MEMCHK, &count, "memchk");
	if (retval)
		return retval;

	fprintf(stdout, "%u entries are currently allocated\n", count);
	return 0;
}
