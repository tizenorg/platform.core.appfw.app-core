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

#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <stdio.h>
#include <unistd.h>
#include <stdarg.h>
#include <errno.h>
#include <string.h>
#include <stdlib.h>

#ifdef WAYLAND
#include <Ecore_Wayland.h>
#endif

#ifdef X11
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <Ecore_X.h>
#endif

#include <Ecore.h>
#include <Ecore_Evas.h>
#include <Ecore_Input_Evas.h>
#include <Elementary.h>
#include <glib-object.h>
#include <malloc.h>
#include <glib.h>
#include <dbus/dbus.h>
#include <stdbool.h>
#include <aul.h>
#include "appcore-internal.h"
#include "appcore-efl.h"

static pid_t _pid;

static bool resource_reclaiming = TRUE;
static int tmp_val = 0;

enum proc_status_type { /** cgroup command type **/
	PROC_STATUS_LAUNCH,
	PROC_STATUS_RESUME,
	PROC_STATUS_TERMINATE,
	PROC_STATUS_FOREGRD,
	PROC_STATUS_BACKGRD,
};

struct ui_priv {
	const char *name;
	enum app_state state;

	Ecore_Event_Handler *hshow;
	Ecore_Event_Handler *hhide;
	Ecore_Event_Handler *hvchange;
	Ecore_Event_Handler *hcmsg; /* WM_ROTATE */

	Ecore_Timer *mftimer;	/* Ecore Timer for memory flushing */

	struct appcore_ops *ops;
	void (*mfcb) (void);	/* Memory Flushing Callback */

	/* WM_ROTATE */
	int wm_rot_supported;
	int rot_started;
	int (*rot_cb) (void *event_info, enum appcore_rm, void *);
	void *rot_cb_data;
	enum appcore_rm rot_mode;
};

static struct ui_priv priv;

static const char *_ae_name[AE_MAX] = {
	[AE_UNKNOWN] = "UNKNOWN",
	[AE_CREATE] = "CREATE",
	[AE_TERMINATE] = "TERMINATE",
	[AE_PAUSE] = "PAUSE",
	[AE_RESUME] = "RESUME",
	[AE_RESET] = "RESET",
	[AE_LOWMEM_POST] = "LOWMEM_POST",
	[AE_MEM_FLUSH] = "MEM_FLUSH",
};

static const char *_as_name[] = {
	[AS_NONE] = "NONE",
	[AS_CREATED] = "CREATED",
	[AS_RUNNING] = "RUNNING",
	[AS_PAUSED] = "PAUSED",
	[AS_DYING] = "DYING",
};

static bool b_active = FALSE;
static bool first_launch = TRUE;

struct win_node {
	unsigned int win;
	bool bfobscured;
};

static struct ui_wm_rotate wm_rotate;
static Eina_Bool __visibility_cb(void *data, int type, void *event);


static void _send_to_resourced(enum proc_status_type type)
{
	DBusConnection *conn;
	DBusMessage* msg;
	DBusError dbus_error;

	dbus_error_init(&dbus_error);

	conn = dbus_bus_get(DBUS_BUS_SYSTEM, &dbus_error);
	if (!conn) {
		_ERR("dbus_bus_get failed : [%s]", dbus_error.message);
		dbus_error_free(&dbus_error);
		return;
	}

	msg = dbus_message_new_signal("/Org/Tizen/ResourceD/Process",
			"org.tizen.resourced.process",
			"ProcStatus");
	if (!msg) {
		_ERR("dbus_message_new_signal is failed");
		return;
	}

	if (!dbus_message_append_args(msg,
				DBUS_TYPE_INT32, &type,
				DBUS_TYPE_INT32, &_pid,
				DBUS_TYPE_INVALID)) {
		_ERR("dbus_message_append_args is failed. type = %d, pid = %d",
				type, _pid);
		dbus_message_unref(msg);
		return;
	}

	if (!dbus_connection_send (conn, msg, NULL)) {
		_ERR("dbus_connection_send is failed");
	}

	dbus_message_unref(msg);
}

static GSList *g_winnode_list;

#if defined(MEMORY_FLUSH_ACTIVATE)
static Eina_Bool __appcore_memory_flush_cb(void *data)
{
	struct ui_priv *ui = (struct ui_priv *)data;

	appcore_flush_memory();
	ui->mftimer = NULL;

	return ECORE_CALLBACK_CANCEL;
}

