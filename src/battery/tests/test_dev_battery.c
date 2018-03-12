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

#include <glib.h>
#include <stdio.h>

//
// Provide missing g_test macros if they are not defined in this version.
//
// We can't simply back-port the real definitions from glib as that would
// would change the license for this component.
//

#ifndef g_assert_true
#define g_assert_true(X) g_assert((X))
#endif

#ifndef g_assert_false
#define g_assert_false(X) g_assert(!(X))
#endif

#ifndef g_assert_nonnull
#define g_assert_nonnull(X) g_assert((X) != NULL)
#endif

#ifndef g_assert_null
#define g_assert_null(X) g_assert((X) == NULL)
#endif

//
// Pull in the relevant nyx headers. That way we can redefine macros
// if necessary (e.g. for logging) and the anti-recursion in the headers
// will let our redefinitions leak through into the UUT.
//
#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>

//
// Mock out all the calls to nyx-lib
//
#undef nyx_info
#define nyx_info(m, args...) {}
#undef nyx_debug
#define nyx_debug(m, args...) {}
#undef nyx_error
#define nyx_error(m, args...) {}

// NOTE: define this nyx_debug to send TEST messages (from THIS file, e.g. refcounts) to stderr:
//#define nyx_debug(m, ...) {fprintf(stderr,"\n\t"); fprintf(stderr, m, ##__VA_ARGS__);}
// NOTE: define this nyx_error to send TARGET nyx_error messages (from the tested source) to stderr:
//#define nyx_error(m, ...) {fprintf(stderr,"\n\t"); fprintf(stderr, m, ##__VA_ARGS__);}

// mock out externals defined in chargerlib.c

nyx_device_t *nyxDev = NULL;

//*****************************************************************************
//*****************************************************************************

// Define battery callback context used by _handle_event()
void *battery_callback_context = (void *)12345;

//typedef void (*nyx_device_callback_function_t)(nyx_device_handle_t, nyx_callback_status_t, void *);
void test_battery_callback(nyx_device_handle_t device,
                           nyx_callback_status_t status, void *context);
void test_battery_callback(nyx_device_handle_t device,
                           nyx_callback_status_t status, void *context)
{
	g_assert_true(nyxDev == device);
	g_assert_true(NYX_CALLBACK_STATUS_DONE == status);
	g_assert_true(battery_callback_context == context);
	return;
}

// Define battery callback called by _handle_event()
nyx_device_callback_function_t battery_callback = &test_battery_callback;


//*****************************************************************************
//*****************************************************************************

// Pull in the unit under test
#include "../device/battery.c"

//*****************************************************************************
//*****************************************************************************

// mock out calls to nyx-modules: utils.c
char *find_power_supply_sysfs_path(const char *device_type)
{
	// return whatever they asked for: Battery, USB, Mains, Touch, or Wireless (from _detect_battery_sysfs_paths() in device/battery.c)
	return (char *) device_type;
}

// define paths used in _detect_battery_sysfs_paths() in device/battery.c
static char *test_batt_capacity_path = "Battery/capacity";
static char *test_batt_energy_now_path = "Battery/energy_now";
static char *test_batt_energy_full_path = "Battery/energy_full";
static char *test_batt_charge_now_path = "Battery/charge_now";
static char *test_batt_charge_full_path = "Battery/charge_full";
static char *test_batt_charge_full_design_path = "Battery/charge_full_design";
static char *test_batt_temperature_path = "Battery/temp";
static char *test_batt_voltage_path = "Battery/voltage_now";
static char *test_batt_current_path = "Battery/current_now";
static char *test_batt_present_path = "Battery/present";

// define (mocked) values returns for above paths
int32_t test_batt_capacity_path_retval = 0;
int32_t test_batt_energy_now_path_retval = 0;
int32_t test_batt_energy_full_path_retval = 0;
int32_t test_batt_charge_now_path_retval = 0;
int32_t test_batt_charge_full_path_retval = 0;
int32_t test_batt_charge_full_design_path_retval = 0;
int32_t test_batt_temperature_path_retval = 0;
int32_t test_batt_voltage_path_retval = 0;
int32_t test_batt_current_path_retval = 0;
int32_t test_batt_present_path_retval = 0;

