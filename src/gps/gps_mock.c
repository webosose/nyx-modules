/* @@@LICENSE
*
* Copyright (c) 2020 LG Electronics, Inc.
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
* LICENSE@@@ */

/*
*******************************************************************/

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>


NYX_DECLARE_MODULE(NYX_DEVICE_GPS, "GpsMock");

typedef struct methodStringPair
{
	module_method_t mType;
	const char * mString;
} methodStringPair_t;

static const methodStringPair_t mapMethodString[] = {
    {NYX_GPS_INIT_MODULE_METHOD,                        "init"},
    {NYX_GPS_START_MODULE_METHOD,                       "start"},
    {NYX_GPS_STOP_MODULE_METHOD,                        "stop"},
    {NYX_GPS_CLEANUP_MODULE_METHOD,                     "cleanup"},
    {NYX_GPS_INJECT_TIME_MODULE_METHOD,                 "inject_time"},
    {NYX_GPS_INJECT_LOCATION_MODULE_METHOD,             "inject_location"},
    {NYX_GPS_DELETE_AIDING_DATA_MODULE_METHOD,          "delete_aiding_data"},
    {NYX_GPS_SET_POSITION_MODE_MODULE_METHOD,           "set_position_mode"},
    {NYX_GPS_INJECT_EXTRA_CMD_MODULE_METHOD,            "inject_extra_cmd"},
    {NYX_GPS_INJECT_XTRA_DATA_MODULE_METHOD,            "inject_xtra_data"},
    {NYX_GPS_DATA_CONN_OPEN_MODULE_METHOD,              "data_conn_open"},
    {NYX_GPS_DATA_CONN_CLOSED_MODULE_METHOD,            "data_conn_closed"},
    {NYX_GPS_DATA_CONN_FAILED_MODULE_METHOD,            "data_conn_failed"},
    {NYX_GPS_SET_SERVER_MODULE_METHOD,                  "set_server"},
    {NYX_GPS_SEND_NI_RESPONSE_MODULE_METHOD,            "send_ni_response"},
    {NYX_GPS_SET_REF_LOCATION_MODULE_METHOD,            "set_ref_location"},
    {NYX_GPS_SET_SET_ID_MODULE_METHOD,                  "set_set_id"},
    {NYX_GPS_SEND_NI_MESSAGE_MODULE_METHOD,             "send_ni_message"},
    {NYX_GPS_UPDATE_NETWORK_STATE_MODULE_METHOD,        "update_network_state"},
    {NYX_GPS_UPDATE_NETWORK_AVAILABILITY_MODULE_METHOD, "update_network_availability"},
    {NYX_GPS_ADD_GEOFENCE_AREA_MODULE_METHOD,           "add_geofence_area"},
    {NYX_GPS_REMOVE_GEOFENCE_AREA_MODULE_METHOD,        "remove_geofence_area"},
    {NYX_GPS_PAUSE_GEOFENCE_MODULE_METHOD,              "pause_geofence"},
    {NYX_GPS_RESUME_GEOFENCE_MODULE_METHOD,             "resume_geofence"},
    {NYX_GPS_INIT_XTRA_CLIENT_MODULE_METHOD,            "init_xtra_client"},
    {NYX_GPS_STOP_XTRA_CLIENT_MODULE_METHOD,            "stop_xtra_client"},
    {NYX_GPS_DOWNLOAD_XTRA_DATA_MODULE_METHOD,          "download_xtra_data"},
    {NYX_GPS_DOWNLOAD_NTP_TIME_MODULE_METHOD,           "download_ntp_time"}
};

nyx_device_t *nyx_dev = NULL;

nyx_gps_callbacks_t             *nyx_gps_cbs = NULL;
nyx_gps_xtra_callbacks_t        *nyx_gps_xtra_cbs = NULL;
nyx_agps_callbacks_t            *nyx_agps_cbs = NULL;
nyx_gps_ni_callbacks_t          *nyx_gps_ni_cbs = NULL;
nyx_agps_ril_callbacks_t        *nyx_agps_ril_cbs = NULL;
nyx_gps_geofence_callbacks_t    *nyx_gps_geofence_cbs = NULL;

