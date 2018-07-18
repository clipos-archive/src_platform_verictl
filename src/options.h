// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  options.h - verictl options header
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */
#ifndef _VERICTL_OPTIONS_H
#define _VERICTL_OPTIONS_H

#include "warn.h"

typedef enum {
	NoAction = 0,
	ActionLoad,
	ActionUnload,
	ActionSetLevel,
	ActionPrintLevel,
	ActionAddCtx,
	ActionDelCtx,
	ActionSetCtx,
	ActionSetUpdate,
	ActionMemCheck
} action_t;

typedef enum {
	NoArg = 0,
	ArgCmdLine,
	ArgConfFile
} argtype_t;

#define OPTION_DEBUG		0x0001
#define OPTION_NO_PARSE_ERROR	0x0002

struct verictl_arguments {
	action_t action;
	argtype_t argtype;

	char *args;
	
	unsigned int options;

	unsigned int level;
	unsigned int ctx;
};

extern struct verictl_arguments g_opts;

#define GETOPTS_DONE 		0
#define GETOPTS_CONTINUE 	1
#define GETOPTS_ABORT 		-1

int verictl_getopts(int argc, char *argv[]);

#endif /*_VERICTL_OPTION_H */
