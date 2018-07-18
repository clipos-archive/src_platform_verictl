// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  parse.h - verictl parse header
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */
#ifndef _VERICONFIG_PARSE_H
#define _VERICONFIG_PARSE_H

#include "maps.h"

struct arglist_node;
struct veriexec_arg;
struct veriexec_xarg;

int get_level (const char *, unsigned int *, unsigned int *);

int parse_line (char *, struct veriexec_arg **);
int parse_xline (char *, struct veriexec_xarg **);
int parse_config (const char *, struct arglist_node **);

#endif /*_VERICONFIG_PARSE_H*/
