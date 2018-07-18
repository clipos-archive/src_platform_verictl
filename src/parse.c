// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  parse.c - verictl config file parser 
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2010-2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <errno.h>
#include <limits.h>
#include <stddef.h>

#include "parse.h"
#include "arglist.h"
#include "options.h"

/* Config line maximum length */
#define MAX_LEN 1024

/************************************************/
/*               Keyword maps                   */
/************************************************/

char_map_t privs_map[] = DEFINE_CLSM_PRIV_MAP;
char_map_t flags_map[] = DEFINE_VERIEXEC_FLAG_MAP;

string_map_t levels_map[] = {
	{ VRXLVL_ACTIVE			, "active"	, 6},
	{ VRXLVL_LVL_IMMUTABLE		, "lvl_immutable"	, 13},
	{ VRXLVL_SELF_IMMUTABLE		, "self_immutable"	, 14},
	{ VRXLVL_ADMIN_IMMUTABLE	, "admin_immutable"	, 15},
	{ VRXLVL_UPDATE_IMMUTABLE	, "update_immutable"	, 16},
	{ VRXLVL_CTX_IMMUTABLE		, "ctx_immutable"	, 13},
	{ VRXLVL_CTXSET_IMMUTABLE	, "ctxset_immutable"	, 16},
	{ VRXLVL_ENFORCE_MNTRO		, "enforce_mntro"	, 13},
	{ VRXLVL_ENFORCE_INHERIT	, "enforce_inherit"	, 15},
	{ 0, 0, 0}
};

string_map_t digests_map[] = {
	{ VERIEXEC_DIG_MD5	, "md5",	3},
	{ VERIEXEC_DIG_SHA1	, "sha1",	4},
	{ VERIEXEC_DIG_SHA256	, "sha256",	6},
	{ VERIEXEC_DIG_CCSD	, "ccsd",	4},
	{ 0, 0, 0}
};

#define CAP_TO_MAP(cap) { CAP_##cap, #cap, sizeof(#cap) - 1 }

string_map_t caps_map[] = {
	CAP_TO_MAP(CHOWN),
	CAP_TO_MAP(DAC_OVERRIDE),
	CAP_TO_MAP(DAC_READ_SEARCH),
	CAP_TO_MAP(FOWNER),
	CAP_TO_MAP(FSETID),
	CAP_TO_MAP(KILL),
	CAP_TO_MAP(SETGID),
	CAP_TO_MAP(SETUID),
	CAP_TO_MAP(SETPCAP),
	CAP_TO_MAP(LINUX_IMMUTABLE),
	CAP_TO_MAP(NET_BIND_SERVICE),
	CAP_TO_MAP(NET_BROADCAST),
	CAP_TO_MAP(NET_ADMIN),
	CAP_TO_MAP(NET_RAW),
	CAP_TO_MAP(IPC_LOCK),
	CAP_TO_MAP(IPC_OWNER),
	CAP_TO_MAP(SYS_MODULE),
	CAP_TO_MAP(SYS_RAWIO),
	CAP_TO_MAP(SYS_CHROOT),
	CAP_TO_MAP(SYS_PTRACE),
	CAP_TO_MAP(SYS_PACCT),
	CAP_TO_MAP(SYS_ADMIN),
	CAP_TO_MAP(SYS_BOOT),
	CAP_TO_MAP(SYS_NICE),
	CAP_TO_MAP(SYS_RESOURCE),
	CAP_TO_MAP(SYS_TIME),
	CAP_TO_MAP(SYS_TTY_CONFIG),
	CAP_TO_MAP(MKNOD),
	CAP_TO_MAP(LEASE),
	CAP_TO_MAP(AUDIT_WRITE),
	CAP_TO_MAP(AUDIT_CONTROL),
	CAP_TO_MAP(SETFCAP),
	CAP_TO_MAP(MAC_OVERRIDE),
	CAP_TO_MAP(MAC_ADMIN),
	CAP_TO_MAP(CONTEXT),
	{ 0, 0, 0 }
};

/************************************************/
/*               Basic parsers                  */
/************************************************/

