/*
 * Copyright (c) 2000 - 2016 Samsung Electronics Co., Ltd. All rights reserved.
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
 */


#define _GNU_SOURCE

#include <errno.h>
#include <string.h>
#include <stdlib.h>
#include <sys/types.h>
#include <unistd.h>
#include <malloc.h>
#include <locale.h>
#include <linux/limits.h>
#include <glib.h>
#include <sys/time.h>
#include <dlfcn.h>
#include <vconf.h>
#include <aul.h>
#include <bundle_internal.h>
#include "appcore-internal.h"

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
#include <gio/gio.h>

#define RESOURCED_FREEZER_PATH "/Org/Tizen/Resourced/Freezer"
#define RESOURCED_FREEZER_INTERFACE "org.tizen.resourced.freezer"
#define RESOURCED_FREEZER_SIGNAL "FreezerState"

int __appcore_init_suspend_dbus_handler(void *data);
void __appcore_fini_suspend_dbus_handler(void);
#endif

#define SQLITE_FLUSH_MAX		(1024*1024)

#define PATH_LOCALE "locale"

static struct appcore core;
static pid_t _pid;

static enum appcore_event to_ae[SE_MAX] = {
	APPCORE_EVENT_UNKNOWN,	/* SE_UNKNOWN */
	APPCORE_EVENT_LOW_MEMORY,	/* SE_LOWMEM */
	APPCORE_EVENT_LOW_BATTERY,	/* SE_LOWBAT */
	APPCORE_EVENT_LANG_CHANGE,	/* SE_LANGCGH */
	APPCORE_EVENT_REGION_CHANGE,
	APPCORE_EVENT_SUSPENDED_STATE_CHANGE,
};

static int appcore_event_initialized[SE_MAX] = {0,};

enum cb_type {			/* callback */
	_CB_NONE,
	_CB_SYSNOTI,
	_CB_APPNOTI,
	_CB_VCONF,
};

struct evt_ops {
	enum cb_type type;
	union {
		enum appcore_event sys;
		enum app_event app;
		const char *vkey;
	} key;

	int (*cb_pre) (void *);
	int (*cb) (void *);
	int (*cb_post) (void *);

	int (*vcb_pre) (void *, void *);
	int (*vcb) (void *, void *);
	int (*vcb_post) (void *, void *);
};

struct open_s {
	int (*callback) (void *);
	void *cbdata;
};

static struct open_s open;

static int __app_terminate(void *data);
static int __app_resume(void *data);
static int __app_reset(void *data, bundle *k);

static int __sys_lowmem_post(void *data, void *evt);
static int __sys_lowmem(void *data, void *evt);
static int __sys_lowbatt(void *data, void *evt);
static int __sys_langchg_pre(void *data, void *evt);
static int __sys_langchg(void *data, void *evt);
static int __sys_regionchg_pre(void *data, void *evt);
static int __sys_regionchg(void *data, void *evt);
extern void aul_finalize();


static struct evt_ops evtops[] = {
	{
	 .type = _CB_VCONF,
	 .key.vkey = VCONFKEY_SYSMAN_LOW_MEMORY,
	 .vcb_post = __sys_lowmem_post,
	 .vcb = __sys_lowmem,
	 },
	{
	 .type = _CB_VCONF,
	 .key.vkey = VCONFKEY_SYSMAN_BATTERY_STATUS_LOW,
	 .vcb = __sys_lowbatt,
	 },
	{
	 .type = _CB_VCONF,
	 .key.vkey = VCONFKEY_LANGSET,
	 .vcb_pre = __sys_langchg_pre,
	 .vcb = __sys_langchg,
	 },
	{
	 .type = _CB_VCONF,
	 .key.vkey = VCONFKEY_REGIONFORMAT,
	 .vcb_pre = __sys_regionchg_pre,
	 .vcb = __sys_regionchg,
	 },
	{
	 .type = _CB_VCONF,
	 .key.vkey = VCONFKEY_REGIONFORMAT_TIME1224,
	 .vcb = __sys_regionchg,
	 },
};

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
static GDBusConnection *bus;
static guint __suspend_dbus_handler_initialized;
#endif

static int __get_locale_resource_dir(char *locale_dir, int size)
{
	const char *res_path;

	res_path = aul_get_app_resource_path();
	if (res_path == NULL) {
		_ERR("Failed to get resource path");
		return -1;
	}

	snprintf(locale_dir, size, "%s" PATH_LOCALE, res_path);
	if (access(locale_dir, R_OK) != 0)
		return -1;

	return 0;
}

