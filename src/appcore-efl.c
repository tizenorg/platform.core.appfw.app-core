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
#include <string.h>
#include <stdlib.h>
#include <X11/Xatom.h>
#include <X11/Xlib.h>
#include <Ecore_X.h>
#include <Elementary.h>
#include <glib-object.h>
#include <malloc.h>
#include <sysman.h>
#include <glib.h>
#include <stdbool.h>
#include "appcore-internal.h"
#include "appcore-efl.h"

static pid_t _pid;

static bool resource_reclaiming = TRUE;

struct ui_priv {
	const char *name;
	enum app_state state;

	Ecore_Event_Handler *hshow;
	Ecore_Event_Handler *hhide;
	Ecore_Event_Handler *hvchange;

	Ecore_Timer *mftimer;	/* Ecore Timer for memory flushing */

	struct appcore_ops *ops;
	void (*mfcb) (void);	/* Memory Flushing Callback */
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

static bool b_active = 0;
struct win_node {
	unsigned int win;
	bool bfobscured;
};

static int WIN_COMP(gconstpointer data1, gconstpointer data2)
{
	struct win_node *a = (struct win_node *)data1;
	struct win_node *b = (struct win_node *)data2;
	return (int)((a->win)-(b->win));
}

GSList *g_winnode_list;

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
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:reset:start]",
		    ui->name);
		if (ui->ops->reset)
			r = ui->ops->reset(b, ui->ops->data);
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

		sysman_inform_backgrd();
		break;
	case AE_RESUME:
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:resume:start]",
		    ui->name);
		if (ui->state == AS_PAUSED) {
			_DBG("[APP %d] RESUME", _pid);
			if (ui->ops->resume)
				r = ui->ops->resume(ui->ops->data);
			ui->state = AS_RUNNING;
		}
		/*TODO : rotation start*/
		//r = appcore_resume_rotation_cb();
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:resume:done]",
		    ui->name);
		LOG(LOG_DEBUG, "LAUNCH", "[%s:Application:Launching:done]",
		    ui->name);
		sysman_inform_foregrd();

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

static bool __exist_win(unsigned int win)
{
	struct win_node temp;
	GSList *f;

	temp.win = win;

	f = g_slist_find_custom(g_winnode_list, &temp, WIN_COMP);
	if (f == NULL) {
		return FALSE;
	} else {
		return TRUE;
	}

}

static bool __add_win(unsigned int win)
{
	struct win_node *t;
	GSList *f;

	t = calloc(1, sizeof(struct win_node));
	if (t == NULL)
		return FALSE;

	t->win = win;
	t->bfobscured = FALSE;

	_DBG("[EVENT_TEST][EVENT] __add_win WIN:%x\n", win);

	f = g_slist_find_custom(g_winnode_list, t, WIN_COMP);

	if (f) {
		errno = ENOENT;
		_DBG("[EVENT_TEST][EVENT] ERROR There is already window : %x \n", win);
		free(t);
		return 0;
	}

	g_winnode_list = g_slist_append(g_winnode_list, t);

	return TRUE;

}

static bool __delete_win(unsigned int win)
{
	struct win_node temp;
	GSList *f;

	temp.win = win;

	f = g_slist_find_custom(g_winnode_list, &temp, WIN_COMP);
	if (f == NULL) {
		errno = ENOENT;
		_DBG("[EVENT_TEST][EVENT] ERROR There is no window : %x \n",
		     win);
		return 0;
	}

	g_winnode_list = g_slist_remove_link(g_winnode_list, f);

	free(f->data);

	return TRUE;
}

