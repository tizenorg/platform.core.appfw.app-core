/*
 *  app-core
 *
 * Copyright (c) 2000 - 2011 Samsung Electronics Co., Ltd. All rights reserved.
 *
 * Contact: Jayoun Lee <airjany@samsung.com>, Sewook Park <sewook7.park@samsung.com>, Jaeho Lee <jaeho81.lee@samsung.com>
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 * http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 *
 */


#include <stdlib.h>
#include <stdio.h>
#include <sys/types.h>
#include <unistd.h>
#include <string.h>
#include <errno.h>

#include <heynoti.h>

#include "appcore-internal.h"

#define NOTI_COMP(a, b) (strcmp(a->name, b->name))

struct noti_s {
	const char *name;
	void *data;
	int (*cb_pre) (void *);
	int (*cb) (void *);
	int (*cb_post) (void *);

	struct noti_s *next;
};
typedef struct noti_s notis;


SGLIB_DEFINE_LIST_PROTOTYPES(notis, NOTI_COMP, next);
SGLIB_DEFINE_LIST_FUNCTIONS(notis, NOTI_COMP, next);

static struct noti_s *noti_h;

static void __main_handle(void *data)
{
	struct noti_s *n = data;

	_ret_if(n == NULL || n->name == NULL);

	_DBG("[APP %d] noti: %s", getpid(), n->name);

	if (n->cb_pre)
		n->cb_pre(n->data);

	if (n->cb)
		n->cb(n->data);

	if (n->cb_post)
		n->cb_post(n->data);
}

static int __del_noti(int fd, struct noti_s *t)
{
	int r;

	r = heynoti_unsubscribe(fd, t->name, __main_handle);
	if (r == 0) {
		sglib_notis_delete(&noti_h, t);
		free((void *)t->name);
		free(t);
	}

	return r;
}

static int __del_notis(int fd)
{
	struct sglib_notis_iterator it;
	struct noti_s *t;

	t = sglib_notis_it_init(&it, noti_h);
	while (t) {
		__del_noti(fd, t);
		t = sglib_notis_it_next(&it);
	}

	return 0;
}

int add_noti(int fd, const char *name, int (*cb_pre) (void *),
	     int (*cb) (void *), int (*cb_post) (void *), void *data)
{
	int r;
	struct noti_s *t;

	if (fd < 0) {
		errno = EBADF;
		return -1;
	}

	if (name == NULL || name[0] == '\0') {
		errno = EINVAL;
		return -1;
	}

	t = calloc(1, sizeof(struct noti_s));
	if (t == NULL)
		return -1;

	t->name = strdup(name);
	if (t->name == NULL) {
		free(t);
		return -1;
	}

	t->cb_pre = cb_pre;
	t->cb = cb;
	t->cb_post = cb_post;
	t->data = data;

	r = heynoti_subscribe(fd, name, __main_handle, t);
	if (r != 0) {
		free((void *)t->name);
		free(t);
		return -1;
	}

	sglib_notis_add(&noti_h, t);

	return r;
}

int del_noti(int fd, const char *name)
{
	struct noti_s t;
	struct noti_s *f;

	if (fd < 0) {
		errno = EBADF;
		return -1;
	}

	t.name = name;
	f = sglib_notis_find_member(noti_h, &t);
	if (f == NULL) {
		errno = ENOENT;
		return -1;
	}

	return __del_noti(fd, f);
}

int noti_start(int fd)
{
	return heynoti_attach_handler(fd);
}

int noti_init(void)
{
	return heynoti_init();
}

void noti_exit(int *fd)
{
	_ret_if(fd == NULL || *fd < 0);

	__del_notis(*fd);

	heynoti_close(*fd);

	*fd = -1;
}