static int __app_terminate(void *data)
{
	struct appcore *ac = data;

	_retv_if(ac == NULL || ac->ops == NULL, -1);
	_retv_if(ac->ops->cb_app == NULL, 0);

	ac->ops->cb_app(AE_TERMINATE, ac->ops->data, NULL);

	return 0;
}

static int __bgapp_terminate(void *data)
{
	struct appcore *ac = data;

	_retv_if(ac == NULL || ac->ops == NULL, -1);
	_retv_if(ac->ops->cb_app == NULL, 0);

	ac->ops->cb_app(AE_TERMINATE_BGAPP, ac->ops->data, NULL);

	return 0;
}

static gboolean __prt_ltime(gpointer data)
{
	int msec;

	msec = appcore_measure_time_from(NULL);
	if (msec)
		_DBG("[APP %d] first idle after reset: %d msec", _pid, msec);

	return FALSE;
}

static int __app_reset(void *data, bundle * k)
{
	struct appcore *ac = data;
	_retv_if(ac == NULL || ac->ops == NULL, -1);
	_retv_if(ac->ops->cb_app == NULL, 0);

	g_idle_add(__prt_ltime, ac);

	ac->ops->cb_app(AE_RESET, ac->ops->data, k);

	return 0;
}

static int __app_resume(void *data)
{
	struct appcore *ac = data;
	_retv_if(ac == NULL || ac->ops == NULL, -1);
	_retv_if(ac->ops->cb_app == NULL, 0);

	ac->ops->cb_app(AE_RAISE, ac->ops->data, NULL);
	return 0;
}

static int __app_pause(void *data)
{
	struct appcore *ac = data;
	_retv_if(ac == NULL || ac->ops == NULL, -1);
	_retv_if(ac->ops->cb_app == NULL, 0);

	ac->ops->cb_app(AE_LOWER, ac->ops->data, NULL);
	return 0;
}

static int __sys_do_default(struct appcore *ac, enum sys_event event)
{
	int r;

	switch (event) {
	case SE_LOWBAT:
		/*r = __def_lowbatt(ac);*/
		r = 0;
		break;
	default:
		r = 0;
		break;
	};

	return r;
}

static int __sys_do(struct appcore *ac, void *event_info, enum sys_event event)
{
	struct sys_op *op;

	_retv_if(ac == NULL || event >= SE_MAX, -1);

	op = &ac->sops[event];

	if (op->func == NULL)
		return __sys_do_default(ac, event);

	return op->func(event_info, op->data);
}

static int __sys_lowmem_post(void *data, void *evt)
{
	keynode_t *key = evt;
	int val;

	val = vconf_keynode_get_int(key);

	if (val >= VCONFKEY_SYSMAN_LOW_MEMORY_SOFT_WARNING)	{
#if defined(MEMORY_FLUSH_ACTIVATE)
		struct appcore *ac = data;
		ac->ops->cb_app(AE_LOWMEM_POST, ac->ops->data, NULL);
#else
		malloc_trim(0);
#endif
	}
	return 0;
}

static int __sys_lowmem(void *data, void *evt)
{
	keynode_t *key = evt;
	int val;

	val = vconf_keynode_get_int(key);

	if (val >= VCONFKEY_SYSMAN_LOW_MEMORY_SOFT_WARNING)
		return __sys_do(data, (void *)&val, SE_LOWMEM);

	return 0;
}

static int __sys_lowbatt(void *data, void *evt)
{
	keynode_t *key = evt;
	int val;

	val = vconf_keynode_get_int(key);

	/* VCONFKEY_SYSMAN_BAT_CRITICAL_LOW or VCONFKEY_SYSMAN_POWER_OFF */
	if (val <= VCONFKEY_SYSMAN_BAT_CRITICAL_LOW)
		return __sys_do(data, (void *)&val, SE_LOWBAT);

	return 0;
}

static int __sys_langchg_pre(void *data, void *evt)
{
	update_lang();
	return 0;
}

static int __sys_langchg(void *data, void *evt)
{
	keynode_t *key = evt;
	char *val;

	val = vconf_keynode_get_str(key);

	return __sys_do(data, (void *)val, SE_LANGCHG);
}

static int __sys_regionchg_pre(void *data, void *evt)
{
	update_region();
	return 0;
}

static int __sys_regionchg(void *data, void *evt)
{
	keynode_t *key = evt;
	char *val = NULL;
	const char *name;

	name = vconf_keynode_get_name(key);
	if (!strcmp(name, VCONFKEY_REGIONFORMAT))
		val = vconf_keynode_get_str(key);

	return __sys_do(data, (void *)val, SE_REGIONCHG);
}