static bool __update_win(unsigned int win, bool bfobscured)
{
	struct win_node temp;
	GSList *f;

	struct win_node *t;

	_DBG("[EVENT_TEST][EVENT] __update_win WIN:%x fully_obscured %d\n", win,
	     bfobscured);

	temp.win = win;

	f = g_slist_find_custom(g_winnode_list, &temp, WIN_COMP);

	if (f == NULL) {
		errno = ENOENT;
		_DBG("[EVENT_TEST][EVENT] ERROR There is no window : %x \n", win);
		return FALSE;
	}

	g_winnode_list = g_slist_remove_link(g_winnode_list, f);

	free(f->data);

	t = calloc(1, sizeof(struct win_node));
	if (t == NULL)
		return FALSE;

	t->win = win;
	t->bfobscured = bfobscured;

	g_winnode_list = g_slist_append(g_winnode_list, t);
	
	return TRUE;

}

Ecore_X_Atom atom_parent;

static Eina_Bool __show_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Show *ev;
	int ret;
	Ecore_X_Window parent;

	ev = event;

	ret = ecore_x_window_prop_window_get(ev->win, atom_parent, &parent, 1);
	if (ret != 1)
	{
		// This is child window. Skip!!!
		return ECORE_CALLBACK_PASS_ON;
	}

	_DBG("[EVENT_TEST][EVENT] GET SHOW EVENT!!!. WIN:%x\n", ev->win);

	if (!__exist_win((unsigned int)ev->win))
		__add_win((unsigned int)ev->win);
	else
		__update_win((unsigned int)ev->win, FALSE);

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool __hide_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Hide *ev;
	int bvisibility = 0;

	ev = event;

	_DBG("[EVENT_TEST][EVENT] GET HIDE EVENT!!!. WIN:%x\n", ev->win);

	if (__exist_win((unsigned int)ev->win)) {
		__delete_win((unsigned int)ev->win);
		
		bvisibility = __check_visible();
		if (!bvisibility && b_active == 1) {
			_DBG(" Go to Pasue state \n");
			b_active = 0;
			__do_app(AE_PAUSE, data, NULL);
		}
	}

	return ECORE_CALLBACK_RENEW;
}

static Eina_Bool __visibility_cb(void *data, int type, void *event)
{
	Ecore_X_Event_Window_Visibility_Change *ev;
	int bvisibility = 0;

	ev = event;

	__update_win((unsigned int)ev->win, ev->fully_obscured);
	bvisibility = __check_visible();

	if (bvisibility && b_active == 0) {
		_DBG(" Go to Resume state\n");
		b_active = 1;
		__do_app(AE_RESUME, data, NULL);

	} else if (!bvisibility && b_active == 1) {
		_DBG(" Go to Pasue state \n");
		b_active = 0;
		__do_app(AE_PAUSE, data, NULL);
	} else
		_DBG(" No change state \n");

	return ECORE_CALLBACK_RENEW;

}

static void __add_climsg_cb(struct ui_priv *ui)
{
	_ret_if(ui == NULL);

	atom_parent = ecore_x_atom_get("_E_PARENT_BORDER_WINDOW");
	if (!atom_parent)
	{
		// Do Error Handling
	}

	ui->hshow =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_SHOW, __show_cb, ui);
	ui->hhide =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_HIDE, __hide_cb, ui);
	ui->hvchange =
	    ecore_event_handler_add(ECORE_X_EVENT_WINDOW_VISIBILITY_CHANGE,
				    __visibility_cb, ui);

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
		elm_config_preferred_engine_set("opengl_x11");
		_DBG("elm_config_preferred_engine_set : opengl_x11");
	} else if(strcmp(hwacc, "NOT_USE") == 0) {
		elm_config_preferred_engine_set("software_x11");
		_DBG("elm_config_preferred_engine_set : software_x11");
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

	return 0;
}

static void __unset_data(struct ui_priv *ui)
{
	if (ui->name)
		free((void *)ui->name);

	memset(ui, 0, sizeof(struct ui_priv));
}

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

	__after_loop(&priv);

	__unset_data(&priv);

	return 0;
}

EXPORT_API int appcore_set_system_resource_reclaiming(bool enable)
{
	resource_reclaiming = enable;

	return 0;
}
