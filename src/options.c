// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/*
 *  options.c - verictl options
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */

#define _GNU_SOURCE
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <limits.h>
#include <errno.h>

/* yuck, I really need to explode that header... */
#include <linux/types.h>
typedef __u32 kernel_cap_t;
#include <linux/veriexec.h>
#include <linux/clip_lsm.h>

#include "options.h"
#include "parse.h"

struct verictl_arguments g_opts;

#define opt_error(str) do { \
		fputs("Error parsing options: " str "\n", stderr); \
		return GETOPTS_ABORT; \
} while (0)

static int
checkopts(void)
{
	switch (g_opts.action) {
		case NoAction:
			WARN("No action specified on command line");
			return -1;
		case ActionLoad:
		case ActionUnload:
			if (g_opts.argtype == NoArg) {
				WARN("Missing entry arguments");
				return -1;
			}
			break;
		case ActionSetLevel:
		case ActionPrintLevel:
			break;
		case ActionAddCtx:
		case ActionDelCtx:
		case ActionSetCtx:
			if (g_opts.argtype != ArgCmdLine) {
				WARN("Missing context argument");
				return -1;
			}
			break;
		case ActionSetUpdate:
		case ActionMemCheck:
			break;
	}
	return 0;
}

static int
get_ctx(const char *str)
{
	errno = 0;
	unsigned long ctx = strtoul(str, NULL, 0);
	if (errno)
		return -1;

	/* we didn't overflow unsigned long, so if we have ULONG_MAX that means the
	 * string was "-1" (current context) and it's a valid value */
	if (ctx > UINT_MAX && ctx != ULONG_MAX)
		return -1;

	g_opts.ctx = (unsigned int) ctx;
	return 0;
}

static void 
print_help(const char *prog, int full)
{
	printf("%s -[dDehlmpuxXy] [-f <config_file> | -c <arg>] [-L <level>]"
			" [-U <ctxnum>] \n", prog);
	puts("\t -c : use single argument in <arg>");
	puts("\t -d : disable veriexec");
	puts("\t -D : debug (only print commands)");
	puts("\t -e : enable veriexec");
	puts("\t -f : use entries from <config_file>");
	puts("\t -h : print a short help and exit");
	puts("\t -H : print a longer help and exit");
	puts("\t -l : load hashes");
	puts("\t -L : set level to <level>");
	puts("\t -m : read total number of allocated entries");
	puts("\t -p : print current level");
	puts("\t -u : unload hashes");
	puts("\t -U : set <ctxnum> as update context");
	puts("\t -v : display the program version and exit");
	puts("\t -x : add ctx passed as -c <xarg>");
	puts("\t -X : del ctx passed as -c <xarg>");
	puts("\t -y : change ctx passed as -c <xarg>");
	puts("\n");
	
	if (!full)
		return;

	puts("* Level specification: -L [<ctx>-][<kwd>:]+<kwd>,"
		" with <kwd> one of:");
	puts("\tinactive");
	fputs("\t", stdout);
	print_string_map(levels_map, "\n\t");
	puts("\n");

	puts("* Load/unload argument format:");
	puts("\t<fname>  <ctx>  <flags>  <cap_eff> <cap_perm> <cap_inh>"
		"  <privs> <algo>  <fingerprint>");
	fputs("   with <flags> as ", stdout);
	print_char_map(flags_map, ':');
	fputs("   and <cap_X> as either '-' (no cap) or "
		"a list of capabilities\n", stdout);
	fputs("     separated by '|', each capability being one "
		"of the following keywords:\n     ", stdout);
	print_string_map(caps_map, " / ");
	fputs("   and <privs> as ", stdout);
	print_char_map(privs_map, ':');
	fputs("   and <algo> as ", stdout);
	print_string_map(digests_map, "/");
	puts("\n");

	puts("* Context operations argument format:");
	puts("\t<ctx>  <initial level>  <cap bounding set> <priv bounding set>");
	puts("   with <level> as <kwd1>:<kwd2>:... as above, and <priv> as above.");
	puts("\n");
}

#define _TO_STR(var) #var
#define TO_STR(var) _TO_STR(var)

static inline void
print_version(const char *prog)
{
	printf("%s - Version %s\n", prog, TO_STR(VERSION));
}

static inline int
set_action(action_t action)
{
	if (g_opts.action != NoAction) {
		WARN("Multiple actions on command line, aborting");
		return -1;
	}
	g_opts.action = action;
	return 0;
}

static inline int
set_args(argtype_t type, char *str)
{
	if (g_opts.argtype != NoArg) {
		WARN("Multiple argument types on command line, aborting");
		return -1;
	}
	g_opts.argtype = type;
	g_opts.args = str;
	return 0;
}


int
verictl_getopts(int argc, char *argv[])
{
	int optnum = 0;
	int c;

	memset(&g_opts, 0, sizeof(g_opts));

	while ((c = getopt(argc, argv, "c:dDef:hHlL:mpuU:vxXy")) != -1) {
        	optnum++;
		switch (c) {
			case 'c':
				if (set_args(ArgCmdLine, optarg))
					return GETOPTS_ABORT;
				break;
			case 'd':
				if (set_action(ActionSetLevel))
					return GETOPTS_ABORT;
				g_opts.level = 0;
				break;
			case 'D':
				g_opts.options |= OPTION_DEBUG;
				break;
			case 'e':
				if (set_action(ActionSetLevel))
					return GETOPTS_ABORT;
				g_opts.level = VRXLVL_ACTIVE;
				break;
			case 'f':
				if (set_args(ArgConfFile, optarg))
					return GETOPTS_ABORT;
                		break;
			case 'h':
                		print_help(basename(argv[0]), 0);
                		return GETOPTS_DONE;
				break;
			case 'H':
                		print_help(basename(argv[0]), 1);
                		return GETOPTS_DONE;
				break;
			case 'l':
				if (set_action(ActionLoad))
					return GETOPTS_ABORT;
				break;
			case 'L':
				if (set_action(ActionSetLevel))
					return GETOPTS_ABORT;
				if (get_level(optarg, &(g_opts.level), 
								&(g_opts.ctx)))
					opt_error("failed to parse level");
				break;
			case 'm':
				if (set_action(ActionMemCheck))
					return GETOPTS_ABORT;
				break;
			case 'p':
				if (set_action(ActionPrintLevel))
					return GETOPTS_ABORT;
				break;
			case 'u':
				if (set_action(ActionUnload))
					return GETOPTS_ABORT;
				g_opts.options |= OPTION_NO_PARSE_ERROR;
				break;
			case 'U':
				if (set_action(ActionSetUpdate))
					return GETOPTS_ABORT;
				if (get_ctx(optarg))
					opt_error("failed to parse ctx number");
				break;
			case 'v':
				print_version(basename(argv[0]));
				return GETOPTS_DONE;
				break;
			case 'x':
				if (set_action(ActionAddCtx))
					return GETOPTS_ABORT;
				break;
			case 'X':
				if (set_action(ActionDelCtx))
					return GETOPTS_ABORT;
				break;
			case 'y':
				if (set_action(ActionSetCtx))
					return GETOPTS_ABORT;
				break;
			default:
				fprintf(stderr, "Unknown option: %c\n", c);
				print_help(argv[0], 0);
				return GETOPTS_ABORT;
				break;
		}
	}
	if (!optnum) {
		fputs("You must specify at least one option\n", stderr);
		return GETOPTS_ABORT;
	}
	if (checkopts()) 
		return GETOPTS_ABORT;

	return GETOPTS_CONTINUE;
}