static void __vconf_do(struct evt_ops *eo, keynode_t * key, void *data)
{
	_ret_if(eo == NULL);

	if (eo->vcb_pre)
		eo->vcb_pre(data, key);

	if (eo->vcb)
		eo->vcb(data, key);

	if (eo->vcb_post)
		eo->vcb_post(data, key);
}

static void __vconf_cb(keynode_t *key, void *data)
{
	int i;
	const char *name;

	name = vconf_keynode_get_name(key);
	_ret_if(name == NULL);

	_DBG("[APP %d] vconf changed: %s", _pid, name);

	for (i = 0; i < sizeof(evtops) / sizeof(evtops[0]); i++) {
		struct evt_ops *eo = &evtops[i];

		switch (eo->type) {
		case _CB_VCONF:
			if (!strcmp(name, eo->key.vkey))
				__vconf_do(eo, key, data);
			break;
		default:
			/* do nothing */
			break;
		}
	}
}

static int __add_vconf(struct appcore *ac, enum sys_event se)
{
	int r;

	switch (se) {
	case SE_LOWMEM:
		r = vconf_notify_key_changed(VCONFKEY_SYSMAN_LOW_MEMORY, __vconf_cb, ac);
		break;
	case SE_LOWBAT:
		r = vconf_notify_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, __vconf_cb, ac);
		break;
	case SE_LANGCHG:
		r = vconf_notify_key_changed(VCONFKEY_LANGSET, __vconf_cb, ac);
		break;
	case SE_REGIONCHG:
		r = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT, __vconf_cb, ac);
		if (r < 0)
			break;

		r = vconf_notify_key_changed(VCONFKEY_REGIONFORMAT_TIME1224, __vconf_cb, ac);
		break;
	default:
		r = -1;
		break;
	}

	return r;
}

static int __del_vconf(enum sys_event se)
{
	int r;

	switch (se) {
	case SE_LOWMEM:
		r = vconf_ignore_key_changed(VCONFKEY_SYSMAN_LOW_MEMORY, __vconf_cb);
		break;
	case SE_LOWBAT:
		r = vconf_ignore_key_changed(VCONFKEY_SYSMAN_BATTERY_STATUS_LOW, __vconf_cb);
		break;
	case SE_LANGCHG:
		r = vconf_ignore_key_changed(VCONFKEY_LANGSET, __vconf_cb);
		break;
	case SE_REGIONCHG:
		r = vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT, __vconf_cb);
		if (r < 0)
			break;

		r = vconf_ignore_key_changed(VCONFKEY_REGIONFORMAT_TIME1224, __vconf_cb);
		break;
	default:
		r = -1;
		break;
	}

	return r;
}

static int __del_vconf_list(void)
{
	int r;
	enum sys_event se;

	for (se = SE_LOWMEM; se < SE_MAX; se++) {
		if (appcore_event_initialized[se]) {
			r = __del_vconf(se);
			if (r < 0)
				_ERR("Delete vconf callback failed");
			else
				appcore_event_initialized[se] = 0;
		}
	}

	return 0;
}

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
static gboolean __flush_memory(gpointer data)
{
	int suspend = APPCORE_SUSPENDED_STATE_WILL_ENTER_SUSPEND;
	struct appcore *ac = (struct appcore *)data;

	appcore_flush_memory();

	if (!ac)
		return FALSE;

	ac->tid = 0;

	if (!ac->allowed_bg && !ac->suspended_state) {
		_DBG("[__SUSPEND__] flush case");
		__sys_do(ac, &suspend, SE_SUSPENDED_STATE);
		ac->suspended_state = true;
	}

	return FALSE;
}

static void __add_suspend_timer(struct appcore *ac)
{
	ac->tid = g_timeout_add_seconds(5, __flush_memory, ac);
}

static void __remove_suspend_timer(struct appcore *ac)
{
	if (ac->tid > 0) {
		g_source_remove(ac->tid);
		ac->tid = 0;
	}
}
#endif