nyx_error_t nyx_module_open(nyx_instance_t instance, nyx_device_t **device_ptr)
{
    char nyx_gps_module_path[256];
    int cnt, arraySize;
    nyx_error_t error = NYX_ERROR_INCOMPATIBLE_LIBRARY;

    if (device_ptr == NULL) {
        return NYX_ERROR_INVALID_VALUE;
    }

    if (nyx_dev) {
        *device_ptr = (nyx_device_t *)nyx_dev;
        return NYX_ERROR_NONE;
    }

    if (!(nyx_dev = (nyx_device_t *)calloc(1, sizeof(nyx_device_t)))) {
        error = NYX_ERROR_OUT_OF_MEMORY;
        goto ERROR_HANDLER;
    }

    if ((error = nyx_module_set_description(instance, nyx_dev, "Module to drive the Qualcomm GPS")) != NYX_ERROR_NONE) {
        nyx_error("MSGID_NYX_MOD_GPS_SET_DESCRIBE_ERR", 0, "Failed to set GPS nyx module description");
        goto ERROR_HANDLER;
    }

    nyx_debug("Succeeded to set GPS nyx module description");

    if ((error = nyx_module_set_name(instance, nyx_dev, "GPS Qualcomm")) != NYX_ERROR_NONE) {
        nyx_error("MSGID_NYX_MOD_GPS_SET_NAME_ERR", 0, "Failed to set GPS nyx module name");
        goto ERROR_HANDLER;
    }

    nyx_debug("Succeeded to set GPS nyx module name");

    for (cnt = 0, arraySize = sizeof(mapMethodString) / sizeof(mapMethodString[0]); cnt < arraySize; cnt++) {
        error = nyx_module_register_method(instance, nyx_dev, mapMethodString[cnt].mType, mapMethodString[cnt].mString);

        if (error != NYX_ERROR_NONE) {
            nyx_error("MSGID_NYX_MOD_METHOD_REGISTER_ERR", 0, "Failed to register GPS nyx module methods");
            goto ERROR_HANDLER;
        }
    }

    nyx_debug("Succeeded to register GPS nyx module methods");

    *device_ptr = (nyx_device_t *)nyx_dev;

    nyx_debug("Successfully open nyx GPS module!");

    return NYX_ERROR_NONE;

ERROR_HANDLER:

    if (nyx_dev) {
        free(nyx_dev);
        nyx_dev = NULL;
    }

    return error;
}

nyx_error_t nyx_module_close(nyx_device_t *device)
{
    return NYX_ERROR_NONE;
}

nyx_error_t init(nyx_device_handle_t handle,
                 nyx_gps_callbacks_t *gps_cbs,
                 nyx_gps_xtra_callbacks_t *xtra_cbs,
                 nyx_agps_callbacks_t *agps_cbs,
                 nyx_gps_ni_callbacks_t *gps_ni_cbs,
                 nyx_agps_ril_callbacks_t *agps_ril_cbs,
                 nyx_gps_geofence_callbacks_t *geofence_cbs)
{
    if (nyx_dev == NULL)
        return NYX_ERROR_DEVICE_NOT_EXIST;

    if (handle != nyx_dev || gps_cbs == NULL)
        return NYX_ERROR_INVALID_HANDLE;

    nyx_debug("Called GPS init");

    nyx_gps_cbs = gps_cbs;
    nyx_gps_xtra_cbs = xtra_cbs;
    nyx_agps_cbs = agps_cbs;
    nyx_gps_ni_cbs = gps_ni_cbs;
    nyx_agps_ril_cbs = agps_ril_cbs;
    nyx_gps_geofence_cbs = geofence_cbs;

    return NYX_ERROR_NONE;
}

nyx_error_t start(nyx_device_handle_t handle)
{
    return NYX_ERROR_NONE;
}

nyx_error_t stop(nyx_device_handle_t handle)
{
    return NYX_ERROR_NONE;
}

nyx_error_t cleanup(nyx_device_handle_t handle)
{
    return NYX_ERROR_NONE;
}

nyx_error_t inject_time(nyx_device_handle_t handle,
                        int64_t time,
                        int64_t timeReference,
                        int uncertainty)
{
    return NYX_ERROR_NONE;
}

nyx_error_t inject_location(nyx_device_handle_t handle,
                            double latitude,
                            double longitude,
                            float accuracy)
{
    return NYX_ERROR_NONE;
}