static int __appcore_low_memory_post_cb(struct ui_priv *ui)
{
	if (ui->state == AS_PAUSED) {
		appcore_flush_memory();
	} else {
		malloc_trim(0);
	}

	return 0;
}

static void __appcore_timer_add(struct ui_priv *ui)
{
	ui->mftimer = ecore_timer_add(5, __appcore_memory_flush_cb, ui);
}

static void __appcore_timer_del(struct ui_priv *ui)
{
	if (ui->mftimer) {
		ecore_timer_del(ui->mftimer);
		ui->mftimer = NULL;
	}
}

#else

static int __appcore_low_memory_post_cb(ui_priv *ui)
{
	return -1;
}

#define __appcore_timer_add(ui) 0
#define __appcore_timer_del(ui) 0

#endif

static void __appcore_efl_memory_flush_cb(void)
{
	_DBG("[APP %d]   __appcore_efl_memory_flush_cb()", _pid);
	elm_cache_all_flush();
}

static void __do_app(enum app_event event, void *data, bundle * b)
{
	int r = -1;
	struct ui_priv *ui = data;

	_DBG("[APP %d] Event: %d", _pid, event);
	_ret_if(ui == NULL || event >= AE_MAX);
	_DBG("[APP %d] Event: %s State: %s", _pid, _ae_name[event],
	     _as_name[ui->state]);

	if (event == AE_MEM_FLUSH) {
		ui->mfcb();
		return;
	}

	if (event == AE_LOWMEM_POST) {
		if (__appcore_low_memory_post_cb(ui) == 0)
			return;
	}

	if (!(ui->state == AS_PAUSED && event == AE_PAUSE))
		__appcore_timer_del(ui);

	if (event == AE_TERMINATE) {
		_DBG("[APP %d] TERMINATE", _pid);
		ui->state = AS_DYING;
		elm_exit();
		return;
	}

	_ret_if(ui->ops == NULL);

	switch (event) {
	case AE_RESET:
		_DBG("[APP %d] RESET", _pid);
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:reset:start]", ui->name);
		if (ui->ops->reset)
			r = ui->ops->reset(b, ui->ops->data);
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:reset:done]", ui->name);

		if (first_launch) {
			first_launch = FALSE;
			_INFO("[APP %d] Initial Launching, call the resume_cb", _pid);
			if (ui->ops->resume)
				r = ui->ops->resume(ui->ops->data);
		} else {
			_INFO("[APP %d] App already running, raise the window", _pid);
#ifdef X11
			x_raise_win(getpid());
#endif
			if (ui->state == AS_PAUSED) {
				_INFO("[APP %d] Call the resume_cb", _pid);
				if (ui->ops->resume)
					r = ui->ops->resume(ui->ops->data);
			}
		}

		ui->state = AS_RUNNING;
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:reset:done]",
		    ui->name);
		break;
	case AE_PAUSE:
		if (ui->state == AS_RUNNING) {
			_DBG("[APP %d] PAUSE", _pid);
			if (ui->ops->pause)
				r = ui->ops->pause(ui->ops->data);
			ui->state = AS_PAUSED;
			if(r >= 0 && resource_reclaiming == TRUE)
				__appcore_timer_add(ui);
		}
		/* TODO : rotation stop */
		//r = appcore_pause_rotation_cb();
		_send_to_resourced(PROC_STATUS_BACKGRD);
		break;
	case AE_RESUME:
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:resume:start]",
		    ui->name);
		if (ui->state == AS_PAUSED || tmp_val == 1) {
			_DBG("[APP %d] RESUME", _pid);
			if (ui->ops->resume)
				r = ui->ops->resume(ui->ops->data);
			ui->state = AS_RUNNING;
			 tmp_val = 0;
		}
		/*TODO : rotation start*/
		//r = appcore_resume_rotation_cb();
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:resume:done]",
		    ui->name);
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:Launching:done]",
		    ui->name);
		_send_to_resourced(PROC_STATUS_FOREGRD);
		break;
	default:
		/* do nothing */
		break;
	}
}

