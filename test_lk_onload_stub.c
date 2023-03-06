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

#include <dlfcn.h>
#include <error.h>
#include <errno.h>
#include <errno.h>
#include <fcntl.h>
#include <linux/errqueue.h>
#include <linux/net_tstamp.h>
#include <netinet/ip.h>
#include <netinet/ip6.h>
#include <netinet/udp.h>
#include <stdbool.h>
#include <stddef.h>
#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <sys/ioctl.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>
#include <unistd.h>

#include "lk_onload_stub_ext.h"

static bool has_preload;

/* library support functions */

static int __fail_errno(const char *fn, int line)
{
	fprintf(stderr, "%s.%d: %d (%s)\n", fn, line, errno, strerror(errno));
	return 1;
}
#define fail_errno() __fail_errno(__func__, __LINE__)

static int __fail_str(const char *fn, int line, const char *str)
{
	fprintf(stderr, "%s.%d: %s\n", fn, line, str);
	return 1;
}
#define fail_str(s) __fail_str(__func__, __LINE__, s)


/* test functions */

static int test_dlsym(void)
{
	ssize_t (*vmsplice_fn)(int fd, const struct iovec *iov,
			       size_t nr_segs, unsigned int flags);

	/* verify that dlsym returns failure on missing symbol */
	if (dlsym(RTLD_NEXT, "vmsplice_doesnotexist"))
		return fail_str("dlsym: unexpected non-NULL");

	/* verify that dlsym return success on a real symbol */
	vmsplice_fn = dlsym(RTLD_NEXT, "vmsplice");
	if (vmsplice_fn == NULL)
		return fail_str("dlsym: unexpected NULL");

	/* verify that the result from dlsym is likely sane */
	if (vmsplice_fn(-1, NULL, 0, 0) != -1)
		return fail_str("dlsym: unexpected success");
	if (errno != EBADF)
		return fail_errno();

	return 0;
}

static int socketpair_open(int domain, int type, int *fdt_p, int *fdr_p)
{
	struct sockaddr_in addr4 = {0};
	struct sockaddr_in6 addr6 = {0};
	struct sockaddr *addr;
	socklen_t alen;
	int fdt, fdr;

	fdt = socket(domain, type, 0);
	if (fdt == -1)
		return fail_errno();
	fdr = socket(domain, type, 0);
	if (fdr == -1)
		return fail_errno();

	if (domain == PF_INET6) {
		addr6.sin6_family = domain;
		addr6.sin6_port = 0;
		addr6.sin6_addr = in6addr_loopback;
		alen = sizeof(addr6);
		addr = (void *)&addr6;
	} else {
		addr4.sin_family = domain;
		addr4.sin_port = 0;
		addr4.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
		alen = sizeof(addr4);
		addr = (void *)&addr4;
	}
	if (bind(fdr, addr, alen))
		return fail_errno();
	if (getsockname(fdr, addr, &alen))
		return fail_errno();
	if (type == SOCK_STREAM) {
		if (listen(fdr, 1))
			return fail_errno();
	}
	if (connect(fdt, addr, alen))
		return fail_errno();

	if (type == SOCK_STREAM) {
		int fdl = fdr;

		fdr = accept(fdl, NULL, NULL);
		if (fdr == -1)
			return fail_errno();
		if (close(fdl))
			return fail_errno();
	}

	*fdt_p = fdt;
	*fdr_p = fdr;

	return 0;
}

static int test_recv_msg_onepkt(int domain, int type)
{
	char rxbuf[2];
	int fdt, fdr, ret;

	ret = socketpair_open(domain, type, &fdt, &fdr);
	if (ret)
		return ret;

	if (write(fdt, "a", 1) != 1)
		return fail_errno();
	if (recv(fdr, rxbuf, sizeof(rxbuf), 0) != 1)
		return fail_errno();

	if (write(fdt, "a", 1) != 1)
		return fail_errno();
	if (recv(fdr, rxbuf, sizeof(rxbuf), ONLOAD_MSG_ONEPKT) != 1)
		return fail_errno();

	if (close(fdr))
		return fail_errno();
	if (close(fdt))
		return fail_errno();

	return 0;
}

static int test_setsockopt_timestamping_ctrl(int domain, int type)
{
	int fd, val;

	fd = socket(domain, type, 0);
	if (fd == -1)
		return fail_errno();

	/* setsockopt will register request for timestamp types, even when
	 * hardware lacks support and will not generate the timestamps.
	 */
	val = SOF_TIMESTAMPING_SOFTWARE |
	      SOF_TIMESTAMPING_RX_SOFTWARE |
	      SOF_TIMESTAMPING_TX_SOFTWARE |
	      SOF_TIMESTAMPING_RAW_HARDWARE |
	      SOF_TIMESTAMPING_TX_HARDWARE |
	      SOF_TIMESTAMPING_RX_HARDWARE;

	if (setsockopt(fd, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(val)))
		return fail_errno();

	if (close(fd))
		return fail_errno();

	return 0;
}

