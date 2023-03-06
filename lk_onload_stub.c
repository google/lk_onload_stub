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
#include <limits.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <stdarg.h>
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

static int lkos_log_fd;		/* 0 (STDIN_FILENO) means disabled */

/* library support functions */

static void lkos_log(const char *fmt, ...)
{
	if (lkos_log_fd) {
		va_list args;

		va_start(args, fmt);
		vdprintf(lkos_log_fd, fmt, args);
		va_end(args);
	}
}

static int __attribute__((unused)) __lkos_error(int err, const char *msg, const char *fn, int lineno)
{
	if (msg)
		lkos_log("%s.%d: %s\n", fn, lineno, msg);

	errno = err;
	return 1;
}
#define lkos_error(err, msg) __lkos_error(err, msg, __func__, __LINE__)

static void * __attribute__((used)) lkos_dlsym(const char *symbol_str)
{
	void *fn;

	fn = dlsym(RTLD_NEXT, symbol_str);
	if (!fn) {
		lkos_log("%s: %s: %s\n", __func__, symbol_str, dlerror());
		exit(1);
	}

	return fn;
};

static void lkos_init_log(void)
{
	unsigned long fd_val;
	const char *fd_str;

	fd_str = getenv("LKOS_LOG_FD");
	if (!fd_str)
		return;

	fd_val = strtoul(fd_str, NULL, 0);
	if (fd_val > INT_MAX)
		return;

	lkos_log_fd = (int) fd_val;

	lkos_log("lk_onload_stub loaded\n");
}

static void __attribute__((constructor)) lkos_init(void)
{
	lkos_init_log();
}


/* intercepted functions */