static struct ui_ops efl_ops = {
	.data = &priv,
	.cb_app = __do_app,
};


static bool __check_visible(void)
{
	GSList *iter = NULL;
	struct win_node *entry = NULL;

	_DBG("[EVENT_TEST][EVENT] __check_visible\n");
	
	for (iter = g_winnode_list; iter != NULL; iter = g_slist_next(iter)) {
		entry = iter->data;	
		_DBG("win : %x obscured : %d\n", entry->win, entry->bfobscured);
		if(entry->bfobscured == FALSE)
			return TRUE;		
	}
	return FALSE;
}

static GSList *__find_win(unsigned int win)
{
	GSList *iter;
	struct win_node *t;

	for (iter = g_winnode_list; iter; iter = g_slist_next(iter)) {
		t = iter->data;
		if (t && t->win == win)
			return iter;
	}

	return NULL;
}

static bool __add_win(unsigned int win)
{
	struct win_node *t;
	GSList *f;

	_DBG("[EVENT_TEST][EVENT] __add_win WIN:%x\n", win);

	f = __find_win(win);
	if (f) {
		errno = ENOENT;
		_DBG("[EVENT_TEST][EVENT] ERROR There is already window : %x \n", win);
		return FALSE;
	}

	t = calloc(1, sizeof(struct win_node));
	if (t == NULL)
		return FALSE;

	t->win = win;
	t->bfobscured = FALSE;

	g_winnode_list = g_slist_append(g_winnode_list, t);

	return TRUE;
}

static bool __delete_win(unsigned int win)
{
	GSList *f;

	f = __find_win(win);
	if (!f) {
		errno = ENOENT;
		_DBG("[EVENT_TEST][EVENT] ERROR There is no window : %x \n",
				win);
		return FALSE;
	}

	g_winnode_list = g_slist_delete_link(g_winnode_list, f);

	free(f->data);

	return TRUE;
}

static bool __update_win(unsigned int win, bool bfobscured)
{
	GSList *f;
	struct win_node *t;

	_DBG("[EVENT_TEST][EVENT] __update_win WIN:%x fully_obscured %d\n", win,
	     bfobscured);

	f = __find_win(win);
	if (!f) {
		errno = ENOENT;
		_DBG("[EVENT_TEST][EVENT] ERROR There is no window : %x \n", win);
		return FALSE;
	}

	g_winnode_list = g_slist_remove_link(g_winnode_list, f);

	t = f->data;
	t->win = win;
	t->bfobscured = bfobscured;

	g_winnode_list = g_slist_concat(g_winnode_list, f);

	return TRUE;

}

/* WM_ROTATE */
#ifdef X11
static Ecore_X_Atom _WM_WINDOW_ROTATION_SUPPORTED = 0;
static Ecore_X_Atom _WM_WINDOW_ROTATION_CHANGE_REQUEST = 0;

static int __check_wm_rotation_support(void)
{
	_DBG("Disable window manager rotation");
	return -1;

	Ecore_X_Window root, win, win2;
	int ret;

	if (!_WM_WINDOW_ROTATION_SUPPORTED) {
		_WM_WINDOW_ROTATION_SUPPORTED =
					ecore_x_atom_get("_E_WINDOW_ROTATION_SUPPORTED");
	}

	if (!_WM_WINDOW_ROTATION_CHANGE_REQUEST) {
		_WM_WINDOW_ROTATION_CHANGE_REQUEST =
					ecore_x_atom_get("_E_WINDOW_ROTATION_CHANGE_REQUEST");
	}

	root = ecore_x_window_root_first_get();
	ret = ecore_x_window_prop_xid_get(root,
			_WM_WINDOW_ROTATION_SUPPORTED,
			ECORE_X_ATOM_WINDOW,
			&win, 1);
	if ((ret == 1) && (win))
	{
		ret = ecore_x_window_prop_xid_get(win,
				_WM_WINDOW_ROTATION_SUPPORTED,
				ECORE_X_ATOM_WINDOW,
				&win2, 1);
		if ((ret == 1) && (win2 == win))
			return 0;
	}

	return -1;
}