static int recvmsg_tstamp_cmsg(struct msghdr *msg)
{
	struct scm_timestamping *tss;
	struct cmsghdr *cm;

	for (cm = CMSG_FIRSTHDR(msg); cm; cm = CMSG_NXTHDR(msg, cm)) {
		if (cm->cmsg_level == SOL_SOCKET &&
		    cm->cmsg_type == SCM_TIMESTAMPING)
			tss = (void *) CMSG_DATA(cm);
	}

	fprintf(stderr, "%s: sw=%lu.%lu hw=%lu.%lu\n",
		__func__,
		tss->ts[0].tv_sec, tss->ts[0].tv_nsec / 1000UL,
		tss->ts[2].tv_sec, tss->ts[2].tv_nsec / 1000UL);

	if (has_preload) {
		if (tss->ts[0].tv_sec != tss->ts[2].tv_sec ||
		    tss->ts[0].tv_nsec != tss->ts[2].tv_nsec)
			return fail_str("hw stamp does not match sw tstamp");
	} else {
		if (tss->ts[2].tv_sec || tss->ts[2].tv_nsec)
			return fail_str("non-zero hw stamp on loopback");
	}

	return 0;
}

static int recvmsg_tstamp(int fd, int flags)
{
        char ctrl[CMSG_SPACE(sizeof(struct scm_timestamping)) +
                  CMSG_SPACE(sizeof(struct sock_extended_err)) +
                  CMSG_SPACE(sizeof(struct sockaddr_in6))] = {0};
	struct msghdr msg = {0};
	struct iovec iov;
	char data[2];
	int ret;

	iov.iov_base = data;
	iov.iov_len = sizeof(data);

	msg.msg_iov = &iov;
	msg.msg_iovlen = 1;

	msg.msg_control = ctrl;
	msg.msg_controllen = sizeof(ctrl);

	ret = recvmsg(fd, &msg, flags);
	if (ret == -1)
		return fail_errno();
	if (flags == 0 && ret != 1)
		return fail_str("recvmsg: wrong length: expect 1 on rx");
	if (flags == MSG_ERRQUEUE && ret != 0)
		return fail_str("recvmsg: wrong length: expect 0 on tx TSONLY");

	ret = recvmsg_tstamp_cmsg(&msg);
	if (ret)
		return ret;

	return 0;
}

/* Request timestamps from the loopback device
 *
 * Loopback does not support hardware timestamps
 * - verify that these are not returned without has_preload
 * - verify that these are the same as sw with has_preload
 */
static int test_setsockopt_timestamping_data(int domain, int type)
{
	int fdt, fdr, ret, val;

	ret = socketpair_open(domain, type, &fdt, &fdr);
	if (ret)
		return ret;

	val = SOF_TIMESTAMPING_SOFTWARE |
	      SOF_TIMESTAMPING_RX_SOFTWARE |
	      SOF_TIMESTAMPING_TX_SOFTWARE |
	      SOF_TIMESTAMPING_RAW_HARDWARE |
	      SOF_TIMESTAMPING_TX_HARDWARE |
	      SOF_TIMESTAMPING_RX_HARDWARE |
	      SOF_TIMESTAMPING_OPT_TSONLY;

	if (setsockopt(fdt, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(val)))
		return fail_errno();
	if (setsockopt(fdr, SOL_SOCKET, SO_TIMESTAMPING, &val, sizeof(val)))
		return fail_errno();

	/* wait for static_branch netstamp_needed_key to be enabled */
	usleep(10 * 1000);

	if (write(fdt, "a", 1) != 1)
		return fail_errno();

	ret = recvmsg_tstamp(fdr, 0);
	if (ret)
		return ret;

	ret = recvmsg_tstamp(fdt, MSG_ERRQUEUE);
	if (ret)
		return ret;

	if (close(fdr))
		return fail_errno();
	if (close(fdt))
		return fail_errno();

	return 0;
}

int main(int argc, char **argv)
{
	const int domains[] = { PF_INET, PF_INET6, 0 }, *p_domain;
	const int types[] = { SOCK_STREAM, SOCK_DGRAM, 0 }, *p_type;
	int ret = 0;

	has_preload = getenv("LD_PRELOAD");

	ret |= test_dlsym();

	for (p_domain = domains; *p_domain; p_domain++) {
		for (p_type = types; *p_type; p_type++) {
			ret |= test_recv_msg_onepkt(*p_domain, *p_type);
			ret |= test_setsockopt_timestamping_ctrl(*p_domain, *p_type);
			ret |= test_setsockopt_timestamping_data(*p_domain, *p_type);
		}
	}

	return !!ret;
}

