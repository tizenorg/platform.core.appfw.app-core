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
#include <sys/time.h>

#include "appcore-internal.h"

static struct timeval tv_s;	/* measure start */

static inline int __get_msec(struct timeval *s, struct timeval *e)
{
	return (e->tv_sec - s->tv_sec) * 1000 +
	    (e->tv_usec - s->tv_usec + 500) / 1000;
}

static int __get_time(struct timeval *s)
{
	struct timeval t;

	_retv_if(s == NULL || (s->tv_sec == 0 && s->tv_usec == 0), 0);

	gettimeofday(&t, NULL);

	return __get_msec(s, &t);
}

static int __get_envtime(const char *name, struct timeval *t)
{
	int r;
	char *s;

	s = getenv(name ? : ENV_START);
	/*_retvm_if(s == NULL, -1, "%s is not set", name);*/
	_retv_if(s == NULL, -1);

	r = sscanf(s, "%u/%u", (int *)&t->tv_sec, (int *)&t->tv_usec);
	if (r != 2)
		r = sscanf(s, "%u %u", (int *)&t->tv_sec, (int *)&t->tv_usec);

	_retv_if(r != 2, -1);

	return 0;
}

static int __get_time_from(const char *name)
{
	int r;
	struct timeval s;
	struct timeval t;

	gettimeofday(&t, NULL);

	r = __get_envtime(name, &s);
	_retv_if(r == -1, 0);

	return __get_msec(&s, &t);
}

EXPORT_API int appcore_measure_time_from(const char *envnm)
{
	return __get_time_from(envnm);
}

EXPORT_API int appcore_measure_time(void)
{
	return __get_time(&tv_s);
}

EXPORT_API void appcore_measure_start(void)
{
	gettimeofday(&tv_s, NULL);
}