#define ifMatchReturnRetvalForTestPath(value) if (0 == strncmp(path, value, PATH_LEN)) { return value##_retval; }
// ifMatchReturnRetvalForTestPath(batt_capacity) expands to:
// if (0 == strncmp(path, test_batt_capacity_path, PATH_LEN))
// {
//  return test_batt_capacity_path_retval;
// }

// return appropriate _retval value for calls to nyx_utils_read_value
int32_t nyx_utils_read_value(char *path)
{
	//fprintf(stderr,"path (%s) passed to nyx_utils_read_value\n", path);
	ifMatchReturnRetvalForTestPath(test_batt_capacity_path)
	else ifMatchReturnRetvalForTestPath(test_batt_energy_now_path)
		else ifMatchReturnRetvalForTestPath(test_batt_energy_full_path)
			else ifMatchReturnRetvalForTestPath(test_batt_charge_now_path)
				else ifMatchReturnRetvalForTestPath(test_batt_charge_full_path)
					else ifMatchReturnRetvalForTestPath(test_batt_charge_full_design_path)
						else ifMatchReturnRetvalForTestPath(test_batt_temperature_path)
							else ifMatchReturnRetvalForTestPath(test_batt_voltage_path)
								else ifMatchReturnRetvalForTestPath(test_batt_current_path)
									else ifMatchReturnRetvalForTestPath(test_batt_present_path)

										// bad path: print error, force g_assert, and return -1
										fprintf(stderr, "Bad path (%s) passed to nyx_utils_read_value\n", path);

	g_assert_true(path == (const char *)"Bad path passed to nyx_utils_read_value");
	return -1;
}

//*****************************************************************************
//*****************************************************************************

// udev is an "Opaque object representing the library context"
struct udev
{
	int opaque;
};

// udev_device is an "Opaque object representing one kernel sys device."
struct udev_device
{
	int opaque;
};

// udev_monitor is an "Opaque object handling an event source"
struct udev_monitor
{
	int opaque;
};


// Mock the udev calls

// udev_monitor_receive_device() is called by _handle_power_supply_event()
// which is a callback passed to g_io_add_watch() in _charger_init()...
struct udev_device testUdevDevice;
struct udev_device *testUdevDevice_retval = &testUdevDevice;
struct udev_device *udev_monitor_receive_device(struct udev_monitor
        *udev_monitor)
{
	return testUdevDevice_retval;
}

int32_t testUdevRefcount = 0;
struct udev testUdevStruct;
struct udev *testUdevStruct_retval = &testUdevStruct;
struct udev *udev_new(void)
{
	// are we returning our valid "test" udev (as opposed to NULL)?
	if (&testUdevStruct == testUdevStruct_retval)
	{
		// yes, so bump our testGIOChannel refcount
		testUdevRefcount++;
	}

	nyx_debug("In udev_new: testUdevRefcount = %d", testUdevRefcount);
	return testUdevStruct_retval;
}

/* kernel and udev generated events over netlink */
struct udev_monitor testUdevMonitorStruct;
struct udev_monitor *testUdevMonitorStruct_retval = &testUdevMonitorStruct;
struct udev_monitor *udev_monitor_new_from_netlink(struct udev *udev,
        const char *name)
{
	return testUdevMonitorStruct_retval;
}

/* in-kernel socket filters to select messages that get delivered to a listener */
int testUdevMonitorFilterAddMatchResult_retval = 0;
int udev_monitor_filter_add_match_subsystem_devtype(struct udev_monitor
        *udev_monitor,
        const char *subsystem, const char *devtype)
{
	return testUdevMonitorFilterAddMatchResult_retval;
}

/* bind socket */
int testUdevMonitorEnableReceiving_retval = 0;
int udev_monitor_enable_receiving(struct udev_monitor *udev_monitor)
{
	return testUdevMonitorEnableReceiving_retval;
}

int testUdevMonitorGetFd_retval = 0;
int udev_monitor_get_fd(struct udev_monitor *udev_monitor)
{
	return testUdevMonitorGetFd_retval;
}

int udev_monitor_filter_remove(struct udev_monitor *udev_monitor)
{
	return 0;
}

