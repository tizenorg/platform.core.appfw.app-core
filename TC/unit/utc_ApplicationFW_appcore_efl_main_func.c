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
#include <tet_api.h>
#include <Elementary.h>
#include <appcore-efl.h>

static void startup(void);
static void cleanup(void);

void (*tet_startup)(void) = startup;
void (*tet_cleanup)(void) = cleanup;

static void utc_ApplicationFW_appcore_efl_main_func_01(void);
static void utc_ApplicationFW_appcore_efl_main_func_02(void);

enum {
	POSITIVE_TC_IDX = 0x01,
	NEGATIVE_TC_IDX,
};

struct tet_testlist tet_testlist[] = {
	{ utc_ApplicationFW_appcore_efl_main_func_01, POSITIVE_TC_IDX },
	{ utc_ApplicationFW_appcore_efl_main_func_02, NEGATIVE_TC_IDX },
	{ NULL, 0},
};

static void startup(void)
{
}

static void cleanup(void)
{
}

static int app_reset(bundle *b, void *data)
{
	elm_exit();
	return 0;
}

/**
 * @brief Positive test case of appcore_efl_main()
 */
static void utc_ApplicationFW_appcore_efl_main_func_01(void)
{
	int r = 0;
	int argc = 1;
	char *_argv[] = {
		"Testcase",
		NULL,
	};
	char **argv;
	struct appcore_ops ops = {
		.reset = app_reset,
	};

	argv = _argv;
	r = appcore_efl_main("Testcase", &argc, &argv, &ops);
	printf("Return %d\n", r);
	if (r) {
		tet_infoline("appcore_efl_main() failed in positive test case");
		tet_result(TET_FAIL);
		return;
	}
	tet_result(TET_PASS);
}

/**
 * @brief Negative test case of ug_init appcore_efl_main()
 */
static void utc_ApplicationFW_appcore_efl_main_func_02(void)
{
	int r = 0;
	int argc = 1;
	char *_argv[] = {
		"Testcase",
		NULL,
	};
	char **argv;
	struct appcore_ops ops = {
		.reset = app_reset,
	};

	argv = _argv;
	r = appcore_efl_main("Testcase", &argc, &argv, NULL);
	if (!r) {
		tet_infoline("appcore_efl_main() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}

	r = appcore_efl_main(NULL, &argc, &argv, &ops);
	if (!r) {
		tet_infoline("appcore_efl_main() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}

	r = appcore_efl_main("Testcase", NULL, &argv, &ops);
	if (!r) {
		tet_infoline("appcore_efl_main() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}

	r = appcore_efl_main("Testcase", &argc, NULL, &ops);
	if (!r) {
		tet_infoline("appcore_efl_main() failed in negative test case");
		tet_result(TET_FAIL);
		return;
	}

	tet_result(TET_PASS);
}