static inline int
get_flags(const char *str, veriexec_flags_t *ret)
{
	unsigned long tmp = 0;

	if (map_char(str, &tmp, flags_map, 
			(g_opts.options & OPTION_NO_PARSE_ERROR)))
		return -EINVAL;

	/* no overflow expected, but enforce the check anyway */
	if (tmp > USHRT_MAX)
		return -EINVAL;

	*ret = (veriexec_flags_t) tmp;
	return 0;
}

static inline int
get_privs(const char *str, clsm_privs_t *ret)
{
	unsigned long tmp = 0;

	if (map_char(str, &tmp, privs_map,
			(g_opts.options & OPTION_NO_PARSE_ERROR)))
		return -EINVAL;
	
	*ret = tmp; /* no overflow expected */
	return 0;
}

static inline int
get_caps(char *str, kernel_cap_t *ret)
{
	char *p, *q;
	unsigned long tmp = 0;
	memset(ret, 0, sizeof(*ret));

	p = str;

	/* empty caps */
	if (*str == '-')
		return 0;
	for (;;) {
		size_t len;
		if (!(*p)) {
			WARN("empty cap field in %s[...]", str);
			return -EINVAL;
		}
		q = strchr(p, '|');
		if (q) {
			if (q == p) {
				WARN("something wrong with "
					"cap field %s", str);
				return -EINVAL;
			}
			*q = '\0';
		}

		len = (q) ? (size_t)(q - p) : strlen(p);

		if (map_string(p, len, &tmp, caps_map)) {
			if (!(g_opts.options & OPTION_NO_PARSE_ERROR))
				return -EINVAL;
		} else {
			cap_raise_p(ret, tmp);
		}

		if (q) 
			p = q + 1;
		else
			return 0; /* end of string */
	}

	/* Huh ? */
	return -EFAULT;
}

static inline int
get_digest(const char *str, veriexec_dig_t *ret)
{	
	unsigned long tmp;
	
	if (map_string(str, strlen(str), &tmp, digests_map)) {
		if (!(g_opts.options & OPTION_NO_PARSE_ERROR))
			return -EINVAL;
		WARN("Failed to read digest type, defaulting to SHA256");
		*ret = VERIEXEC_DIG_SHA256;
	} else {
		*ret = tmp; /* no overflow expected */
	}
	return 0;
}

static inline int
get_ulong(const char *str, unsigned long *ret)
{
	char *endptr;
	unsigned long val;

	errno = 0;
	val = strtoul(str, &endptr, 0);
	if (*str == '\0' || *endptr != '\0') {
		WARN("not a number: %s", str);
		return -EINVAL;
	}
	if (errno != 0) {
		WARN("overflow on %s", str);
		return -ERANGE;
	}
	*ret = val;
	return 0;
}


static inline int
str2level(const char *str, size_t len, unsigned int *dest)
{
	unsigned long tmp;
	if (len == 1 && *str == '-') {
		*dest = 0;
		return 0;
	}
	if (len == 8 && !strncmp("inactive", str, 8)) {
		*dest = 0;
		return 0;
	}
	if (map_string(str, len, &tmp, levels_map)) 
		return -EINVAL;

	if (tmp > UINT_MAX)
		return -EINVAL;

	*dest |= (unsigned int) tmp;
	return 0;
}

int
get_level(const char *str, unsigned int *level, unsigned int *ctx)
{
	const char *cur, *ptr;
	size_t len;
	int ret;
	unsigned int _level = 0;
	unsigned long _ctx;

	cur = str;
	if (ctx) {
		/* ctx specification */
		if ((ptr = strchr(cur, '-'))) {
			errno = 0;
			_ctx = strtoul(str, NULL, 0);
			if (errno)
				return -1;

			/* we didn't overflow unsigned long, so if we have ULONG_MAX that means
			 * the string was "-1" (current context) and it's a valid value */
			if (_ctx > UINT_MAX && _ctx != ULONG_MAX)
				return -1;

			cur = ptr+1;
			*ctx = (unsigned int) _ctx;
		}
	}
	while ((ptr = strchr(cur, ':'))) {
		/* ptr > cur by definition, so the cast to unsigned is safe */
		len = (size_t) (ptr - cur);

		ret = str2level(cur, len, &_level);
		if (ret)
			return ret;
		cur = ptr+1;
	}
	/* Trailing keyword after last ':' */
	len = strlen(cur);
	if (len) {
		ret = str2level(cur, len, &_level);
		if (ret)
			return ret;
	}
	
	*level = _level;
	return 0;
}