static void __set_wm_rotation_support(unsigned int win, unsigned int set)
{
	GSList *iter = NULL;
	struct win_node *entry = NULL;

	if (0 == win) {
		for (iter = g_winnode_list; iter != NULL; iter = g_slist_next(iter)) {
			entry = iter->data;
			if (entry->win) {
				ecore_x_window_prop_card32_set(entry->win,
						_WM_WINDOW_ROTATION_SUPPORTED,
						&set, 1);
			}
		}
	} else {
		ecore_x_window_prop_card32_set(win,
				_WM_WINDOW_ROTATION_SUPPORTED,
				&set, 1);
	}
}

#endif

static Eina_Bool __show_cb(void *data, int type, void *event)
{
#ifdef WAYLAND
	Ecore_Wl_Event_Window_Activate *ev;

	ev = event;

	if (ev->parent_win != 0)
	{
		// This is child window. Skip!!!
		return ECORE_CALLBACK_PASS_ON;
	}

	_DBG("[EVENT_TEST][EVENT] GET SHOW EVENT!!!. WIN:%x\n", ev->win);

	if (!__find_win((unsigned int)ev->win))
		__add_win((unsigned int)ev->win);
	else
		__update_win((unsigned int)ev->win, FALSE);

    __visibility_cb(data, type, event);
#else
	Ecore_X_Event_Window_Show *ev;
	int ret;
	Ecore_X_Window parent;

	ev = event;

	_DBG("[EVENT_TEST][EVENT] GET SHOW EVENT!!!. WIN:%x\n", ev->win);

	if (!__find_win((unsigned int)ev->win)) {
		/* WM_ROTATE */
		if ((priv.wm_rot_supported) && (1 == priv.rot_started)) {
			__set_wm_rotation_support(ev->win, 1);
		}
		__add_win((unsigned int)ev->win);
	}
	else
		__update_win((unsigned int)ev->win, FALSE);
#endif

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool __hide_cb(void *data, int type, void *event)
{
#ifdef WAYLAND
	Ecore_Wl_Event_Window_Deactivate *ev;
	int bvisibility = 0;

	ev = event;

	_DBG("[EVENT_TEST][EVENT] GET HIDE EVENT!!!. WIN:%x\n", ev->win);

	if (__find_win((unsigned int)ev->win)) {
		__delete_win((unsigned int)ev->win);

		bvisibility = __check_visible();
		if (!bvisibility && b_active == TRUE) {
			_DBG(" Go to Pasue state \n");
			b_active = FALSE;
			__do_app(AE_PAUSE, data, NULL);
		}
	}
#else
	Ecore_X_Event_Window_Hide *ev;
	int bvisibility = 0;

	ev = event;

	_DBG("[EVENT_TEST][EVENT] GET HIDE EVENT!!!. WIN:%x\n", ev->win);

	if (__find_win((unsigned int)ev->win)) {
		__delete_win((unsigned int)ev->win);
		bvisibility = __check_visible();
		if (!bvisibility && b_active == TRUE) {
			_DBG(" Go to Pasue state \n");
			b_active = FALSE;
			__do_app(AE_PAUSE, data, NULL);
		}
	}
#endif

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool __visibility_cb(void *data, int type, void *event)
{
#ifdef WAYLAND
	Ecore_Wl_Event_Window_Activate *ev;
	int bvisibility = 0;

	ev = event;

	__update_win((unsigned int)ev->win, ev->fobscured);
#else
	Ecore_X_Event_Window_Visibility_Change *ev;
	int bvisibility = 0;

	ev = event;

	__update_win((unsigned int)ev->win, ev->fully_obscured);
#endif
	bvisibility = __check_visible();

	_DBG("bvisibility %d, b_active %d", bvisibility, b_active);

	if (bvisibility && b_active == FALSE) {
		_DBG(" Go to Resume state\n");
		b_active = TRUE;
		__do_app(AE_RESUME, data, NULL);

	} else if (!bvisibility && b_active == TRUE) {
		_DBG(" Go to Pasue state \n");
		b_active = FALSE;
		__do_app(AE_PAUSE, data, NULL);
	} else
		_DBG(" No change state \n");

	return ECORE_CALLBACK_RENEW;

}

#ifdef X11
/* WM_ROTATE */
static Eina_Bool __cmsg_cb(void *data, int type, void *event)
{
	struct ui_priv *ui = (struct ui_priv *)data;
	Ecore_X_Event_Client_Message *e = event;

	if (!ui) return ECORE_CALLBACK_PASS_ON;
	if (e->format != 32) return ECORE_CALLBACK_PASS_ON;
	if (e->message_type == _WM_WINDOW_ROTATION_CHANGE_REQUEST) {
		if ((0 == ui->wm_rot_supported) ||
			(0 == ui->rot_started) ||
			(NULL == ui->rot_cb)) {
			return ECORE_CALLBACK_PASS_ON;
		}

		enum appcore_rm rm;
		switch (e->data.l[1])
		{
			case   0: rm = APPCORE_RM_PORTRAIT_NORMAL;   break;
			case  90: rm = APPCORE_RM_LANDSCAPE_REVERSE; break;
			case 180: rm = APPCORE_RM_PORTRAIT_REVERSE;  break;
			case 270: rm = APPCORE_RM_LANDSCAPE_NORMAL;  break;
			default:  rm = APPCORE_RM_UNKNOWN;           break;
		}

		ui->rot_mode = rm;

		if (APPCORE_RM_UNKNOWN != rm) {
			ui->rot_cb((void *)&rm, rm, ui->rot_cb_data);
		}
	}

	return ECORE_CALLBACK_PASS_ON;
}
#endif

static void __add_climsg_cb(struct ui_priv *ui)
{
	_ret_if(ui == NULL);
#ifdef WAYLAND
	ui->hshow =
	    ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_ACTIVATE, __show_cb, ui);
	ui->hhide =
	    ecore_event_handler_add(ECORE_WL_EVENT_WINDOW_DEACTIVATE, __hide_cb, ui);
#else
	ui->hshow =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW, __show_cb, ui);
	ui->hhide =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE, __hide_cb, ui);
	ui->hvchange =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE,
				    __visibility_cb, ui);

	/* Add client message callback for WM_ROTATE */
	if(!__check_wm_rotation_support())
	{
		ui->hcmsg =
			ecore_event_handler_add(ECORE_X_EVENT_CLIENT_MESSAGE, __cmsg_cb, ui);
		ui->wm_rot_supported = 1;
		appcore_set_wm_rotation(&wm_rotate);
	}
