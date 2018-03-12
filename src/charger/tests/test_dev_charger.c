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

// TODO: Callbacks called (if non-NULL) by _handle_power_supply_event()
void *charger_status_callback_context = NULL;
void *state_change_callback_context = NULL;
nyx_device_callback_function_t charger_status_callback = NULL;
nyx_device_callback_function_t state_change_callback = NULL;

//*****************************************************************************
//*****************************************************************************

// Pull in the unit under test
#include "../device/charger.c"

//*****************************************************************************
//*****************************************************************************

// mock out calls to nyx-modules: utils.c
char *find_power_supply_sysfs_path(const char *device_type)
{
	// return whatever they asked for: Battery, USB, Mains, Touch, or Wireless (from _detect_charger_sysfs_paths() in device/charger.c)
	return (char *) device_type;
}

// TODO: Called by _battery_read_status(), which checks for -1 (but doesn't care about ret_string)
int FileGetString(const char *path, char *ret_string, size_t maxlen)
{
	return 0;
}

// define paths used in _detect_charger_sysfs_paths() in device/charger.c
static char *test_battery_sysfs_path = "Battery/online";
static char *test_charger_usb_sysfs_path = "USB/online";
static char *test_charger_ac_sysfs_path = "Mains/online";
static char *test_charger_touch_sysfs_path = "Touch/online";
static char *test_charger_wireless_sysfs_path = "Wireless/online";

// define (mocked) values returns for above paths
int32_t test_battery_sysfs_path_retval = 0;
int32_t test_charger_usb_sysfs_path_retval = 0;
int32_t test_charger_ac_sysfs_path_retval = 0;
int32_t test_charger_touch_sysfs_path_retval = 0;
int32_t test_charger_wireless_sysfs_path_retval = 0;