/************************************************/
/*          Individual fields readers           */
/************************************************/


static inline char *skip_sep(char *ptr)
{
	while (ptr && *ptr && (*ptr == '\t' || *ptr == ' '))
		ptr++;
	return ptr;
}

static inline int
read_string(char **buf, char **out, size_t *outlen)
{
	char *ptr, *new, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	new = strdup(ptr);
	if (!new) {
		ret = -ENOMEM;
		goto err;
	}

	*out = new;
	if (outlen)
		*outlen = strlen(ptr);

	*buf = orig;
	return 0;

err:
	WARN("Could not read string from %s : %s", *buf, strerror(-ret));
	return ret;
}


static inline int
read_flags(char **buf, veriexec_flags_t *flags)
{
	char *ptr, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	ret = get_flags(ptr, flags);
	if (ret)
		goto err;

	*buf = orig;
	return 0;

err:
	WARN("Could not read flags from %s : %s", *buf, strerror(-ret));
	return ret;
}

static inline int
read_privs(char **buf, clsm_privs_t *privs)
{
	char *ptr, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	ret = get_privs(ptr, privs);
	if (ret)
		goto err;

	*buf = orig;
	return 0;

err:
	WARN("Could not read privs from %s : %s", *buf, strerror(-ret));
	return ret;	
}	

static inline int
read_caps(char **buf, kernel_cap_t *cap)
{
	char *ptr, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	ret = get_caps(ptr, cap);
	if (ret)
		goto err;

	*buf = orig;
	return 0;

err:
	WARN("Could not read caps from %s[...] : %s", *buf, strerror(-ret));
	return ret;
}

static inline int
read_ulong(char **buf, unsigned long *ulong)
{
	char *ptr, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	ret = get_ulong(ptr, ulong);
	if (ret)
		goto err;

	*buf = orig;
	return 0;

err:
	WARN("Could not read ulong from %s : %s", *buf, strerror(-ret));
	return ret;	
}

static inline int
read_uint(char **buf, unsigned int *uint)
{
	int retval;
	unsigned long tmp = 0; /* shut up gcc */

	retval = read_ulong(buf, &tmp);
	if (retval)
		return retval;

	/* we didn't overflow unsigned long, so if we have ULONG_MAX that means the
	 * string was "-1" (current context) and it's a valid value */
	if (tmp > UINT_MAX && tmp != ULONG_MAX) {
		WARN("Too big for unsigned int: %lx", tmp);
		return -ERANGE;
	}

	*uint = (unsigned int)tmp;
	return 0;
}

static inline int
read_digest_type(char **buf, veriexec_dig_t *dig)
{
	char *ptr, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	ret = get_digest(ptr, dig);
	if (ret)
		goto err;

	*buf = orig;
	return 0;

err:
	WARN("Could not read digest type from %s : %s", *buf, 
							strerror(-ret));
	return ret;	
}

static inline int
read_level(char **buf, unsigned int *level)
{
	char *ptr, *orig;
	int ret = -EINVAL;

	orig = skip_sep(*buf);
	if (!orig)
		return ret;
	ptr = strsep(&orig, " \t");
	if (!ptr || !*ptr)
		goto err;

	ret = get_level(ptr, level, NULL);
	if (ret)
		goto err;

	*buf = orig;
	return 0;

err:
	WARN("Could not read level from %s : %s", *buf, strerror(-ret));
	return ret;	
}


/*****************************************************************************/

/* Note: an error might leave some of the fields from args allocated.
 * This is dealt with elsewhere, with a generic FREE_ARG */
static int
fill_args(char *buf, struct veriexec_arg *args)
{
	int ret;
	char *ptr;
	char *tok = buf;

	ret = read_string(&tok, &args->a_fname, &args->a_fname_size);
	if (ret)
		return ret;

	ret = read_uint(&tok, &args->a_ctx);
	if (ret)
		return ret;

	ret = read_flags(&tok, &args->a_flags);
	if (ret)
		return ret;

	ret = read_caps(&tok, &(args->a_caps.v_cap_e));
	if (ret)
		return ret;
	ret = read_caps(&tok, &(args->a_caps.v_cap_p));
	if (ret)
		return ret;
	ret = read_caps(&tok, &(args->a_caps.v_cap_i));
	if (ret)
		return ret;

	ret = read_privs(&tok, &args->a_privs);
	if (ret)
		return ret;

	ret = read_digest_type(&tok, &args->a_dig);
	if (ret)
		return ret;
	
	if ((ptr = strchr(tok, ' ')))
		*ptr = '\0';
	if ((ptr = strchr(tok, '\t')))
		*ptr = '\0';
	if (!*tok) {
		WARN("empty fingerprint");
		return -EINVAL;
	}
	ptr = strdup(tok);
	if (!ptr) {
		WARN("could not allocate fingerprint");
		return -ENOMEM;
	}
	args->a_fp = ptr;
	
	return 0;
}

