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
#include <Ecore_X.h>
#include <Ecore.h>
#include <X11/Xlib.h>

#include "appcore-internal.h"

#define _MAKE_ATOM(a, s)                              \
   do {                                               \
        a = ecore_x_atom_get(s);                      \
        if (!a)                                       \
          _ERR("##s creation failed.\n");             \
   } while(0)

#define STR_ATOM_ROTATION_LOCK                "_E_ROTATION_LOCK"

static Ecore_X_Atom ATOM_ROTATION_LOCK = 0;
static Ecore_X_Window root;

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

static enum appcore_rm changed_m;
static void *changed_data;
static Ecore_Event_Handler *changed_handle;

static enum appcore_rm __get_mode(int event_data)
{
	int i;
	enum appcore_rm m;

	m = APPCORE_RM_UNKNOWN;

	for (i = 0; i < sizeof(re_to_rm) / sizeof(re_to_rm[0]); i++) {
		if (re_to_rm[i].re == event_data) {
			m = re_to_rm[i].rm;
			break;
		}
	}

	return m;
}

static Eina_Bool __property(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Property *ev = event;

	if (!ev)
		return ECORE_CALLBACK_PASS_ON;

	if (ev->atom == ATOM_ROTATION_LOCK) {
		_DBG("[APP %d] Rotation: %d -> %d, cb_set : %d", getpid(), rot.mode, changed_m, rot.cb_set);
		if (rot.cb_set && rot.mode != changed_m) {
			rot.callback(changed_m, changed_data);
			rot.mode = changed_m;
		}

		ecore_event_handler_del(changed_handle);
		changed_handle = NULL;
	}

	return ECORE_CALLBACK_PASS_ON;
}

static void __changed_cb(unsigned int event_type, sensor_event_data_t *event,
		       void *data)
{
	int *cb_event_data;
	enum appcore_rm m;
	int ret;
	int val;

	if (rot.lock)
		return;

	if (event_type != ACCELEROMETER_EVENT_ROTATION_CHECK) {
		errno = EINVAL;
		return;
	}

	cb_event_data = (int *)(event->event_data);

	m = __get_mode(*cb_event_data);

	_DBG("[APP %d] Rotation: %d -> %d", getpid(), rot.mode, m);

	if (rot.callback) {
		if (rot.cb_set && rot.mode != m) {
			ret = ecore_x_window_prop_card32_get(root, ATOM_ROTATION_LOCK, &val, 1);

			_DBG("[APP %d] Rotation: %d -> %d, val : %d, ret : %d", getpid(), rot.mode, m, val, ret);
			if (!val || ret < 1) {
				rot.callback(m, data);
				rot.mode = m;
			} else {
				changed_data = data;
				if(changed_handle) {
					 ecore_event_handler_del(changed_handle);
  					 changed_handle = NULL;
				}
				changed_handle = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, __property, NULL);
			}
		}
		changed_m = m;
	}
}

static void __lock_cb(keynode_t *node, void *data)
{
	int r;
	enum appcore_rm m;
	int ret;
	int val;

	rot.lock = vconf_keynode_get_bool(node);

	if (rot.lock) {
		_DBG("[APP %d] Rotation locked", getpid());
		return;
	}

	_DBG("[APP %d] Rotation unlocked", getpid());
	if (rot.callback) {
		if (rot.cb_set) {
			r = appcore_get_rotation_state(&m);
			_DBG("[APP %d] Rotmode prev %d -> curr %d", getpid(),
			     rot.mode, m);
			if (!r && rot.mode != m) {
				if(!val) {
					rot.callback(m, data);
					rot.mode = m;
				} else {
					changed_data = data;
					if(changed_handle) {
						 ecore_event_handler_del(changed_handle);
  						 changed_handle = NULL;
					}
					changed_handle = ecore_event_handler_add(ECORE_X_EVENT_WINDOW_PROPERTY, __property, NULL);
				}
			}

			if(!r)
				changed_m = m;
		}
	}
}

static void __add_rotlock(void *data)
{
	int r;
	int lock;

	lock = 0;
	r = vconf_get_bool(VCONFKEY_SETAPPL_ROTATE_LOCK_BOOL, &lock);
	if (r) {
		_DBG("[APP %d] Rotation vconf get bool failed", getpid());
	}

	rot.lock = lock;

	vconf_notify_key_changed(VCONFKEY_SETAPPL_ROTATE_LOCK_BOOL, __lock_cb,
				 data);
}

static void __del_rotlock(void)
{
	vconf_ignore_key_changed(VCONFKEY_SETAPPL_ROTATE_LOCK_BOOL, __lock_cb);
	rot.lock = 0;
}