static int __aul_handler(aul_type type, bundle *b, void *data)
{
	int ret;
	const char **tep_path = NULL;
	int len = 0;
	int i;
#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
	const char *bg = NULL;
	struct appcore *ac = data;
#endif

	switch (type) {
	case AUL_START:
		_DBG("[APP %d]     AUL event: AUL_START", _pid);
		tep_path = bundle_get_str_array(b, AUL_TEP_PATH, &len);
		if (tep_path) {
			for (i = 0; i < len; i++) {
				ret = aul_check_tep_mount(tep_path[i]);
				if (ret == -1) {
					_ERR("mount request not completed within 1 sec");
					exit(-1);
				}
			}
		}

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
		bg = bundle_get_val(b, AUL_K_ALLOWED_BG);
		if (bg && strncmp(bg, "ALLOWED_BG", strlen("ALLOWED_BG")) == 0) {
			_DBG("[__SUSPEND__] allowed background");
			ac->allowed_bg = true;
			__remove_suspend_timer(data);
		}
#endif

		__app_reset(data, b);
		break;
	case AUL_RESUME:
		_DBG("[APP %d]     AUL event: AUL_RESUME", _pid);
#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
		bg = bundle_get_val(b, AUL_K_ALLOWED_BG);
		if (bg && strncmp(bg, "ALLOWED_BG", strlen("ALLOWED_BG")) == 0) {
			_DBG("[__SUSPEND__] allowed background");
			ac->allowed_bg = true;
			__remove_suspend_timer(data);
		}
#endif

		if (open.callback) {
			ret = open.callback(open.cbdata);
			if (ret == 0)
				__app_resume(data);
		} else {
			__app_resume(data);
		}
		break;
	case AUL_TERMINATE:
		_DBG("[APP %d]     AUL event: AUL_TERMINATE", _pid);
#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
		if (!ac->allowed_bg)
			__remove_suspend_timer(data);
#endif

		__app_terminate(data);
		break;
	case AUL_TERMINATE_BGAPP:
		_DBG("[APP %d]     AUL event: AUL_TERMINATE_BGAPP", _pid);
#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
		if (!ac->allowed_bg)
			__remove_suspend_timer(data);
#endif

		__bgapp_terminate(data);
		break;
	case AUL_PAUSE:
		_DBG("[APP %d]	   AUL event: AUL_PAUSE", _pid);
		__app_pause(data);
		break;
#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
	case AUL_WAKE:
		_DBG("[APP %d]     AUL event: AUL_WAKE", _pid);
		if (!ac->allowed_bg && ac->suspended_state) {
			int suspend = APPCORE_SUSPENDED_STATE_DID_EXIT_FROM_SUSPEND;
			__remove_suspend_timer(data);
			__sys_do(ac, &suspend, SE_SUSPENDED_STATE);
			ac->suspended_state = false;
		}
		break;
	case AUL_SUSPEND:
		_DBG("[APP %d]     AUL event: AUL_SUSPEND", _pid);
		if (!ac->allowed_bg && !ac->suspended_state) {
			__remove_suspend_timer(data);
			__flush_memory((gpointer)ac);
		}
		break;
#endif
	default:
		_DBG("[APP %d]     AUL event: %d", _pid, type);
		/* do nothing */
		break;
	}

	return 0;
}


static void __clear(struct appcore *ac)
{
	memset(ac, 0, sizeof(struct appcore));
}

EXPORT_API void appcore_get_app_core(struct appcore **ac)
{
	*ac = &core;
}

EXPORT_API int appcore_set_open_cb(int (*cb) (void *),
				       void *data)
{
	open.callback = cb;
	open.cbdata = data;

	return 0;
}

EXPORT_API int appcore_set_event_callback(enum appcore_event event,
					  int (*cb) (void *, void *), void *data)
{
	struct appcore *ac = &core;
	struct sys_op *op;
	enum sys_event se;
	int r = 0;

	for (se = SE_UNKNOWN; se < SE_MAX; se++) {
		if (event == to_ae[se])
			break;
	}

	if (se == SE_UNKNOWN || se >= SE_MAX) {
		_ERR("Unregistered event");
		errno = EINVAL;
		return -1;
	}

	op = &ac->sops[se];

	op->func = cb;
	op->data = data;

	if (op->func && !appcore_event_initialized[se]) {
		r = __add_vconf(ac, se);
		if (r < 0)
			_ERR("Add vconf callback failed");
		else
			appcore_event_initialized[se] = 1;
	} else if (!op->func && appcore_event_initialized[se]) {
		r = __del_vconf(se);
		if (r < 0)
			_ERR("Delete vconf callback failed");
		else
			appcore_event_initialized[se] = 0;
	}

	return 0;
}

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
static gboolean __init_suspend(gpointer data)
{
	int r;

	r = __appcore_init_suspend_dbus_handler(&core);
	if (r == -1) {
		_ERR("Initailzing suspended state handler failed");
	}

	return FALSE;
}
#endif

