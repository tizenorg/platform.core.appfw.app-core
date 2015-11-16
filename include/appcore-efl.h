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



#ifndef __APPCORE_ELF_H__
#define __APPCORE_ELF_H__

/**
 * @file    appcore-efl.h
 * @version 1.1
 * @brief   This file contains APIs of the Appcore EFL library
 */

/**
 * @addtogroup APPLICATION_FRAMEWORK
 * @{
 *
 * @defgroup Appcore_EFL Appcore EFL
 * @version  1.1
 * @brief    A base library for EFL application based on Appcore
 *
 */

/**
 * @addtogroup Appcore_EFL
 * @{
 */

#include <stdbool.h>
#include <appcore-common.h>

#ifdef __cplusplus
extern "C" {
#endif

int appcore_efl_init(const char *name, int *argc, char ***argv,
		     struct appcore_ops *ops);

void appcore_efl_fini(void);


/**
 * @par Description:
 * This is a main function for EFL application on SLP platform. \n
 * Refer to programming guide for the details.
 *
 * @par Purpose:
 * To develop an application using EFL on this platform.
 *
 * @par Method of function operation:
 * Initialize the EFL, internationalization, and notifications
 * for receiving system events such as rotation, low battery, etc.\n
 * And, start an ecore mainloop.
 *
 * @param[in] name Application name
 * @param[in] argc A count of the arguments
 * @param[in] argv An array of pointers to the strings which are those arguments
 * @param[in] ops Appcore operations
 *
 * @return 0 on success, -1 on error (<I>errno</I> set)
 *
 * @par Errors:
 * EALREADY - mainloop already started \n
 * EINVAL - one of parameters is NULL \n
 * ECANCELED - create() callback returns error (none zero value) \n
 *
 * @par Corner cases/exceptions:
 * If <I>ops</I> has no callback function, this function just starts a mainloop.
 *
 * @par Known issues/bugs:
 * If <I>errno</I> set another value, check the <I>dlog</I> message.
 * This doesn't care internal <I>errno</I> set.
 *
 * @pre None.
 * @post None.
 * @see None.
 * @remarks None.
 *
 * @par Sample code:
 * @code
#include <appcore-efl.h>

static int _create(void *);
static int _reset(bundle *, void *);

int main(int argc, char *argv[])
{
	int r;
	const char *name;
	struct appcore_ops ops = {
		.create = _create,
		.reset = _reset,
		...
	};

	...

	r = appcore_efl_main(name, &argc, &argv, &ops);
	if (r) {
		// add exception handling
		perror("Appcore EFL main");
	}
}
 * @endcode
 *
 */
int appcore_efl_main(const char *name, int *argc, char ***argv,
		     struct appcore_ops *ops);

/**
 * @par Description:
 * This API sets system resource reclaiming.
 *
 * @par Purpose:
 * When this is not enabled, the application doesn't reclaim system resource.
 *
 * @param[in] enable Application name
 *
 * @return 0 on success, -1 on error (<I>errno</I> set)
 *
 *
 * @pre None.
 * @post None.
 * @see None.
 * @remarks None.
 *
 * @par Sample code:
 * @code
#include <appcore-efl.h>

int _create(void *data)
{
	int r;

	...
	r = appcore_set_system_resource_reclaiming(FALSE);
	if (r) {
		// add exception handling
		perror("appcore_set_system_resource_reclaiming");
	}
	...
}
 * @endcode
 *
 */
int appcore_set_system_resource_reclaiming(bool enable);

int appcore_set_preinit_window_name(const char *win_name);

#ifdef __cplusplus
}
#endif
/**
 * @}
 */
/**
 * @}
 */
#endif /* __APPCORE_ELF_H__ */
