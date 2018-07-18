// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  verictl.c - verictl main file
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>

#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>

#include "cmd.h"
#include "options.h"

int main(int argc, char *argv[])
{
	int fd;
	
	int retval;

	retval = verictl_getopts(argc, argv);
	if (retval == GETOPTS_DONE)
		return EXIT_SUCCESS;
	if (retval == GETOPTS_ABORT)
		return EXIT_FAILURE;

	if (!(g_opts.options & OPTION_DEBUG)) {
		fd = open("/dev/veriexec", O_RDWR);
		if (fd == -1) {
			fputs("Could not open /dev/veriexec\n", stderr);
			return EXIT_FAILURE;
		}
	} else {
		fd = -1;
	}
	switch (g_opts.action) {
		case ActionLoad:
		case ActionUnload:
			switch (g_opts.argtype) {
			case ArgCmdLine:
				retval = do_line(g_opts.args, fd);
				break;
			case ArgConfFile:
				retval = do_config(g_opts.args, fd);
				break;
			default:
				WARN("Unsupported load entry argument type");
				retval = EXIT_FAILURE;
				break;
			}
			break;
		case ActionAddCtx:
		case ActionDelCtx:
		case ActionSetCtx:
			retval = do_ctx(g_opts.args, fd);
			break;
		case ActionSetLevel:
			retval = do_setlvl(fd);
			break;
		case ActionPrintLevel:
			retval = do_printlvl(fd);
			break;
		case ActionSetUpdate:
			retval = do_setupdate(g_opts.ctx, fd);
			break;
		case ActionMemCheck:
			retval = do_memchk(fd);
			break;
		default:
			WARN("Unsupported action: %d", g_opts.action);
			retval = EXIT_FAILURE;
	}

	close(fd);
	return retval;
}


