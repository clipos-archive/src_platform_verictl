# SPDX-License-Identifier: LGPL-2.1-or-later
# Copyright © 2007-2018 ANSSI. All Rights Reserved.
dnl Process this file with autoconf to produce a configure script.
AC_INIT(src/verictl.c)

dnl Checks for programs.
AC_PROG_CC

[
if test "x$GCC" = "xyes"; then
  CFLAGS="$CFLAGS -Wall -Wextra -Werror \
	-Wstrict-prototypes -Wmissing-prototypes \
	-Wcast-qual -Wcast-align -Wpointer-arith \
	-Wwrite-strings -Wnested-externs"
fi
]

dnl Check for static option
AC_ARG_ENABLE(static,   
		AC_HELP_STRING([--enable-static], [buidl static binaries]),
			[static=$enableval], [static=no])
AC_SUBST(static)

dnl Check for devctl
AC_ARG_ENABLE(devctl,   
		AC_HELP_STRING([--enable-devctl], [build devctl]),
			[devctl=$enableval], [devctl=no])
AC_SUBST(devctl)

dnl Check for veribypass
AC_ARG_ENABLE(veribypass,   
		AC_HELP_STRING([--enable-veribypass], [build veribypass]),
			[veribypass=$enableval], [veribypass=no])
AC_SUBST(veribypass)

dnl Check for kernel includes
AC_ARG_WITH(kernel-includes, 
	AC_HELP_STRING([--with-kernel-includes=path], 
		[Use path for kernel includes (default: /usr/src/linux/include)]),
		[KERNEL_INCLUDES=$withval],
		[KERNEL_INCLUDES=/usr/src/linux/include])
AC_SUBST(KERNEL_INCLUDES)

dnl Checks for header files.
AC_HEADER_STDC

dnl Checks for typedefs, structures, and compiler characteristics.
AC_C_CONST
AC_TYPE_SIZE_T

AC_OUTPUT(Makefile src/Makefile man/Makefile)