EXPORT_API int appcore_init(const char *name, const struct ui_ops *ops,
			    int argc, char **argv)
{
	int r;
	char locale_dir[PATH_MAX];

	if (core.state != 0) {
		_ERR("Already in use");
		errno = EALREADY;
		return -1;
	}

	if (ops == NULL || ops->cb_app == NULL) {
		_ERR("ops or callback function is null");
		errno = EINVAL;
		return -1;
	}

	r = __get_locale_resource_dir(locale_dir, sizeof(locale_dir));
	r = set_i18n(name, locale_dir);
	_retv_if(r == -1, -1);

	r = aul_launch_init(__aul_handler, &core);
	if (r < 0) {
		_ERR("Aul init failed: %d", r);
		goto err;
	}

	r = aul_launch_argv_handler(argc, argv);
	if (r < 0) {
		_ERR("Aul argv handler failed: %d", r);
		goto err;
	}

	core.ops = ops;
	core.state = 1;		/* TODO: use enum value */
	core.tid = 0;
	core.suspended_state = false;
	core.allowed_bg = false;

	_pid = getpid();

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
	g_idle_add(__init_suspend, NULL);
#endif

	return 0;
 err:
	__del_vconf_list();
	__clear(&core);
	return -1;
}

EXPORT_API void appcore_exit(void)
{
	if (core.state) {
		__del_vconf_list();
		__clear(&core);
#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
		__remove_suspend_timer(&core);
		__appcore_fini_suspend_dbus_handler();
#endif
	}
	aul_finalize();
}

EXPORT_API int appcore_flush_memory(void)
{
	int (*flush_fn) (int);

	struct appcore *ac = &core;

	if (!core.state) {
		_ERR("Appcore not initialized");
		return -1;
	}

	_DBG("[APP %d] Flushing memory ...", _pid);

	if (ac->ops->cb_app)
		ac->ops->cb_app(AE_MEM_FLUSH, ac->ops->data, NULL);

	flush_fn = dlsym(RTLD_DEFAULT, "sqlite3_release_memory");
	if (flush_fn)
		flush_fn(SQLITE_FLUSH_MAX);

	malloc_trim(0);
	/*
	*Disabled - the impact of stack_trim() is unclear
	*stack_trim();
	*/

	_DBG("[APP %d] Flushing memory DONE", _pid);

	return 0;
}

#ifdef _APPFW_FEATURE_BACKGROUND_MANAGEMENT
static void __suspend_dbus_signal_handler(GDBusConnection *connection,
					const gchar *sender_name,
					const gchar *object_path,
					const gchar *interface_name,
					const gchar *signal_name,
					GVariant *parameters,
					gpointer user_data)
{
	struct appcore *ac = (struct appcore *)user_data;
	gint suspend = APPCORE_SUSPENDED_STATE_DID_EXIT_FROM_SUSPEND;
	gint pid;
	gint status;

	if (g_strcmp0(signal_name, RESOURCED_FREEZER_SIGNAL) == 0) {
		g_variant_get(parameters, "(ii)", &status, &pid);
		if (pid == getpid() && status == 0) { /* thawed */
			if (ac && !ac->allowed_bg && ac->suspended_state) {
				__remove_suspend_timer(ac);
				__sys_do(ac, &suspend, SE_SUSPENDED_STATE);
				ac->suspended_state = false;
				__add_suspend_timer(ac);
			}
		}
	}
}

int __appcore_init_suspend_dbus_handler(void *data)
{
	GError *err = NULL;

	if (__suspend_dbus_handler_initialized)
		return 0;

	if (!bus) {
		bus = g_bus_get_sync(G_BUS_TYPE_SYSTEM, NULL, &err);
		if (!bus) {
			_ERR("Failed to connect to the D-BUS daemon: %s",
						err->message);
			g_error_free(err);
			return -1;
		}
	}

	__suspend_dbus_handler_initialized = g_dbus_connection_signal_subscribe(
						bus,
						NULL,
						RESOURCED_FREEZER_INTERFACE,
						RESOURCED_FREEZER_SIGNAL,
						RESOURCED_FREEZER_PATH,
						NULL,
						G_DBUS_SIGNAL_FLAGS_NONE,
						__suspend_dbus_signal_handler,
						data,
						NULL);
	if (__suspend_dbus_handler_initialized == 0) {
		_ERR("g_dbus_connection_signal_subscribe() is failed.");
		return -1;
	}

	_DBG("[__SUSPEND__] suspend signal initialized");

	return 0;
}

void __appcore_fini_suspend_dbus_handler(void)
{
	if (bus == NULL)
		return;

	if (__suspend_dbus_handler_initialized) {
		g_dbus_connection_signal_unsubscribe(bus,
				__suspend_dbus_handler_initialized);
		__suspend_dbus_handler_initialized = 0;
	}

	g_object_unref(bus);
	bus = NULL;
}
#endif

