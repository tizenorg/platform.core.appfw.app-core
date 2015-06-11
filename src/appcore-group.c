#include <Elementary.h>
#include <stdio.h>
#include <glib.h>
#include <aul.h>
#include <pkgmgr-info.h>
#include <bundle_internal.h>

#include "appcore-internal.h"

#define APP_SVC_K_LAUNCH_MODE   "__APP_SVC_LAUNCH_MODE__"

static int __get_top_window(int lpid)
{
	int *gpids;
	int gcnt;
	int ret = -1;

	aul_app_group_get_group_pids(lpid, &gcnt, &gpids);
	if (gcnt > 0) {
		ret =  aul_app_group_get_window(gpids[gcnt-1]);
	}

	if (gpids != NULL)
		free(gpids);

	return ret;
}

static gboolean __can_attach_window(bundle *b)
{
	char *str = NULL;
	char *mode = NULL;
	char appid[255] = {0, };
	int ret;

	pkgmgrinfo_appinfo_h handle;
	ret = aul_app_get_appid_bypid(getpid(), appid, sizeof(appid));
	if (ret != AUL_R_OK) {
		_ERR("Failed to aul_app_get_appid_bypid()");
		return FALSE;
	}

	ret = pkgmgrinfo_appinfo_get_usr_appinfo(appid, getuid(), &handle);
	if (ret != PMINFO_R_OK) {
		_ERR("Failed to pkgmgrinfo_appinfo_get_appinfo()");
		return FALSE;
	}
	ret = pkgmgrinfo_appinfo_get_launch_mode(handle, &mode);

	if (ret != PMINFO_R_OK) {
		pkgmgrinfo_appinfo_destroy_appinfo(handle);
		_ERR("Failed to pkgmgrinfo_appinfo_get_launch_mode()");
		return FALSE;
	}

	if (mode != NULL && strncmp(mode, "caller", 6) == 0) {
		_DBG("launch mode from db is caller");

		bundle_get_str(b, APP_SVC_K_LAUNCH_MODE, &str);
		if (str != NULL && strncmp(str, "group", 5) == 0) {
			pkgmgrinfo_appinfo_destroy_appinfo(handle);
			return TRUE;
		}
	} else if (mode != NULL && strncmp(mode, "group", 5) == 0) {
		pkgmgrinfo_appinfo_destroy_appinfo(handle);
		return TRUE;
	}

	pkgmgrinfo_appinfo_destroy_appinfo(handle);

	return FALSE;
}

void appcore_group_reset(bundle *b)
{
	_DBG("appcore_group_reset");
	int wid = appcore_get_main_window();

	if (__can_attach_window(b)) {
		_DBG("attach!!");
		int lpid;
		const char *val = NULL;
		int caller_pid;
		int caller_wid;

		val = bundle_get_val(b, AUL_K_CALLER_PID);

		if (val != NULL) {
			caller_pid = atoi(val);
			lpid = aul_app_group_get_leader_pid(caller_pid);

			if (lpid != -1) {
				caller_wid = __get_top_window(lpid);
			_DBG("lpid %d, getpid() %d, wid %d, caller_wid %d",
					lpid, getpid(), wid, caller_wid);
				aul_app_group_add(lpid, getpid(), wid);
				aul_app_group_attach_window(caller_wid, wid);
			} else {
				_ERR("no lpid");
				elm_exit();
			}
		} else {
			_ERR("caller pid is null");
		}
	} else {
		int pid = getpid();
		aul_app_group_add(pid, pid, wid);
	}
}

void appcore_group_resume()
{
	_DBG("appcore_group_resume");
	aul_app_group_clear_top();
}