EXPORT_API int appcore_set_rotation_cb(int (*cb) (enum appcore_rm, void *),
				       void *data)
{
	int r;
	int handle;

	if (cb == NULL) {
		errno = EINVAL;
		return -1;
	}

	if (rot.callback != NULL) {
		errno = EALREADY;
		return -1;
	}

	handle = sf_connect(ACCELEROMETER_SENSOR);
	if (handle < 0) {
		_ERR("sf_connect failed: %d", handle);
		return -1;
	}

	r = sf_register_event(handle, ACCELEROMETER_EVENT_ROTATION_CHECK,
			      NULL, __changed_cb, data);
	if (r < 0) {
		_ERR("sf_register_event failed: %d", r);
		sf_disconnect(handle);
		return -1;
	}

	rot.cb_set = 1;
	rot.callback = cb;
	rot.cbdata = data;

	r = sf_start(handle, 0);
	if (r < 0) {
		_ERR("sf_start failed: %d", r);
		sf_unregister_event(handle, ACCELEROMETER_EVENT_ROTATION_CHECK);
		rot.callback = NULL;
		rot.cbdata = NULL;
		rot.cb_set = 0;
		rot.sf_started = 0;
		sf_disconnect(handle);
		return -1;
	}
	rot.sf_started = 1;

	rot.handle = handle;
	__add_rotlock(data);

	_MAKE_ATOM(ATOM_ROTATION_LOCK, STR_ATOM_ROTATION_LOCK );
	root =  ecore_x_window_root_first_get();
	XSelectInput(ecore_x_display_get(), root, PropertyChangeMask);

	return 0;
}

EXPORT_API int appcore_unset_rotation_cb(void)
{
	int r;

	_retv_if(rot.callback == NULL, 0);

	__del_rotlock();

	if (rot.cb_set) {
		r = sf_unregister_event(rot.handle,
					ACCELEROMETER_EVENT_ROTATION_CHECK);
		if (r < 0) {
			_ERR("sf_unregister_event failed: %d", r);
			return -1;
		}
		rot.cb_set = 0;
	}
	rot.callback = NULL;
	rot.cbdata = NULL;

	if (rot.sf_started == 1) {
		r = sf_stop(rot.handle);
		if (r < 0) {
			_ERR("sf_stop failed: %d", r);
			return -1;
		}
		rot.sf_started = 0;
	}

	r = sf_disconnect(rot.handle);
	if (r < 0) {
		_ERR("sf_disconnect failed: %d", r);
		return -1;
	}
	rot.handle = -1;

	return 0;
}

EXPORT_API int appcore_get_rotation_state(enum appcore_rm *curr)
{
	int r;
	unsigned long event;

	if (curr == NULL) {
		errno = EINVAL;
		return -1;
	}

	r = sf_check_rotation(&event);
	if (r < 0) {
		_ERR("sf_check_rotation failed: %d", r);
		*curr = APPCORE_RM_UNKNOWN;
		return -1;
	}

	*curr = __get_mode(event);

	return 0;
}

EXPORT_API int appcore_pause_rotation_cb(void)
{
	int r;

	_retv_if(rot.callback == NULL, 0);
	_DBG("[APP %d] appcore_pause_rotation_cb is called", getpid());

	__del_rotlock();

	if (rot.cb_set) {
		r = sf_unregister_event(rot.handle,
					ACCELEROMETER_EVENT_ROTATION_CHECK);
		if (r < 0) {
			_ERR("sf_unregister_event in appcore_internal_sf_stop failed: %d", r);
			return -1;
		}
		rot.cb_set = 0;
	}

	if (rot.sf_started == 1) {
		r = sf_stop(rot.handle);
		if (r < 0) {
			_ERR("sf_stop in appcore_internal_sf_stop failed: %d",
			     r);
			return -1;
		}
		rot.sf_started = 0;
	}

	return 0;
}

EXPORT_API int appcore_resume_rotation_cb(void)
{
	int r;
	enum appcore_rm m;

	_retv_if(rot.callback == NULL, 0);
	_DBG("[APP %d] appcore_resume_rotation_cb is called", getpid());

	if (rot.cb_set == 0) {
		r = sf_register_event(rot.handle,
				      ACCELEROMETER_EVENT_ROTATION_CHECK, NULL,
				      __changed_cb, rot.cbdata);
		if (r < 0) {
			_ERR("sf_register_event in appcore_internal_sf_start failed: %d", r);
			return -1;
		}
		rot.cb_set = 1;
	}

	if (rot.sf_started == 0) {
		r = sf_start(rot.handle, 0);
		if (r < 0) {
			_ERR("sf_start in appcore_internal_sf_start failed: %d",
			     r);
			sf_unregister_event(rot.handle,
					    ACCELEROMETER_EVENT_ROTATION_CHECK);
			rot.cb_set = 0;
			return -1;
		}
		rot.sf_started = 1;
	}

	__add_rotlock(rot.cbdata);

	r = appcore_get_rotation_state(&m);
	_DBG("[APP %d] Rotmode prev %d -> curr %d", getpid(), rot.mode, m);
	if (!r && rot.mode != m && rot.lock == 0) {
		rot.callback(m, rot.cbdata);
		rot.mode = m;
	}

	return 0;
}