// return (mocked) int32_t values based on path
int32_t nyx_utils_read_value(char *path)
{
	//fprintf(stderr,"path (%s) passed to nyx_utils_read_value\n", path);
	if (0 == strncmp(path, test_battery_sysfs_path, PATH_LEN))
	{
		return test_battery_sysfs_path_retval;
	}
	else if (0 == strncmp(path, test_charger_usb_sysfs_path, PATH_LEN))
	{
		return test_charger_usb_sysfs_path_retval;
	}
	else if (0 == strncmp(path, test_charger_ac_sysfs_path, PATH_LEN))
	{
		return test_charger_ac_sysfs_path_retval;
	}
	else if (0 == strncmp(path, test_charger_touch_sysfs_path, PATH_LEN))
	{
		return test_charger_touch_sysfs_path_retval;
	}
	else if (0 == strncmp(path, test_charger_wireless_sysfs_path, PATH_LEN))
	{
		return test_charger_wireless_sysfs_path_retval;
	}

	fprintf(stderr, "Bad path (%s) passed to nyx_utils_read_value\n", path);
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

// TODO: mock g_file_test() for _battery_read_status() function, then mock FileGetString's ret_string (above)
// gboolean g_file_test (const gchar *filename, GFileTest test)
// {
// }

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

//
// Tests for the _charger_init API method
// nyx_error_t _charger_init(void)
//
static void
test__charger_init(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// NOTE: We always call _charger_deinit() after calling _charger_init() to prevent leaks

	// Initial setup of good return values
	testUdevStruct_retval = &testUdevStruct;
	testUdevMonitorStruct_retval = &testUdevMonitorStruct;
	testUdevMonitorFilterAddMatchResult_retval = 0;
	testUdevMonitorEnableReceiving_retval = 0;
	testUdevMonitorGetFd_retval = 0;
	testGIOChannel_retval = &testGIOChannel;
	testEventSourceId_retVal = testEventSourceIdGood;

	// setup BAD return value for udev_new and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for udev_new");
	testUdevStruct_retval = NULL;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_new
	testUdevStruct_retval = &testUdevStruct;


	// setup BAD return value for udev_monitor_new_from_netlink and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for udev_monitor_new_from_netlink");
	testUdevMonitorStruct_retval = NULL;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_new_from_netlink
	testUdevMonitorStruct_retval = &testUdevMonitorStruct;


	// setup BAD return value for udev_monitor_filter_add_match_subsystem_devtype and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for udev_monitor_filter_add_match_subsystem_devtype");
	testUdevMonitorFilterAddMatchResult_retval = -1;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_filter_add_match_subsystem_devtype
	testUdevMonitorFilterAddMatchResult_retval = 0;


	// setup BAD return value for udev_monitor_enable_receiving and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for udev_monitor_enable_receiving");
	testUdevMonitorEnableReceiving_retval = -1;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_enable_receiving
	testUdevMonitorEnableReceiving_retval = 0;


	// setup BAD return value for udev_monitor_get_fd and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for udev_monitor_get_fd");
	testUdevMonitorGetFd_retval = -1;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for udev_monitor_get_fd
	testUdevMonitorGetFd_retval = 0;


	// setup BAD return value for g_io_channel_unix_new and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for g_io_channel_unix_new");
	testGIOChannel_retval = NULL;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for g_io_channel_unix_new
	testGIOChannel_retval = &testGIOChannel;


	// setup BAD return value for g_io_add_watch and force expected event
	nyx_debug("\nIn test__charger_init: setup BAD return value for g_io_add_watch");
	testEventSourceId_retVal = testEventSourceIdBad;
	g_assert_false(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);

	// setup GOOD return value for g_io_add_watch and force expected event
	nyx_debug("\nIn test__charger_init: setup GOOD return value for g_io_add_watch");
	testEventSourceId_retVal = testEventSourceIdGood;
	g_assert_true(NYX_ERROR_NONE == _charger_init());
	g_assert_true(NYX_ERROR_NONE == _charger_deinit());
	// make sure we didn't leak any GIOChannel or udev references
	g_assert_true(0 == testGIOChannelRefcount);
	g_assert_true(0 == testUdevRefcount);
	nyx_debug("\n");
}

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

//
// Tests for the _charger_enable_charging API method
// nyx_error_t _charger_enable_charging(nyx_charger_status_t *status)
//
static void
test__charger_enable_charging(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	nyx_charger_status_t testChargerStatus;
	resetTestChargerStatus(&testChargerStatus);

	// Check for no error
	g_assert_true(NYX_ERROR_NONE == _charger_enable_charging(&testChargerStatus));

	// TODO: Check returned values (what should they be? should we fiddle with mocked functions to get different results?)
	// How much should we assume to know about the actual implementation to mock the various functions?
	// For now, check to make sure values returned are different:
	g_assert_true(testChargerStatus.charger_max_current !=
	              init_charger_max_current);
	g_assert_true(testChargerStatus.connected != init_connected);
	g_assert_true(testChargerStatus.powered != init_powered);
	// can't check to see if is_charging changed since it's a "bool"
	//g_assert_true(testChargerStatus.is_charging != init_is_charging);
	g_assert_true(0 != strncmp(testChargerStatus.dock_serial_number,
	                           init_serial_number, NYX_DOCK_SERIAL_NUMBER_LEN));

	// NOTE: We don't bother passing NULL for status since status is checked in charger_enable_charging() in chargerlib.c
}

//
// Tests for the _charger_disable_charging API method
// nyx_error_t _charger_disable_charging(nyx_charger_status_t *status)
//
static void
test__charger_disable_charging(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	nyx_charger_status_t testChargerStatus;
	resetTestChargerStatus(&testChargerStatus);

	// Check for no error
	g_assert_true(NYX_ERROR_NONE == _charger_disable_charging(&testChargerStatus));

	// TODO: Check returned values (what should they be? should we fiddle with mocked functions to get different results?)
	// How much should we assume to know about the actual implementation to mock the various functions?
	// For now, check to make sure values returned are different:
	g_assert_true(testChargerStatus.charger_max_current !=
	              init_charger_max_current);
	g_assert_true(testChargerStatus.connected != init_connected);
	g_assert_true(testChargerStatus.powered != init_powered);
	// can't check to see if is_charging changed since it's a "bool"
	//g_assert_true(testChargerStatus.is_charging != init_is_charging);
	g_assert_true(0 != strncmp(testChargerStatus.dock_serial_number,
	                           init_serial_number, NYX_DOCK_SERIAL_NUMBER_LEN));

	// NOTE: We don't bother passing NULL for status since status is checked in charger_disable_charging() in chargerlib.c
}

//
// Tests for the _charger_query_charger_event API method
// nyx_error_t _charger_query_charger_event(nyx_charger_event_t *event)
//
static void
test__charger_query_charger_event(/*api_test_fixture *fixture, gconstpointer unused*/)
{
	// setup return value and force expected event
	nyx_charger_event_t returnedEvent = NYX_CHARGER_FAULT;
	current_event = NYX_CHARGER_CONNECTED;

	// Call function; check for no error and test returned value
	g_assert_true(NYX_ERROR_NONE == _charger_query_charger_event(&returnedEvent));
	g_assert_true(returnedEvent == NYX_CHARGER_CONNECTED);

	// setup return value and force expected event
	returnedEvent = NYX_CHARGER_FAULT;
	current_event = NYX_CHARGE_COMPLETE;

	// Call function; check for no error and test returned value
	g_assert_true(NYX_ERROR_NONE == _charger_query_charger_event(&returnedEvent));
	g_assert_true(returnedEvent == NYX_CHARGE_COMPLETE);

	// NOTE: We don't bother passing NULL for status since status is checked in charger_query_charger_event() in chargerlib.c
}

//
// Set-up GLib, then register and run the tests.
int main(int argc, char **argv)
{
	g_test_init(&argc, &argv, NULL);

	g_test_add_func("/charger/device/_charger_init", test__charger_init);
	g_test_add_func("/charger/device/_charger_read_status",
	                test__charger_read_status);
	g_test_add_func("/charger/device/_charger_enable_charging",
	                test__charger_enable_charging);
	g_test_add_func("/charger/device/_charger_disable_charging",
	                test__charger_disable_charging);
	g_test_add_func("/charger/device/_charger_query_charger_event",
	                test__charger_query_charger_event);

	// TODO: Add test for _handle_power_supply_event() callback function?

	return g_test_run();
}