static int
fill_xargs (char *buf, struct veriexec_xarg *xargs)
{
	int ret;
	char *tok = buf;

	ret = read_uint(&tok, &xargs->a_ctx);
	if (ret)
		return ret;

	ret = read_level(&tok, &xargs->a_lvl);
	if (ret)
		return ret;

	ret = read_caps(&tok, &(xargs->a_capset));
	if (ret)
		return ret;
	
	ret = read_privs(&tok, &xargs->a_privset);

	return ret;
}

/*****************************************************************************/

int 
parse_line(char *line, struct veriexec_arg **node)
{
	struct veriexec_arg *args;
	int retval;

	if (!(args = malloc(sizeof(*args))))
		return -ENOMEM;
	
	if ((retval = fill_args(line, args))) {
		FREE_ARG(args);
		return retval;
	}

	*node = args;
	return 0;
}

int
parse_xline(char *line, struct veriexec_xarg **node)
{
	struct veriexec_xarg *xargs;
	int retval;

	if (!(xargs = malloc(sizeof(*xargs))))
		return -ENOMEM;
	
	if ((retval = fill_xargs(line, xargs))) {
		free(xargs);
		return retval;
	}

	*node = xargs;
	return 0;
}

/*****************************************************************************/

/* Read one line from file, return results in *node if valid */
static int
read_line(FILE *file, struct veriexec_arg **node)
{
	char buf[MAX_LEN];
	char *ptr;

	if (!(fgets(buf, MAX_LEN, file)))
		return -EFAULT;

	if ((ptr = strchr(buf, '#')))
		*ptr = '\0';
	if ((ptr = strchr(buf, '\n')))
		*ptr = '\0';
	
	ptr = buf;
	while (*ptr == ' ' || *ptr == '\t')
		ptr++;

	if (!*ptr) /* empty line */
		return -EAGAIN;

	return parse_line(ptr, node);
}

#define config_error(fmt, args...) fprintf(stderr, "Error parsing %s: " \
					fmt "\n", fname, ## args)
	
int 
parse_config(const char *fname, struct arglist_node **node)
{
	FILE *file;
	struct arglist_node *head, *cur, *tmp;
	int lnum; 
	int ret = 0;
	
	if (*fname == '-' && ! *(fname+1)) 
		file=stdin;
	else 
		if (!(file = fopen(fname, "r"))) {
			config_error("could not open file for reading");
			return -errno;
		}

	if (!(head = malloc(sizeof(struct arglist_node)))) {
		config_error("out of memory");
		goto out_close;
	}
	
	/* Dummy list */
	list_init(head);

	if(!(cur = malloc(sizeof(struct arglist_node)))) {
		config_error("out of memory");
		goto out_free;
	}

	for(lnum = 1; lnum; ++lnum) {
		ret = read_line(file, &(cur->arg));
		switch (ret) 
		{
		case 0:
			push_front(cur, head);

			if(!(cur = malloc(sizeof(struct arglist_node)))) {
				config_error("out of memory");
				goto out_free;
			}
			break;
			
		case -EAGAIN:  /* comment line */
			break;
		case -EFAULT: /* possibly EOF, else error */
			if (feof(file)) goto read_done;
			/* else fall through */
		default:
			config_error("read_line error: %s at line %d", 
					strerror(-ret), lnum);
			goto out_free;
		}
	}

read_done:
	*node = head;
	return 0;

out_free:
	cur = head;
	do {
		tmp = cur;
		cur = cur->next;
		FREE_ARGLIST(tmp);
	} while (cur != head);
out_close:
	fclose(file);
	return ret;

}

#undef config_error