void udev_unref(struct udev *udev)
{
	// is this a request for our valid "test" udev?
	if (&testUdevStruct == udev)
	{
		// yes, so decrement our testGIOChannel refcount
		testUdevRefcount--;
	}

	nyx_debug("In udev_unref: testUdevRefcount = %d", testUdevRefcount);
	return;
}

//*****************************************************************************
//*****************************************************************************

// Mock the glib calls

// define (mocked) values returns for above paths
int32_t test_batt_capacity_path_exists = false;
int32_t test_batt_energy_now_path_exists = false;
int32_t test_batt_energy_full_path_exists = false;
int32_t test_batt_charge_now_path_exists = false;
int32_t test_batt_charge_full_path_exists = false;
int32_t test_batt_charge_full_design_path_exists = false;
int32_t test_batt_temperature_path_exists = false;
int32_t test_batt_voltage_path_exists = false;
int32_t test_batt_current_path_exists = false;
int32_t test_batt_present_path_exists = false;

#define ifMatchReturnExistsForTestPath(value) if (0 == strncmp(path, value, PATH_LEN)) { return value##_exists; }
// ifMatchReturnExistsForTestPath(batt_capacity) expands to:
// if (0 == strncmp(path, test_batt_capacity_path, PATH_LEN))
// {
//  return test_batt_capacity_path_exists;
// }

// return appropriate _exists value for calls to g_file_test
gboolean g_file_test(const gchar *path, GFileTest test)
{
	//fprintf(stderr,"path (%s) passed to g_file_test\n", path);
	if (G_FILE_TEST_EXISTS != test)
	{
		fprintf(stderr, "Un-mocked test (%d) passed to g_file_test\n",
		        G_FILE_TEST_EXISTS);
		g_assert_true(path == (const char *)"Un-mocked test passed to g_file_test");
		return -1;
	}

	ifMatchReturnExistsForTestPath(test_batt_capacity_path)
	else ifMatchReturnExistsForTestPath(test_batt_energy_now_path)
		else ifMatchReturnExistsForTestPath(test_batt_energy_full_path)
			else ifMatchReturnExistsForTestPath(test_batt_charge_now_path)
				else ifMatchReturnExistsForTestPath(test_batt_charge_full_path)
					else ifMatchReturnExistsForTestPath(test_batt_charge_full_design_path)
						else ifMatchReturnExistsForTestPath(test_batt_temperature_path)
							else ifMatchReturnExistsForTestPath(test_batt_voltage_path)
								else ifMatchReturnExistsForTestPath(test_batt_current_path)
									else ifMatchReturnExistsForTestPath(test_batt_present_path)

										// bad path: print error, force g_assert, and return -1
										fprintf(stderr, "Bad path (%s) passed to g_file_test\n", path);

	g_assert_true(path == (const char *)"Bad path passed to g_file_test");
	return -1;
}



// code calls: channel = g_io_channel_unix_new(fd);
int32_t testGIOChannelRefcount = 0;
GIOChannel testGIOChannel;
GIOChannel *testGIOChannel_retval = &testGIOChannel;
GIOChannel *g_io_channel_unix_new(int fd)
{
	// are we returning our valid "test" channel (as opposed to NULL)?
	if (&testGIOChannel == testGIOChannel_retval)
	{
		// yes, so bump our testGIOChannel refcount
		testGIOChannelRefcount++;
	}

	nyx_debug("In g_io_channel_unix_new: testGIOChannelRefcount = %d",
	          testGIOChannelRefcount);
	return testGIOChannel_retval;
}

// code calls: g_io_add_watch(channel, G_IO_IN | G_IO_HUP | G_IO_NVAL, _handle_power_supply_event, NULL);
static const guint testEventSourceIdGood = 1;
static const guint testEventSourceIdBad = 0;
guint     testEventSourceId_retVal = 0;
guint     g_io_add_watch(GIOChannel      *channel,
                         GIOCondition     condition,
                         GIOFunc          func,
                         gpointer         user_data)
{
	// is this a request for our valid "test" channel (with good retVal)?
	if ((&testGIOChannel == channel) &&
	        (testEventSourceIdGood == testEventSourceId_retVal))
	{
		// yes, so bump our testGIOChannel refcount
		testGIOChannelRefcount++;
	}

	nyx_debug("In g_io_add_watch: testGIOChannelRefcount = %d",
	          testGIOChannelRefcount);
	// return value is "the event source id" which is not used by charger.c
	return testEventSourceId_retVal;
}

