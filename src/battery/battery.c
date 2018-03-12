// Copyright (c) 2014-2018 LG Electronics, Inc.
//
// Licensed under the Apache License, Version 2.0 (the "License");
// you may not use this file except in compliance with the License.
// You may obtain a copy of the License at
//
// http://www.apache.org/licenses/LICENSE-2.0
//
// Unless required by applicable law or agreed to in writing, software
// distributed under the License is distributed on an "AS IS" BASIS,
// WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
// See the License for the specific language governing permissions and
// limitations under the License.
//
// SPDX-License-Identifier: Apache-2.0

/**
 * @file battery.c
 *
 * @brief Interface for reading all the battery values from sysfs node.
 *
 */

#include <stdio.h>
#include <unistd.h>
#include <string.h>
#include <glib.h>
#include <errno.h>
#include <stdlib.h>
#include <fcntl.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>
#include <nyx/module/nyx_log.h>
#include "msgid.h"

#include <glib.h>
#include <libudev.h>

#include "battery.h"
#include "utils.h"

#define CHARGE_MIN_TEMPERATURE_C 0
#define CHARGE_MAX_TEMPERATURE_C 57
#define BATTERY_MAX_TEMPERATURE_C  60

#define PATH_LEN 256

char *battery_sysfs_path = NULL;

nyx_battery_ctia_t battery_ctia_params;

int current_battery_percentage;
bool current_battery_present;

struct udev *udev = NULL;
struct udev_monitor *mon = NULL;
guint watch = 0;

extern nyx_device_t *nyxDev;
extern void *battery_callback_context;
extern nyx_device_callback_function_t battery_callback;

char batt_capacity_path[PATH_LEN] = {0,};
char batt_energy_now_path[PATH_LEN] = {0,};
char batt_energy_full_path[PATH_LEN] = {0,};
char batt_energy_full_design_path[PATH_LEN] = {0,};
char batt_charge_now_path[PATH_LEN] = {0,};
char batt_charge_full_path[PATH_LEN] = {0,};
char batt_charge_full_design_path[PATH_LEN] = {0,};
char batt_temperature_path[PATH_LEN] = {0,};
char batt_voltage_path[PATH_LEN] = {0,};
char batt_current_path[PATH_LEN] = {0,};
char batt_present_path[PATH_LEN] = {0,};
char batt_fake_battery_path[PATH_LEN] = {0,};

nyx_battery_ctia_t *get_battery_ctia_params(void)
{
	battery_ctia_params.charge_min_temp_c = CHARGE_MIN_TEMPERATURE_C;
	battery_ctia_params.charge_max_temp_c = CHARGE_MAX_TEMPERATURE_C;
	battery_ctia_params.battery_crit_max_temp = BATTERY_MAX_TEMPERATURE_C;
	battery_ctia_params.skip_battery_authentication = true;

	return &battery_ctia_params;
}

/**
 * @brief Read battery percentage
 *
 * @retval Battery percentage (integer)
 */
int battery_percent(void)
{
	int now, full;
	int capacity;

	// TODO: Might first confirm that battery is present?

	/* try capacity node first but keep in mind it's not supported by all power class devices */
	if ((capacity = nyx_utils_read_value(batt_capacity_path)) < 0)
	{
		/* capacity node is not available so next try is energy_full path */
		if (g_file_test(batt_energy_full_path, G_FILE_TEST_EXISTS))
		{
			if ((now = nyx_utils_read_value(batt_energy_now_path)) < 0)
			{
				return -1;
			}

			if ((full = nyx_utils_read_value(batt_energy_full_path)) < 0)
			{
				return -1;
			}

			capacity = (100 * now / full);
		}
		/* as last try we can use charge_now path */
		else if (g_file_test(batt_charge_now_path, G_FILE_TEST_EXISTS))
		{
			if ((full = nyx_utils_read_value(batt_charge_full_path)) < 0)
			{
				return -1;
			}

			if ((now = nyx_utils_read_value(batt_charge_now_path)) < 0)
			{
				return -1;
			}

			capacity = (100 * now / full);
		}
		else
		{
			return -1;
		}
	}

	return capacity;
}

/**
 * @brief Read battery temperature
 *
 * @retval Battery temperature (integer)
 */
int battery_temperature(void)
{
	int temp;

	if ((temp = nyx_utils_read_value(batt_temperature_path)) < 0)
	{
		return -1;
	}

	return temp;
}

