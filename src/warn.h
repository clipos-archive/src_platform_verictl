// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  warn.h - verictl warning macros
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */
#ifndef _VERICTLWARN_H
#define _VERICTLWARN_H

#include <string.h>
#include <errno.h>

#define WARN(fmt, args...) \
	fprintf(stderr, "%s: Warning: " fmt"\n", __FUNCTION__, ## args)

#define WARN_ERRNO(fmt, args...) \
	WARN(fmt": %s", ##args, strerror(errno))

#endif /*_VERICTLWARN_H */
