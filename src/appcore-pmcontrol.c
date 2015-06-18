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
#include <unistd.h>
#include <errno.h>

#include <pmapi.h>

#include "appcore-internal.h"

EXPORT_API int appcore_lock_power_state(int s_bits)
{
	switch (s_bits) {
	case LCD_NORMAL:
		return pm_lock_state(s_bits, GOTO_STATE_NOW, 0);

	case LCD_DIM:
		if (pm_lock_state(s_bits, STAY_CUR_STATE, 0) < 0)
			return -1;
		return pm_change_state(LCD_NORMAL);

	case LCD_OFF:
		return pm_lock_state(s_bits, STAY_CUR_STATE, 0);

	default:
		break;
	}

	return -1;
}

EXPORT_API int appcore_unlock_power_state(int s_bits)
{
	return pm_unlock_state(s_bits, STAY_CUR_STATE);
}
