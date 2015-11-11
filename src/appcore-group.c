#include <Elementary.h>
#include <stdio.h>
#include <glib.h>
#include <aul.h>
#include <pkgmgr-info.h>
#include <bundle_internal.h>

#include "appcore-internal.h"

void appcore_group_attach()
{
	_DBG("appcore_group_attach");
	static bool attached = false;

	if (attached)
		return;

#ifdef X11
	int wid = appcore_get_main_window();
#else
	int wid = appcore_get_main_surface();
#endif
	if (wid == 0) {
		_ERR("window wasn't ready");
		return;
	}

	aul_app_group_set_window(wid);
	attached = true;
}

void appcore_group_lower()
{
	_DBG("appcore_group_lower");
	int exit = 0;

	aul_app_group_lower(&exit);
	if (exit) {
		_DBG("appcore_group_lower : sub-app!");
		elm_exit();
	}
}