/**
 * @brief Read battery voltage
 *
 * @retval Battery voltage (integer)
 */

int battery_voltage(void)
{
	int voltage;

	if ((voltage = nyx_utils_read_value(batt_voltage_path)) < 0)
	{
		return -1;
	}

	return voltage;
}

/**
 * @brief Read the amount of current being drawn by the battery (negative = charging!)
 *
 * @retval Current (integer)
 */
int battery_current(void)
{
	signed int current;

	if ((current = nyx_utils_read_value(batt_current_path)) < 0)
	{
		return -1;
	}

	return current;
}

/**
 * @brief Read average current being drawn by the battery.
 *
 * @retval Current (integer)
 */

int battery_avg_current(void)
{
	// return battery_current for this device unless we have a way to separately read "average" current
	return battery_current();
}

/**
 * @brief Read battery full capacity
 *
 * @retval Battery capacity (double)
 */
double battery_full40(void)
{
	int charge_full;

	if (!g_file_test(batt_charge_full_path, G_FILE_TEST_EXISTS) ||
	        ((charge_full = nyx_utils_read_value(batt_charge_full_path)) < 0))
	{
		if ((charge_full = nyx_utils_read_value(batt_charge_full_design_path)) < 0)
		{
			return -1;
		}
	}

	/* Divide the value by 1000 to convert from uAh to mAh */
	return (double) charge_full / 1000;
}

/**
 * @brief Read battery current raw capacity
 *
 * @retval Battery capacity (double)
 */

double battery_rawcoulomb(void)
{
	return -1;
}

/**
 * @brief Read battery current capacity
 *
 * @retval Battery capacity (double)
 */

double battery_coulomb(void)
{
	int charge_now;

	if ((charge_now = nyx_utils_read_value(batt_charge_now_path)) < 0)
	{
		return -1;
	}

	/* Divide the value by 1000 to convert from uAh to mAh */
	return (double) charge_now / 1000;
}

/**
 * @brief Read battery age
 *
 * @retval Battery age (double)
 */
double battery_age(void)
{
	return -1;
}

bool battery_is_present(void)
{
	int present;

	if ((present = nyx_utils_read_value(batt_present_path)) < 0)
	{
		return false;
	}

	return (1 == present);
}

gboolean _handle_event(GIOChannel *channel, GIOCondition condition,
                       gpointer data)
{
	struct udev_device *dev;

	if ((condition  & G_IO_IN) == G_IO_IN)
	{
		dev = udev_monitor_receive_device(mon);

		if (dev)
		{
			/*Initiate callback only if battery percentage or present parameters change*/
			int prev_battery_percentage = current_battery_percentage;
			bool prev_battery_present = current_battery_present;

			current_battery_present = battery_is_present();
			current_battery_percentage = current_battery_present ? battery_percent() : 0;

			if ((current_battery_present != prev_battery_present) ||
			        (current_battery_percentage != prev_battery_percentage))
			{
				if (battery_callback != NULL)
				{
					battery_callback(nyxDev, NYX_CALLBACK_STATUS_DONE, battery_callback_context);
				}
			}
		}
		else
		{
			if (battery_callback != NULL)
			{
				battery_callback(nyxDev, NYX_CALLBACK_STATUS_DONE, battery_callback_context);
			}
		}
	}

	return TRUE;
}

static void detect_battery_sysfs_paths()
{
	battery_sysfs_path = find_power_supply_sysfs_path("Battery");

	if (battery_sysfs_path)
	{
		snprintf(batt_capacity_path, PATH_LEN, "%s/capacity", battery_sysfs_path);
		snprintf(batt_energy_now_path, PATH_LEN, "%s/energy_now", battery_sysfs_path);
		snprintf(batt_energy_full_path, PATH_LEN, "%s/energy_full", battery_sysfs_path);
		snprintf(batt_charge_now_path, PATH_LEN, "%s/charge_now", battery_sysfs_path);
		snprintf(batt_charge_full_path, PATH_LEN, "%s/charge_full", battery_sysfs_path);
		snprintf(batt_charge_full_design_path, PATH_LEN, "%s/charge_full_design",
		         battery_sysfs_path);
		snprintf(batt_temperature_path, PATH_LEN, "%s/temp", battery_sysfs_path);
		snprintf(batt_voltage_path, PATH_LEN, "%s/voltage_now", battery_sysfs_path);
		snprintf(batt_current_path, PATH_LEN, "%s/current_now", battery_sysfs_path);
		snprintf(batt_present_path, PATH_LEN, "%s/present", battery_sysfs_path);
		snprintf(batt_fake_battery_path, PATH_LEN, "%s/pseudo_batt",
		         battery_sysfs_path);
	}
}

