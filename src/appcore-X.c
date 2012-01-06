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


#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <errno.h>
#include <signal.h>

#include <X11/Xlib.h>
#include <X11/Xatom.h>

#include "appcore-internal.h"

static Atom a_pid;

static pid_t __get_win_pid(Display *d, Window win)
{
	int r;
	pid_t pid;

	Atom a_type;
	int format;
	unsigned long nitems;
	unsigned long bytes_after;
	unsigned char *prop_ret;

	_retv_if(d == NULL || !a_pid, -1);

	prop_ret = NULL;
	r = XGetWindowProperty(d, win, a_pid, 0, 1, False, XA_CARDINAL,
			       &a_type, &format, &nitems, &bytes_after,
			       &prop_ret);
	if (r != Success || prop_ret == NULL)
		return -1;

	if (a_type == XA_CARDINAL && format == 32)
		pid = *(unsigned long *)prop_ret;
	else
		pid = -1;

	XFree(prop_ret);

	return pid;
}

static int __find_win(Display *d, Window *win, pid_t pid)
{
	int r;
	pid_t p;
	unsigned int n;
	Window root, parent, *child;

	p = __get_win_pid(d, *win);
	if (p == pid)
		return 1;

	r = XQueryTree(d, *win, &root, &parent, &child, &n);
	if (r) {
		int i;
		int found = 0;

		for (i = 0; i < n; i++) {
			found = __find_win(d, &child[i], pid);
			if (found) {
				*win = child[i];
				break;
			}
		}
		XFree(child);

		if (found)
			return 1;
	}

	return 0;
}

static int __raise_win(Display *d, Window win)
{
	XWindowAttributes attr;

	XMapRaised(d, win);
	XGetWindowAttributes(d, win, &attr);
	_retv_if(attr.map_state != IsViewable, -1);

	/*
	 * Window Manger sets the Input Focus. 
	 * Appcore does not need to enforce this anymore.
	 *
	 * XSetInputFocus(d, win, RevertToPointerRoot, CurrentTime);
	 *
	 */

	return 0;
}

int x_raise_win(pid_t pid)
{

	int r;
	int found;
	Display *d;
	Window win;

	if (pid < 1) {
		errno = EINVAL;
		return -1;
	}

	r = kill(pid, 0);
	if (r == -1) {
		errno = ESRCH;	/* No such process */
		return -1;
	}

	d = XOpenDisplay(NULL);
	_retv_if(d == NULL, -1);

	win = XDefaultRootWindow(d);

	if (!a_pid)
		a_pid = XInternAtom(d, "_NET_WM_PID", True);

	found = __find_win(d, &win, pid);
	if (!found) {
		XCloseDisplay(d);
		errno = ENOENT;
		return -1;
	}

	r = __raise_win(d, win);

	XCloseDisplay(d);

	return r;
}
