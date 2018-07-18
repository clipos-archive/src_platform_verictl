// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  devctl.c - devctl main
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */

 
#define _GNU_SOURCE
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdio.h>
#include <errno.h>
#include <ctype.h>
#include <limits.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>

#include <linux/devctl.h>
#include "maps.h"

#define LINE_LEN 1024
#define DEVCTL_DEVICE "/dev/devctl"

static char_map_t devices_map[] = DEFINE_DEVCTL_PERM_MAP;

              /**********************************/
              /*            Parsing             */
              /**********************************/

static int
read_uint(const char *str, unsigned int *val)
{
	unsigned long tmp;
	char *endptr;
	errno = 0;

	tmp = strtoul(str, &endptr, 0);
	
	if (errno) {
		WARN("error reading integer from %s", str);
		return -1;
	}

	if (*endptr != '\0') {
		WARN("invalid characters in integer %s", str);
		return -1;
	}

	if (tmp > UINT_MAX) {
		WARN("integer %s is too big", str);
		return -1;
	}

	*val = (unsigned int)tmp;
	return 0;
}

static int
read_perms(const char *str, unsigned int *perms)
{
	unsigned long tmp = 0;

	if (map_char(str, &tmp, devices_map, 0))
		return -1;
	
	*perms = tmp; /* overflow not expected */
	return 0;
}

static inline void
skip_sep(const char **tok)
{
	const char *ptr = *tok;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;
	*tok = ptr;
}

#define READ_OK		0
#define READ_ERROR	-1
#define READ_NULL	1

static int
read_line(char *buf, struct devctl_arg *arg)
{
	char *ptr, *major, *minor, *range, *prio, *perms;
	const char *cptr;

	if ((ptr = strchr(buf, '#')))
		*ptr = '\0';
	if ((ptr = strchr(buf, '\n')))
		*ptr = '\0';

	ptr = buf;

	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	if (!*ptr)
		return READ_NULL;

#define READ_VAR(var) do { \
	var = strsep(&ptr, " \t"); \
	if (!var) { \
		WARN("no %s", #var); \
		return READ_ERROR; \
	} \
	cptr = ptr; \
	skip_sep(&cptr); \
} while (0)

	READ_VAR(major);
	READ_VAR(minor);
	READ_VAR(range);
	READ_VAR(prio);
#undef READ_VAR
	perms = ptr;
	if ((ptr = strchr(perms, ' ')))
		*ptr = '\0';
	if ((ptr = strchr(perms, '\t')))
		*ptr = '\0';

#define PARSE_VAR(src, dst, fn) do { \
	if (fn(src, dst) == -1) { \
		WARN("failed to parse %s", #src); \
		return READ_ERROR; \
	} \
} while (0)

	PARSE_VAR(major, &arg->a_major, read_uint);
	PARSE_VAR(minor, &arg->a_minor, read_uint);
	PARSE_VAR(range, &arg->a_range, read_uint);
	PARSE_VAR(prio, &arg->a_priority, read_uint);
	PARSE_VAR(perms, &arg->a_perms, read_perms);
#undef PARSE_VAR

	return READ_OK;
}

              /**********************************/
              /*            Conf file           */
              /**********************************/

static int
do_conf_file(const char *fname, int load_p)
{
	char buf[LINE_LEN];
	int ret, fd, cmd, line, count;
	struct devctl_arg arg;
	FILE *file; 

	cmd = (load_p) ? DEVCTL_IO_LOAD : DEVCTL_IO_UNLOAD;
	
	file = fopen(fname, "r");
	if (!file) {
		WARN("could not open %s for reading", fname);
		return -1;
	}

	fd = open(DEVCTL_DEVICE, O_RDWR);
	if (fd == -1) {
		WARN("could not open %s as devctl device", DEVCTL_DEVICE);
		return -1;
	}

	memset(&arg, 0, sizeof(arg));

	line = 0;
	count = 0;
	while (!feof(file)) {
		line++;
		if (!fgets(buf, LINE_LEN, file)) {
			WARN("error reading %s at line %d", fname, line);
			return -1;
		}

		ret = read_line(buf, &arg);
		if (ret == READ_ERROR) {
			WARN("error parsing %s at line %d", fname, line);
			return -1;
		}
		if (ret == READ_NULL)
			continue;

		ret = ioctl(fd, cmd, &arg);
		if (ret == -1) {
			WARN_ERRNO("ioctl error at line %d from %s",
								line, fname);
			return -1;
		}
		count++;
	}

	if (!count) 
		WARN("no action was performed: empty configuration file?");

	return 0;
}

              /**********************************/
              /*         Single command         */
              /**********************************/

static int
do_cmd_line(char *line, int load_p)
{
	int ret, fd, cmd;
	struct devctl_arg arg;

	cmd = (load_p) ? DEVCTL_IO_LOAD : DEVCTL_IO_UNLOAD;
	
	fd = open(DEVCTL_DEVICE, O_RDWR);
	if (fd == -1) {
		WARN("could not open %s as devctl device", DEVCTL_DEVICE);
		return -1;
	}

	memset(&arg, 0, sizeof(arg));

	ret = read_line(line, &arg);
	if (ret != READ_OK) {
		WARN("error parsing %s", line);
		return -1;
	}
	ret = ioctl(fd, cmd, &arg);
	if (ret == -1) {
		WARN_ERRNO("ioctl error");
		return -1;
	}

	return 0;
}


              /**********************************/
              /*           Main helpers         */
              /**********************************/

static void 
print_help(const char *prog)
{
	printf("%s [opts] [-c <line> | -f <file>]\n", prog);
	puts("Line format:");
	puts("  <major> <minor> <range> <priority> <perms>");
	puts("     with <perms> := [rwsxd]+");
	puts("Options:");
	puts(" -u : unload the specified entry/entries (default is load)");
	puts(" -h : print this help and exit");
	puts(" -v : print version and exit");
}

#define _TO_STR(var) #var
#define TO_STR(var) _TO_STR(var)

static inline void
print_version(const char *prog)
{
	printf("%s - Version %s\n", prog, TO_STR(VERSION));
}

              /**********************************/
              /*              Main              */
              /**********************************/

#define DO_CMD_LINE 1
#define DO_CONF_FILE 2

int
main(int argc, char *argv[])
{
	int c;
	int load_p = 1;
	int action = 0;
	char *_optarg = NULL;

	while ((c = getopt(argc, argv, "c:f:huv")) != -1) {
		switch (c) {
			case 'c':
				if (action) {
					WARN("-c and -f are exclusive");
					return EXIT_FAILURE;
				}
				action = DO_CMD_LINE;
				_optarg = optarg;
				break;
			case 'f':
				if (action) {
					WARN("-c and -f are exclusive");
					return EXIT_FAILURE;
				}
				action = DO_CONF_FILE;
				_optarg = optarg;
				break;
			case 'h':
				print_help(basename(argv[0]));
				return EXIT_SUCCESS;
				break;
			case 'v':
				print_version(basename(argv[0]));
				return EXIT_SUCCESS;
				break;
			case 'u':
				load_p = 0;
				break;
			default:
				WARN("unsupported option: %c", c);
				return EXIT_FAILURE;
				break;
		}
	}

	switch (action) {
		case DO_CMD_LINE:
			return do_cmd_line(_optarg, load_p);
			break;
		case DO_CONF_FILE:
			return do_conf_file(_optarg, load_p);
			break;
		default:
			WARN("-c or -f must be specified");
			return EXIT_FAILURE;
			break;
	}
	/* Not reached */
}
			
