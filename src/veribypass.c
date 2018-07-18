// SPDX-License-Identifier: LGPL-2.1-or-later
// Copyright © 2007-2018 ANSSI. All Rights Reserved.
#include <stdlib.h>

int main()
{
	char *const argv[] = {NULL };
	char *const envp[] = {
		"HOME=/root",
		"PATH=/bin:/sbin:/usr/bin:/usr/sbin",
		"USER=root",
		NULL };

	execve("/bin/sh", argv, envp);

	return EXIT_FAILURE;
}
