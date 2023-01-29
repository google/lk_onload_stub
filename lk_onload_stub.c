/*
 * Copyright 2023 Google LLC
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * version 2 as published by the Free Software Foundation.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE. See the
 * GNU General Public License for more details.
 */

#define _GNU_SOURCE

#include <stddef.h>
#include <dlfcn.h>
#include <error.h>
#include <errno.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <stdbool.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <time.h>
#include <unistd.h>

#include "lk_onload_stub_ext.h"

/* file scope definitions */


/* library support functions */

static int __attribute__((unused)) __lkos_error(int err, const char *msg, const char *fn, int lineno)
{
	if (msg)
		fprintf(stderr, "%s.%d: %s\n", fn, lineno, msg);

	errno = err;
	return 1;
}
#define lkos_error(err, msg) __lkos_error(err, msg, __func__, __LINE__)

static void * __attribute__((used)) lkos_dlsym(const char *symbol_str)
{
	void *fn;

	fn = dlsym(RTLD_NEXT, symbol_str);
	if (!fn)
		error(1, 0, "%s: %s: %s", __func__, symbol_str, dlerror());

	return fn;
};

static void __attribute__((constructor)) lkos_init(void)
{
	fprintf(stderr, "lk_onload_stub loaded\n");
}


/* intercepted functions */


