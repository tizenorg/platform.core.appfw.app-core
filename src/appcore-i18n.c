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


#include <locale.h>
#include <libintl.h>
#include <stdlib.h>
#include <errno.h>

#include <vconf.h>

#include "appcore-internal.h"

static int _set;

void update_lang(void)
{
	char *lang;
	char *r;

	lang = vconf_get_str(VCONFKEY_LANGSET);
	if (lang) {
		setenv("LANG", lang, 1);
		setenv("LC_MESSAGES", lang, 1);
		r = setlocale(LC_ALL, "");
		if (r == NULL) {
			r = setlocale(LC_ALL, vconf_get_str(VCONFKEY_LANGSET));
			_DBG("*****appcore setlocale=%s\n", r);
		}
		free(lang);
	}
}

void update_region(void)
{
	char *region;

	region = vconf_get_str(VCONFKEY_REGIONFORMAT);
	if (region) {
		setenv("LC_CTYPE", region, 1);
		setenv("LC_NUMERIC", region, 1);
		setenv("LC_TIME", region, 1);
		setenv("LC_COLLATE", region, 1);
		setenv("LC_MONETARY", region, 1);
		setenv("LC_PAPER", region, 1);
		setenv("LC_NAME", region, 1);
		setenv("LC_ADDRESS", region, 1);
		setenv("LC_TELEPHONE", region, 1);
		setenv("LC_MEASUREMENT", region, 1);
		setenv("LC_IDENTIFICATION", region, 1);
		free(region);
	}
}

static int __set_i18n(const char *domain, const char *dir)
{
	char *r;

	if (domain == NULL) {
		errno = EINVAL;
		return -1;
	}

	r = setlocale(LC_ALL, "");
	/* if locale is not set properly, try again to set as language base */
	if (r == NULL) {
		r = setlocale(LC_ALL, vconf_get_str(VCONFKEY_LANGSET));
		_DBG("*****appcore setlocale=%s\n", r);
	}
	_retvm_if(r == NULL, -1, "appcore: setlocale() error");

	r = bindtextdomain(domain, dir);
	_retvm_if(r == NULL, -1, "appcore: bindtextdomain() error");

	r = textdomain(domain);
	_retvm_if(r == NULL, -1, "appcore: textdomain() error");

	return 0;
}

EXPORT_API int appcore_set_i18n(const char *domainname, const char *dirname)
{
	int r;

	update_lang();
	update_region();

	r = __set_i18n(domainname, dirname);
	if (r == 0)
		_set = 1;

	return r;
}

int set_i18n(const char *domainname, const char *dirname)
{
	_retv_if(_set, 0);

	update_lang();
	update_region();

	return __set_i18n(domainname, dirname);
}

EXPORT_API int appcore_get_timeformat(enum appcore_time_format *timeformat)
{
	int r;

	if (timeformat == NULL) {
		errno = EINVAL;
		return -1;
	}

	r = vconf_get_int(VCONFKEY_REGIONFORMAT_TIME1224, timeformat);

	if (r < 0) {
		*timeformat = APPCORE_TIME_FORMAT_UNKNOWN;
		return -1;
	} else
		return 0;
}
