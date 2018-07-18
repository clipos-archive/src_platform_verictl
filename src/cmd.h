// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
/* 
 *  cmd.h - verictl commands header
 *  Copyright (C) 2006-2009 SGDN/DCSSI
 *  Copyright (C) 2014 SGDSN/ANSSI
 *  Author: Vincent Strubel <clipos@ssi.gouv.fr>
 *
 *  All rights reserved.
 *
 */


#ifndef _VERICONFIG_CMD_H
#define _VERICONFIG_CMD_H

extern int
do_line(char *, int);

extern int
do_config(const char *, int);

extern int
do_ctx(char *, int);

extern int
do_setlvl(int);

extern int
do_printlvl(int);

extern int
do_setupdate(unsigned int, int);

extern int
do_memchk(int);

#endif /* _VERICONFIG_CMD_H */
