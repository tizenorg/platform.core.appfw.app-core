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


#include <errno.h>
#include <stdlib.h>
#include <unistd.h>

#include <sensor.h>
#include <vconf.h>

#include "appcore-internal.h"

#define LCD_TYPE_KEY "memory/sensor/lcd_type"

struct rot_s {
	int handle;
	int (*callback) (enum appcore_rm, void *);
	enum appcore_rm mode;
	int lock;
	void *cbdata;
	int cb_set;
	int sf_started;
};
static struct rot_s rot;

struct rot_evt {
	enum accelerometer_rotate_state re;
	enum appcore_rm rm;
};

static struct rot_evt re_to_rm[] = {
	{
	 ROTATION_EVENT_0,
	  APPCORE_RM_PORTRAIT_NORMAL,
	},
	{
	 ROTATION_EVENT_90,
	 APPCORE_RM_LANDSCAPE_NORMAL,
	},
	{
	 ROTATION_EVENT_180,
	 APPCORE_RM_PORTRAIT_REVERSE,
	},
	{
	 ROTATION_EVENT_270,
	 APPCORE_RM_LANDSCAPE_REVERSE,
	},
};


EXPORT_API int appcore_set_rotation_cb(int (*cb) (enum appcore_rm, void *),
				       void *data)
{
	return 0;
}

EXPORT_API int appcore_unset_rotation_cb(void)
{
	return 0;
}

EXPORT_API int appcore_get_rotation_state(enum appcore_rm *curr)
{
	return 0;
}

EXPORT_API int appcore_pause_rotation_cb(void)
{
	return 0;
}

EXPORT_API int appcore_resume_rotation_cb(void)
{
	return 0;
}