static void battery_cleanup(void)
{
	// battery_init sets g_io_channel_set_close_on_unref, and calls g_io_channel_unref.
	// This leaves one ref associated with the watch, so removing the watch should close the channel.
	if (0 != watch)
	{
		g_source_remove(watch);
		watch = 0;
	}

	if (NULL != mon)
	{
		udev_monitor_filter_remove(mon);
		mon = NULL;
	}

	if (NULL != udev)
	{
		udev_unref(udev);
		udev = NULL;
	}

	return;
}

nyx_error_t battery_init(void)
{
	int fd;
	GIOChannel *channel = NULL;

	udev = udev_new();

	if (!udev)
	{
		nyx_error(MSGID_NYX_MOD_UDEV_ERR, 0 ,
		          "Could not initialize udev component; battery status updates will not be available");
		return NYX_ERROR_GENERIC;
	}

	/*Initialize the sysfs paths*/
	detect_battery_sysfs_paths();

	// initialize current battery present/percentage values
	current_battery_present = battery_is_present();
	current_battery_percentage = current_battery_present ? battery_percent() : 0;

	mon = udev_monitor_new_from_netlink(udev, "kernel");

	if (mon == NULL)
	{
		nyx_error(MSGID_NYX_MOD_UDEV_MONITOR_ERR, 0,
		          "Failed to create udev monitor for kernel events");
		battery_cleanup();
		return NYX_ERROR_GENERIC;
	}

	if (udev_monitor_filter_add_match_subsystem_devtype(mon, "power_supply",
	        NULL) < 0)
	{
		nyx_error(MSGID_NYX_MOD_UDEV_SUBSYSTEM_ERR, 0,
		          "Failed to setup udev filter for power_supply subsytem events");
		battery_cleanup();
		return NYX_ERROR_GENERIC;
	}

	if (udev_monitor_enable_receiving(mon) < 0)
	{
		nyx_error(MSGID_NYX_MOD_UDEV_RECV_ERR, 0,
		          "Failed to enable receiving kernel events for power_supply subsytem\n");
		battery_cleanup();
		return NYX_ERROR_GENERIC;
	}

	/* Setup io watch for uevents */
	fd = udev_monitor_get_fd(mon);

	if (-1 == fd)
	{
		battery_cleanup();
		return NYX_ERROR_GENERIC;
	}

	channel = g_io_channel_unix_new(fd);

	if (!channel)
	{
		battery_cleanup();
		return NYX_ERROR_GENERIC;
	}

	/* add watch event (which adds a ref) before calling g_io_channel_unref */
	watch = g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_NVAL, _handle_event,
	                       NULL);

	/* Remove the ref from g_io_channel_unix_new so we won't leak the channel if g_io_add_watch failed */
	/* watch holds another ref which is removed in battery_cleanup */
	g_io_channel_set_close_on_unref(channel, TRUE);
	g_io_channel_unref(channel);

	if (0 == watch)
	{
		battery_cleanup();
		return NYX_ERROR_GENERIC;
	}

	return NYX_ERROR_NONE;
}

nyx_error_t battery_deinit(void)
{
	battery_cleanup();
	return NYX_ERROR_NONE;
}

#if 0
// not currently called by batterylib.c
bool battery_is_authenticated(const char *pair_challenge,
                              const char *pair_response)
{
	/* not supported */
	return true;
}
#endif

bool battery_authenticate(void)
{
	/* not supported */
	return true;
}

void battery_set_wakeup_percent(int percentage)
{
	/* not supported */
	return;
}

void battery_set_fakemode(bool enable)
{
	char buf[32];

	snprintf(buf, sizeof(buf), "%d %s", enable, "1 100 40 4100 80 1");
	nyx_utils_write(batt_fake_battery_path, buf, sizeof(buf));

	return;
}

nyx_error_t battery_get_fakemode(bool *enable)
{
	char buf[32];

	if (enable == NULL)
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	if (nyx_utils_read(batt_fake_battery_path, buf, sizeof(buf)))
	{
		*enable = strstr(buf, "NORMAL") == 0;
	}
	else
	{
		return NYX_ERROR_INVALID_VALUE;
	}

	return NYX_ERROR_NONE;
}
