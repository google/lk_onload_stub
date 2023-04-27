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

/* lk_onload_stub_ext
 *
 * For regression tests only. Not intended to be used directly.
 *
 * Exports symbols so that test binary can build and link.
 * The test then uses LD_PRELOAD with ld_onload_stub to get the
 * "real" (stub) implementation.
 *
 * Therefore all functions here return with error.
 */

#include <sys/epoll.h>

#include "lk_onload_stub_ext.h"

int onload_fd_stat(int fd, void *unused)
{
	return -1;
}

int onload_is_present(void)
{
	return -1;
}

int onload_move_fd(int fd)
{
	return -1;
}

int onload_ordered_epoll_wait(int epfd, struct epoll_event *events,
			      struct onload_ordered_epoll_event *oo_events,
			      int maxevents, int timeout)
{
	return -1;
}

int onload_set_stackname(int who, int scope, const char* stackname)
{
	return -1;
}

int onload_socket_nonaccel(int domain, int type, int protocol)
{
	return -1;
}

int onload_stackname_restore(void)
{
	return -1;
}

int onload_stackname_save(void)
{
	return -1;
}

int onload_stack_opt_get_int(const char* opt, int64_t *val)
{
	return -1;
}

int onload_stack_opt_get_str(const char* opt, char* val_out, size_t* val_out_len)
{
	return -1;
}

int onload_stack_opt_reset(void)
{
	return -1;
}

int onload_stack_opt_set_int(const char* opt, int64_t val)
{
	return -1;
}

int onload_stack_opt_set_str(const char* opt, const char* val)
{
	return -1;
}