#endif
}

static int __before_loop(struct ui_priv *ui, int *argc, char ***argv)
{
	int r;
	char *hwacc = NULL;

	if (argc == NULL || argv == NULL) {
		_ERR("argc/argv is NULL");
		errno = EINVAL;
		return -1;
	}

	g_type_init();
	elm_init(*argc, *argv);

	hwacc = getenv("HWACC");

	if(hwacc == NULL) {
		_DBG("elm_config_preferred_engine_set is not called");
	} else if(strcmp(hwacc, "USE") == 0) {
#ifdef WAYLAND
		elm_config_preferred_engine_set("wayland_egl");
		_DBG("elm_config_preferred_engine_set : wayland_egl");
#else
		elm_config_preferred_engine_set("opengl_x11");
		_DBG("elm_config_preferred_engine_set : opengl_x11");
#endif
	} else if(strcmp(hwacc, "NOT_USE") == 0) {
#ifdef WAYLAND
		elm_config_preferred_engine_set("wayland_shm");
		_DBG("elm_config_preferred_engine_set : wayland_shm");
#else
		elm_config_preferred_engine_set("software_x11");
		_DBG("elm_config_preferred_engine_set : software_x11");
#endif
	} else {
		_DBG("elm_config_preferred_engine_set is not called");
	}

	r = appcore_init(ui->name, &efl_ops, *argc, *argv);
	_retv_if(r == -1, -1);

	LOG(LOG_DEBUG, "LAUNCH", "[%s:Platform:appcore_init:done]", ui->name);
	if (ui->ops && ui->ops->create) {
		r = ui->ops->create(ui->ops->data);
		if (r == -1) {
			_ERR("create() return error");
			appcore_exit();
			errno = ECANCELED;
			return -1;
		}
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:create:done]",
		    ui->name);
	}
	ui->state = AS_CREATED;

	__add_climsg_cb(ui);

	return 0;
}