// code calls: g_io_channel_set_close_on_unref(channel, TRUE);
void                g_io_channel_set_close_on_unref(GIOChannel *channel,
        gboolean do_close)
{
	return;
}

// code calls: g_io_channel_unref(channel);
void                g_io_channel_unref(GIOChannel *channel)
{
	// is this a request for our valid "test" channel?
	if (&testGIOChannel == channel)
	{
		// yes, so decrement our testGIOChannel refcount
		testGIOChannelRefcount--;
	}

	nyx_debug("In g_io_channel_unref: testGIOChannelRefcount = %d",
	          testGIOChannelRefcount);
	return;
}

// code calls: g_source_remove(watch);
gboolean g_source_remove(guint tag)
{
	// is this a request for our valid "test" watch?
	if (testEventSourceIdGood == tag)
	{
		// yes, so decrement our testGIOChannel refcount
		testGIOChannelRefcount--;
	}

	nyx_debug("In g_source_remove: testGIOChannelRefcount = %d",
	          testGIOChannelRefcount);
	return TRUE;
}

//*****************************************************************************
//*****************************************************************************

#if 0
static int32_t init_charger_max_current = -1;
static int32_t init_connected = -1;
static int32_t init_powered = -1;
static bool init_is_charging = false;
static char *init_serial_number = "serialNumber";
static void resetTestChargerStatus(nyx_charger_status_t *chargerStatus)
{
	chargerStatus->charger_max_current = init_charger_max_current;
	chargerStatus->connected = init_connected;
	chargerStatus->powered = init_powered;
	chargerStatus->is_charging = init_is_charging;
	strncpy(chargerStatus->dock_serial_number, init_serial_number,
	        NYX_DOCK_SERIAL_NUMBER_LEN);
}
#endif

