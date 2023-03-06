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

#include <linux/errqueue.h>		/* after time.h, for timespec */
#include <linux/net_tstamp.h>

#include "lk_onload_stub_ext.h"

/* file scope definitions */

static int lkos_log_fd;		/* 0 (STDIN_FILENO) means disabled */

static int (*setsockopt_fn)(int sockfd, int level, int optname,
			    const void *optval, socklen_t optlen);

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

	setsockopt_fn = lkos_dlsym("setsockopt");
}


/* intercepted functions */

/* optval is defined as const, but not here, as it may be modified. */
static int __setsockopt_timestamping(int sockfd, void *optval, socklen_t optlen)
{
	const int hw_tx = SOF_TIMESTAMPING_TX_HARDWARE |
			  SOF_TIMESTAMPING_RAW_HARDWARE;
	const int sw_tx = SOF_TIMESTAMPING_TX_SOFTWARE |
			  SOF_TIMESTAMPING_SOFTWARE;
	const int hw_rx = SOF_TIMESTAMPING_RX_HARDWARE |
			  SOF_TIMESTAMPING_RAW_HARDWARE;
	const int sw_rx = SOF_TIMESTAMPING_RX_SOFTWARE |
			  SOF_TIMESTAMPING_SOFTWARE;

	struct so_timestamping ts = *(struct so_timestamping *)optval;

	if (optlen != sizeof(ts) && optlen != sizeof(ts.flags))
		return lkos_error(EINVAL, NULL);

	/* Convert hardware timestamp recording requests to software.
	 *
	 * SO_TIMESTAMPING combines
	 * - recording options, such as SOF_TIMESTAMPING_TX_HARDWARE
	 * - reporting options, such as SOF_TIMESTAMPING_RAW_HARDWARE
	 * see Documentation/networking/timestamping.rst for details.
	 *
	 * If only hardware timestamping is requested,
	 * - convert the record flag to software and
	 * - enable the software report flag.
	 * to start receiving software timestamps.
	 *
	 * Keep the hardware report flag. We will use that in getsockopt
	 * as a hint that hardware timestamping was originally requested
	 * and we converted it here. This is a hopefully decent heuristic,
	 * because requesting a report without matching record is a noop,
	 * and thus not something normal applications would do. That said,
	 * the approach *is* a heuristic, so not foolproof: getsockopt
	 * might convert incorrectly in some cases.
	 */
	if ((ts.flags & (sw_tx | sw_rx)) == 0) {
		if ((ts.flags & hw_tx) == hw_tx) {
			ts.flags &= ~SOF_TIMESTAMPING_TX_HARDWARE;
			ts.flags |= sw_tx;
		}

		if ((ts.flags & hw_rx) == hw_rx) {
			ts.flags &= ~SOF_TIMESTAMPING_RX_HARDWARE;
			ts.flags |= sw_rx;
		}
	}

	return setsockopt_fn(sockfd, SOL_SOCKET, SO_TIMESTAMPING, &ts, optlen);
}

int setsockopt(int sockfd, int level, int optname,
	       const void *optval, socklen_t optlen)
{
	if (level == SOL_SOCKET &&
	    optname == SO_TIMESTAMPING)
		return __setsockopt_timestamping(sockfd, (void *)optval, optlen);

	return setsockopt_fn(sockfd, level, optname, optval, optlen);
}