static void __after_loop(struct ui_priv *ui)
{
	appcore_unset_rotation_cb();
	appcore_exit();

	if (ui->state == AS_RUNNING) {
		_DBG("[APP %d] PAUSE before termination", _pid);
		if (ui->ops && ui->ops->pause)
			ui->ops->pause(ui->ops->data);
	}

	if (ui->ops && ui->ops->terminate)
		ui->ops->terminate(ui->ops->data);

	if (ui->hshow)
		ecore_event_handler_del(ui->hshow);
	if (ui->hhide)
		ecore_event_handler_del(ui->hhide);
	if (ui->hvchange)
		ecore_event_handler_del(ui->hvchange);

	__appcore_timer_del(ui);

	elm_shutdown();
}

static int __set_data(struct ui_priv *ui, const char *name,
		    struct appcore_ops *ops)
{
	if (ui->name) {
		_ERR("Mainloop already started");
		errno = EINPROGRESS;
		return -1;
	}

	if (name == NULL || name[0] == '\0') {
		_ERR("Invalid name");
		errno = EINVAL;
		return -1;
	}

	if (ops == NULL) {
		_ERR("ops is NULL");
		errno = EINVAL;
		return -1;
	}

	ui->name = strdup(name);
	_retv_if(ui->name == NULL, -1);

	ui->ops = ops;

	ui->mfcb = __appcore_efl_memory_flush_cb;

	_pid = getpid();

	/* WM_ROTATE */
	ui->wm_rot_supported = 0;
	ui->rot_started = 0;
	ui->rot_cb = NULL;
	ui->rot_cb_data = NULL;
	ui->rot_mode = APPCORE_RM_UNKNOWN;

	return 0;
}

static void __unset_data(struct ui_priv *ui)
{
	if (ui->name)
		free((void *)ui->name);

	memset(ui, 0, sizeof(struct ui_priv));
}

/* WM_ROTATE */
static int __wm_set_rotation_cb(int (*cb) (void *event_info, enum appcore_rm, void *), void *data)
{
	if (cb == NULL) {
		errno = EINVAL;
		return -1;
	}

	if ((priv.wm_rot_supported) && (0 == priv.rot_started)) {
		__set_wm_rotation_support(0, 1);
	}

	priv.rot_cb = cb;
	priv.rot_cb_data = data;
	priv.rot_started = 1;

	return 0;
}

static int __wm_unset_rotation_cb(void)
{
	if ((priv.wm_rot_supported) && (1 == priv.rot_started)) {
		__set_wm_rotation_support(0, 0);
	}

	priv.rot_cb = NULL;
	priv.rot_cb_data = NULL;
	priv.rot_started = 0;

	return 0;
}

static int __wm_get_rotation_state(enum appcore_rm *curr)
{
	if (curr == NULL) {
		errno = EINVAL;
		return -1;
	}

	*curr = priv.rot_mode;

	return 0;
}

static int __wm_pause_rotation_cb(void)
{
	if ((1 == priv.rot_started) && (priv.wm_rot_supported)) {
		__set_wm_rotation_support(0, 0);
	}

	priv.rot_started = 0;

	return 0;
}

static int __wm_resume_rotation_cb(void)
{
	if ((0 == priv.rot_started) && (priv.wm_rot_supported)) {
		__set_wm_rotation_support(0, 1);
	}

	priv.rot_started = 1;

	return 0;
}

static struct ui_wm_rotate wm_rotate = {
	__wm_set_rotation_cb,
	__wm_unset_rotation_cb,
	__wm_get_rotation_state,
	__wm_pause_rotation_cb,
	__wm_resume_rotation_cb
};

EXPORT_API int appcore_efl_main(const char *name, int *argc, char ***argv,
				struct appcore_ops *ops)
{
	int r;

	LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:main:done]", name);

	r = __set_data(&priv, name, ops);
	_retv_if(r == -1, -1);

	r = __before_loop(&priv, argc, argv);
	if (r == -1) {
		__unset_data(&priv);
		return -1;
	}

	elm_run();

	aul_status_update(STATUS_DYING);

	__after_loop(&priv);

	__unset_data(&priv);

	return 0;
}

EXPORT_API int appcore_set_system_resource_reclaiming(bool enable)
{
	resource_reclaiming = enable;

	return 0;
}

EXPORT_API int appcore_set_app_state(int state)
{
	priv.state = state;

	tmp_val = 1;

	return 0;
}