//
// Tests for the _charger_init API method
// nyx_error_t _charger_init(void)
//
static void
test_battery_init(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// NOTE: We always call battery_deinit() after calling battery_init() to prevent leaks

	// Initial setup of good return values
	testUdevStruct_retval = &testUdevStruct;
	testUdevMonitorStruct_retval = &testUdevMonitorStruct;
	testUdevMonitorFilterAddMatchResult_retval = 0;
	testUdevMonitorEnableReceiving_retval = 0;
	testUdevMonitorGetFd_retval = 0;
	testGIOChannel_retval = &testGIOChannel;
	testEventSourceId_retVal = testEventSourceIdGood;

	// setup BAD return value for udev_new and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for udev_new");
	testUdevStruct_retval = NULL;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_new
	testUdevStruct_retval = &testUdevStruct;


	// setup BAD return value for udev_monitor_new_from_netlink and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for udev_monitor_new_from_netlink");
	testUdevMonitorStruct_retval = NULL;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_new_from_netlink
	testUdevMonitorStruct_retval = &testUdevMonitorStruct;


	// setup BAD return value for udev_monitor_filter_add_match_subsystem_devtype and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for udev_monitor_filter_add_match_subsystem_devtype");
	testUdevMonitorFilterAddMatchResult_retval = -1;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_filter_add_match_subsystem_devtype
	testUdevMonitorFilterAddMatchResult_retval = 0;


	// setup BAD return value for udev_monitor_enable_receiving and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for udev_monitor_enable_receiving");
	testUdevMonitorEnableReceiving_retval = -1;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_enable_receiving
	testUdevMonitorEnableReceiving_retval = 0;


	// setup BAD return value for udev_monitor_get_fd and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for udev_monitor_get_fd");
	testUdevMonitorGetFd_retval = -1;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_get_fd
	testUdevMonitorGetFd_retval = 0;


	// setup BAD return value for g_io_channel_unix_new and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for g_io_channel_unix_new");
	testGIOChannel_retval = NULL;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for g_io_channel_unix_new
	testGIOChannel_retval = &testGIOChannel;


	// setup BAD return value for g_io_add_watch and force expected event
	nyx_debug("\nIn test_battery_init: setup BAD return value for g_io_add_watch");
	testEventSourceId_retVal = testEventSourceIdBad;
	g_assert_false(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for g_io_add_watch and force expected event
	nyx_debug("\nIn test_battery_init: setup GOOD return value for g_io_add_watch");
	testEventSourceId_retVal = testEventSourceIdGood;
	g_assert_true(NYX_ERROR_NONE == battery_init());
	g_assert_true(NYX_ERROR_NONE == battery_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);
	nyx_debug("\n");
}

#if 0
//
// Tests for the _charger_read_status API method
// nyx_error_t _charger_read_status(nyx_charger_status_t *status)
//
static void
test__charger_read_status(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	nyx_charger_status_t testChargerStatus;
	resetTestChargerStatus(&testChargerStatus);

	// Check for no error
	g_assert_true(NYX_ERROR_NONE == _charger_read_status(&testChargerStatus));

	// For now, check to make sure values returned are different from our initialized test values
	g_assert_true(testChargerStatus.charger_max_current !=
	              init_charger_max_current);
	g_assert_true(testChargerStatus.connected != init_connected);
	g_assert_true(testChargerStatus.powered != init_powered);
	// can't check to see if is_charging changed since it's a "bool"
	//g_assert_true(testChargerStatus.is_charging != init_is_charging);
	g_assert_true(0 != strncmp(testChargerStatus.dock_serial_number,
	                           init_serial_number, NYX_DOCK_SERIAL_NUMBER_LEN));

	// Check to see if is_charging returns true when we claim to be connected to USB
	test_battery_sysfs_path_retval = 0;
	test_charger_usb_sysfs_path_retval = 1;
	test_charger_ac_sysfs_path_retval = 0;
	test_charger_touch_sysfs_path_retval = 0;
	test_charger_wireless_sysfs_path_retval = 0;
	resetTestChargerStatus(&testChargerStatus);
	// force is_charging status to false; make sure it returns true
	testChargerStatus.is_charging = false;
	g_assert_true(NYX_ERROR_NONE == _charger_read_status(&testChargerStatus));
	g_assert_true(true == testChargerStatus.is_charging);

	// Check to see if is_charging returns true when we claim to be connected to AC
	test_battery_sysfs_path_retval = 0;
	test_charger_usb_sysfs_path_retval = 0;
	test_charger_ac_sysfs_path_retval = 1;
	test_charger_touch_sysfs_path_retval = 0;
	test_charger_wireless_sysfs_path_retval = 0;
	resetTestChargerStatus(&testChargerStatus);
	// force is_charging status to false; make sure it returns true
	testChargerStatus.is_charging = false;
	g_assert_true(NYX_ERROR_NONE == _charger_read_status(&testChargerStatus));
	g_assert_true(true == testChargerStatus.is_charging);

	// Check to see if is_charging returns false when we claim to NOT be connected to AC or USB
	test_battery_sysfs_path_retval = 0;
	test_charger_usb_sysfs_path_retval = 0;
	test_charger_ac_sysfs_path_retval = 0;
	test_charger_touch_sysfs_path_retval = 0;
	test_charger_wireless_sysfs_path_retval = 0;
	// force is_charging status to true; make sure it returns false
	resetTestChargerStatus(&testChargerStatus);
	testChargerStatus.is_charging = 1;
	g_assert_true(NYX_ERROR_NONE == _charger_read_status(&testChargerStatus));
	g_assert_true(0 == testChargerStatus.is_charging);

	// NOTE: We don't bother passing NULL for status since status is checked in charger_read_status() in chargerlib.c
}

#endif

void reset_battery_path_retvals(void)
{
	test_batt_capacity_path_exists = false;
	test_batt_energy_now_path_exists = false;
	test_batt_energy_full_path_exists = false;
	test_batt_charge_now_path_exists = false;
	test_batt_charge_full_path_exists = false;
	test_batt_charge_full_design_path_exists = false;
	test_batt_temperature_path_exists = false;
	test_batt_voltage_path_exists = false;
	test_batt_current_path_exists = false;
	test_batt_present_path_exists = false;

	test_batt_capacity_path_retval = -1;
	test_batt_energy_now_path_retval = -1;
	test_batt_energy_full_path_retval = -1;
	test_batt_charge_now_path_retval = -1;
	test_batt_charge_full_path_retval = -1;
	test_batt_charge_full_design_path_retval = -1;
	test_batt_temperature_path_retval = -1;
	test_batt_voltage_path_retval = -1;
	test_batt_current_path_retval = -1;
	test_batt_present_path_retval = -1;
}

//
// Tests for the battery_percent API method
// int battery_percent(void)
//
static void
test_battery_percent(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_capacity_path if NO paths available
	reset_battery_path_retvals();
	g_assert_true(-1 == battery_percent());

	// Check for failure returned from test_batt_capacity_path if (only) invalid capacity
	reset_battery_path_retvals();
	test_batt_capacity_path_exists = true;
	test_batt_capacity_path_retval = -1;
	g_assert_true(-1 == battery_percent());

	// Check for correct return value from test_batt_capacity_path using valid capacity
	reset_battery_path_retvals();
	test_batt_capacity_path_exists = true;
	test_batt_capacity_path_retval = 80;
	g_assert_true(80 == battery_percent());


	// Check for failure returned from test_batt_capacity_path using invalid energy_now
	reset_battery_path_retvals();
	test_batt_energy_now_path_exists = true;
	test_batt_energy_full_path_exists = true;
	test_batt_energy_now_path_retval = -1;
	test_batt_energy_full_path_retval = 1000000;
	g_assert_true(-1 == battery_percent());

	// Check for failure returned from test_batt_capacity_path using invalid energy_full
	reset_battery_path_retvals();
	test_batt_energy_now_path_exists = true;
	test_batt_energy_full_path_exists = true;
	test_batt_energy_now_path_retval = 800000;
	test_batt_energy_full_path_retval = -1;
	g_assert_true(-1 == battery_percent());

	// Check for correct return value from test_batt_capacity_path using energy_now / energy_full
	reset_battery_path_retvals();
	test_batt_energy_now_path_exists = true;
	test_batt_energy_full_path_exists = true;
	test_batt_energy_now_path_retval = 800000;
	test_batt_energy_full_path_retval = 1000000;
	g_assert_true(80 == battery_percent());


	// Check for failure returned from test_batt_capacity_path using invalid charge_now
	reset_battery_path_retvals();
	test_batt_charge_now_path_exists = true;
	test_batt_charge_full_path_exists = true;
	test_batt_charge_now_path_retval = -1;
	test_batt_charge_full_path_retval = 2300000;
	g_assert_true(-1 == battery_percent());

	// Check for failure returned from test_batt_capacity_path using invalid charge_full
	reset_battery_path_retvals();
	test_batt_charge_now_path_exists = true;
	test_batt_charge_full_path_exists = true;
	test_batt_charge_now_path_retval = 1840000;
	test_batt_charge_full_path_retval = -1;
	g_assert_true(-1 == battery_percent());

	// Check for correct return value from test_batt_capacity_path using charge_now / charge_full
	reset_battery_path_retvals();
	test_batt_charge_now_path_exists = true;
	test_batt_charge_full_path_exists = true;
	test_batt_charge_now_path_retval = 1840000;
	test_batt_charge_full_path_retval = 2300000;
	g_assert_true(80 == battery_percent());

}

//
// Tests for the battery_temperature API method
// int battery_temperature(void)
//
static void
test_battery_temperature(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_temperature_path
	reset_battery_path_retvals();
	test_batt_temperature_path_exists = false;
	test_batt_temperature_path_retval = 333;
	g_assert_true(-1 == battery_temperature());

	// Check for failure returned from test_batt_temperature_path
	reset_battery_path_retvals();
	test_batt_temperature_path_exists = true;
	test_batt_temperature_path_retval = -1;
	g_assert_true(-1 == battery_temperature());

	// Check for correct return value from test_batt_temperature_path
	reset_battery_path_retvals();
	test_batt_temperature_path_exists = true;
	test_batt_temperature_path_retval = 333;
	g_assert_true(333 == battery_temperature());
}

//
// Tests for the battery_voltage API method
// int battery_voltage(void)
//
static void
test_battery_voltage(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_voltage_path
	reset_battery_path_retvals();
	test_batt_voltage_path_exists = false;
	test_batt_voltage_path_retval = 3995000;
	g_assert_true(-1 == battery_voltage());

	// Check for failure returned from test_batt_voltage_path
	reset_battery_path_retvals();
	test_batt_voltage_path_exists = true;
	test_batt_voltage_path_retval = -1;
	g_assert_true(-1 == battery_voltage());

	// Check for correct return value from test_batt_voltage_path
	// TODO: Should this be in mV or uV?  Device returns uV but emulator returns mV!
	reset_battery_path_retvals();
	test_batt_voltage_path_exists = true;
	test_batt_voltage_path_retval = 3995000;
	g_assert_true(3995000 == battery_voltage());
}

//
// Tests for the battery_current API method
// int battery_current(void)
//
static void
test_battery_current(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_current_path
	reset_battery_path_retvals();
	test_batt_current_path_exists = false;
	test_batt_current_path_retval = 371870;
	g_assert_true(-1 == battery_current());

	// Check for failure returned from test_batt_current_path
	reset_battery_path_retvals();
	test_batt_current_path_exists = true;
	test_batt_current_path_retval = -1;
	g_assert_true(-1 == battery_current());

	// Check for correct return value from test_batt_current_path
	reset_battery_path_retvals();
	test_batt_current_path_exists = true;
	test_batt_current_path_retval = 371870;
	// TODO: Should this be in mA or uA?  Device returns uA but emulator returns mA!
	g_assert_true(371870 == battery_current());
}

//
// Tests for the battery_avg_current API method
// int battery_avg_current(void)
//
static void
test_battery_avg_current(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_current_path
	reset_battery_path_retvals();
	test_batt_current_path_exists = false;
	test_batt_current_path_retval = 371870;
	g_assert_true(-1 == battery_current());

	// Check for failure returned from test_batt_current_path
	reset_battery_path_retvals();
	test_batt_current_path_exists = true;
	test_batt_current_path_retval = -1;
	g_assert_true(-1 == battery_current());

	// Check for correct return value from test_batt_current_path
	reset_battery_path_retvals();
	test_batt_current_path_exists = true;
	test_batt_current_path_retval = 371870;
	// TODO: Should this be in mA or uA?  Device returns uA but emulator returns mA!
	g_assert_true(371870 == battery_current());
}

//
// Tests for the battery_full40 API method
// double battery_full40(void)
//
static void
test_battery_full40(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_charge_full_path
	reset_battery_path_retvals();
	test_batt_charge_full_path_exists = false;
	test_batt_charge_full_path_retval = 2300000;
	g_assert_true(-1 == battery_full40());

	// Check for failure returned from test_batt_charge_full_path
	reset_battery_path_retvals();
	test_batt_charge_full_path_exists = true;
	test_batt_charge_full_path_retval = -1;
	g_assert_true(-1 == battery_full40());

	// Check for correct return value from test_batt_charge_full_path
	reset_battery_path_retvals();
	test_batt_charge_full_path_exists = true;
	test_batt_charge_full_path_retval = 2300000;
	// TODO: Should this be in mA or uA?  Device returns uA but emulator returns mA!
	g_assert_true((2300000 / 1000) == battery_full40());


	// Check for failure returned from test_batt_charge_full_design_path
	reset_battery_path_retvals();
	test_batt_charge_full_design_path_exists = false;
	test_batt_charge_full_design_path_retval = 3400000;
	g_assert_true(-1 == battery_full40());

	// Check for failure returned from test_batt_charge_full_design_path
	reset_battery_path_retvals();
	test_batt_charge_full_design_path_exists = true;
	test_batt_charge_full_design_path_retval = -1;
	g_assert_true(-1 == battery_full40());

	// Check for correct return value from test_batt_charge_full_design_path
	reset_battery_path_retvals();
	test_batt_charge_full_design_path_exists = true;
	test_batt_charge_full_design_path_retval = 3400000;
	// TODO: Should this be in mA or uA?  Device returns uA but emulator returns mA!
	g_assert_true((3400000 / 1000) == battery_full40());
}

//
// Tests for the battery_rawcoulomb API method
// double battery_rawcoulomb(void)
//
static void
test_battery_rawcoulomb(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from battery_rawcoulomb (not implemented)
	g_assert_true(-1 == battery_rawcoulomb());
}

//
// Tests for the battery_coulomb API method
// double battery_coulomb(void)
//
static void
test_battery_coulomb(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_charge_now_path
	reset_battery_path_retvals();
	test_batt_charge_now_path_exists = false;
	test_batt_charge_now_path_retval = 1840000;
	g_assert_true(-1 == battery_coulomb());

	// Check for failure returned from test_batt_charge_now_path
	reset_battery_path_retvals();
	test_batt_charge_now_path_exists = true;
	test_batt_charge_now_path_retval = -1;
	g_assert_true(-1 == battery_coulomb());

	// Check for correct return value from test_batt_charge_now_path
	reset_battery_path_retvals();
	test_batt_charge_now_path_exists = true;
	test_batt_charge_now_path_retval = 1840000;
	g_assert_true((1840000 / 1000) == battery_coulomb());
}

//
// Tests for the battery_age API method
// double battery_age(void)
//
static void
test_battery_age(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from battery_age (not implemented)
	g_assert_true(-1 == battery_age());
}

//
// Tests for the battery_is_present API method
// bool battery_is_present(void)
//
static void
test_battery_is_present(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for failure returned from test_batt_present_path
	reset_battery_path_retvals();
	test_batt_present_path_exists = false;
	test_batt_present_path_retval = 0;
	g_assert_true(false == battery_is_present());

	// Check for failure returned from test_batt_present_path
	reset_battery_path_retvals();
	test_batt_present_path_exists = true;
	test_batt_present_path_retval = -1;
	g_assert_true(false == battery_is_present());


	// Check for correct return value from test_batt_present_path (battery not present)
	reset_battery_path_retvals();
	test_batt_present_path_exists = true;
	test_batt_present_path_retval = 0;
	g_assert_true(false == battery_is_present());

	// Check for correct return value from test_batt_present_path (battery present)
	reset_battery_path_retvals();
	test_batt_present_path_exists = true;
	test_batt_present_path_retval = 1;
	g_assert_true(true == battery_is_present());
}

//
// Tests for the get_battery_ctia_params API method
// nyx_battery_ctia_t *get_battery_ctia_params(void)
//
static void
test_get_battery_ctia_params(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// Check for correct return value when voltage is zero
	nyx_battery_ctia_t *battery_ctia_params;
	battery_ctia_params = get_battery_ctia_params();
	g_assert_true(NULL != battery_ctia_params);

	// Can't assume CHARGE_MIN_TEMPERATURE_C is zero, so skip that...
	//g_assert_true(0 == battery_ctia_params->charge_min_temp_c);

	g_assert_true(0 != battery_ctia_params->charge_max_temp_c);
	g_assert_true(0 != battery_ctia_params->battery_crit_max_temp);

	// Can't assume skip_battery_authentication is true, so skip that...
	//g_assert_true(battery_ctia_params->skip_battery_authentication);
}

//
// Set-up GLib, then register and run the tests.
int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/battery/device/battery_init", test_battery_init);
	// g_test_add_func("/battery/device/battery_deinit", test_battery_deinit);

	g_test_add_func("/battery/device/battery_percent", test_battery_percent);
	g_test_add_func("/battery/device/battery_temperature",
	                test_battery_temperature);
	g_test_add_func("/battery/device/battery_voltage", test_battery_voltage);
	g_test_add_func("/battery/device/battery_current", test_battery_current);
	g_test_add_func("/battery/device/battery_avg_current",
	                test_battery_avg_current);

	g_test_add_func("/battery/device/battery_full40", test_battery_full40);
	g_test_add_func("/battery/device/battery_rawcoulomb", test_battery_rawcoulomb);
	g_test_add_func("/battery/device/battery_coulomb", test_battery_coulomb);
	g_test_add_func("/battery/device/battery_age", test_battery_age);

	g_test_add_func("/battery/device/battery_is_present", test_battery_is_present);
	g_test_add_func("/battery/device/get_battery_ctia_params",
	                test_get_battery_ctia_params);

	// TODO: Add test for _handle_event() callback function?

	// not currently supported by device/battery.c or emulator/fake_battery.c (stub implementations)
	// g_test_add_func("/battery/device/battery_authenticate", test_battery_authenticate);
	// g_test_add_func("/battery/device/battery_set_wakeup_percent", test_battery_set_wakeup_percent);

	return g_test_run();
}