nyx_error_t delete_aiding_data(nyx_device_handle_t handle, nyx_gps_aiding_data_t flags)
{
    return NYX_ERROR_NONE;
}

nyx_error_t set_position_mode(nyx_device_handle_t handle,
                              nyx_gps_position_mode_t mode,
                              nyx_gps_position_recurrence_t recurrence,
                              uint32_t min_interval,
                              uint32_t preferred_accuracy,
                              uint32_t preferred_time)
{
    return NYX_ERROR_NONE;
}

nyx_error_t inject_extra_cmd(nyx_device_handle_t handle, char *cmd, int length)
{
    return NYX_ERROR_NONE;
}

nyx_error_t inject_xtra_data(nyx_device_handle_t handle, char *data, int length)
{
    return NYX_ERROR_NONE;
}

nyx_error_t data_conn_open(nyx_device_handle_t handle,
                           nyx_agps_type_t agpsType,
                           const char *apn,
                           nyx_agps_bearer_type_t bearerType)
{
    return NYX_ERROR_NONE;
}

nyx_error_t data_conn_closed(nyx_device_handle_t handle, nyx_agps_type_t agpsType)
{
    return NYX_ERROR_NONE;
}

nyx_error_t data_conn_failed(nyx_device_handle_t handle, nyx_agps_type_t agpsType)
{
    return NYX_ERROR_NONE;
}

nyx_error_t set_server(nyx_device_handle_t handle,
                       nyx_agps_type_t type,
                       const char *hostname,
                       int port)
{
    return NYX_ERROR_NONE;
}

nyx_error_t send_ni_response(nyx_device_handle_t handle,
                             int notif_id,
                             nyx_gps_ni_user_response_type_t user_response)
{
    return NYX_ERROR_NONE;
}

nyx_error_t set_ref_location(nyx_device_handle_t handle,
                             const nyx_agps_ref_location_t *agps_reflocation,
                             size_t sz_struct)
{
    return NYX_ERROR_NONE;
}

nyx_error_t set_set_id(nyx_device_handle_t handle,
                       nyx_agps_set_id_type_t type,
                       const char *set_id)
{
    return NYX_ERROR_NONE;
}

nyx_error_t send_ni_message(nyx_device_handle_t handle,
                            uint8_t *msg,
                            size_t len)
{
    return NYX_ERROR_NONE;
}

nyx_error_t update_network_state(nyx_device_handle_t handle,
                                 int connected,
                                 int type,
                                 int roaming,
                                 const char *extra_info)
{
    return NYX_ERROR_NONE;
}

nyx_error_t update_network_availability(nyx_device_handle_t handle,
                                        int available,
                                        const char *apn)
{
    return NYX_ERROR_NONE;
}

nyx_error_t add_geofence_area(nyx_device_handle_t handle,
                              int32_t geofence_id,
                              double latitude,
                              double longitude,
                              double radius_meters,
                              int last_transition,
                              int monitor_transitions,
                              int notification_responsiveness_ms,
                              int unknown_timer_ms)
{
    return NYX_ERROR_NONE;
}

nyx_error_t remove_geofence_area(nyx_device_handle_t handle, int32_t geofence_id)
{
    return NYX_ERROR_NONE;
}

nyx_error_t pause_geofence(nyx_device_handle_t handle, int32_t geofence_id)
{
    return NYX_ERROR_NONE;
}

nyx_error_t resume_geofence(nyx_device_handle_t handle,
                            int32_t geofence_id,
                            int monitor_transitions)
{
    return NYX_ERROR_NONE;
}

nyx_error_t init_xtra_client(nyx_device_handle_t handle,
                             nyx_gps_xtra_client_config_t *xtra_client_config,
                             nyx_gps_xtra_client_callbacks_t *xtra_client_cbs)
{
    return NYX_ERROR_NONE;
}

nyx_error_t stop_xtra_client(nyx_device_handle_t handle)
{
    return NYX_ERROR_NONE;
}

nyx_error_t download_xtra_data(nyx_device_handle_t handle)
{
    return NYX_ERROR_NONE;
}

nyx_error_t download_ntp_time(nyx_device_handle_t handle)
{
    return NYX_ERROR_NONE;
}
