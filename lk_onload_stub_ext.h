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

#include <time.h>

#ifdef HAVE_ONLOAD
#include <onload/extensions.h>
#else
/* Linux TCP ignores flags it does not expect.
 * It is safe to pass this flag to a tcp socket recv call.
 *
 * This flag does alias a flag in Linux: MSG_SENDPAGE_NOTLAST.
 * Introduced in v3.4 in commit 35f9c09fe9c7
 * ("tcp: tcp_sendpages() should call tcp_push() once")
 * That flag is internal to Linux and only used in sendpage.
 */
#define ONLOAD_MSG_ONEPKT 0x20000

struct onload_ordered_epoll_event {
	struct timespec ts;
	int bytes;
};

int onload_ordered_epoll_wait(int epfd, struct epoll_event *events,
			      struct onload_ordered_epoll_event *oo_events,
			      int maxevents, int timeout);

/* Stacks API */
int onload_move_fd(int fd);
int onload_set_stackname(int who, int scope, const char* stackname);
int onload_stackname_restore(void);
int onload_stackname_save(void);
int onload_stack_opt_get_int(const char* opt, int64_t *val);
int onload_stack_opt_get_str(const char* opt, char* val_out, size_t* val_out_len);
int onload_stack_opt_reset(void);
int onload_stack_opt_set_int(const char* opt, int64_t val);
int onload_stack_opt_set_str(const char* opt, const char* val);

#endif

