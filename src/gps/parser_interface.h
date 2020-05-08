/* @@@LICENSE
 * *
 * * Copyright (c) 2020 LG Electronics, Inc.
 * *
 * * Licensed under the Apache License, Version 2.0 (the "License");
 * * you may not use this file except in compliance with the License.
 * * You may obtain a copy of the License at
 * *
 * * http://www.apache.org/licenses/LICENSE-2.0
 * *
 * * Unless required by applicable law or agreed to in writing, software
 * * distributed under the License is distributed on an "AS IS" BASIS,
 * * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * * See the License for the specific language governing permissions and
 * * limitations under the License.
 * *
 * * LICENSE@@@ */

/*
 * *******************************************************************/

#ifndef _PARSER_INTERFACE_H_
#define _PARSER_INTERFACE_H_

#include <nyx/common/nyx_gps_common.h>
#include <nyx/module/nyx_log.h>

#ifdef __cplusplus
extern "C" {
#endif

    bool startParsing();
    bool stopParsing();

#define GpsUtcTime                          int64_t

#define GPS_LOCATION_MAP_URL_SIZE           400
#define GPS_LOCATION_MAP_INDEX_SIZE         16

typedef struct {
    /** set to sizeof(GpsLocation) */
    size_t          size;
    /** Contains GpsLocationFlags bits. */
    uint16_t        flags;
    /* Provider indicator for HYBRID or GPS */
    uint16_t        position_source;
    /** Represents latitude in degrees. */
    double          latitude;
    /** Represents longitude in degrees. */
    double          longitude;
    /** Represents altitude in meters above the WGS 84 reference
     * ellipsoid. */
    double          altitude;
    /** Represents speed in meters per second. */
    float           speed;
    /** Represents heading in degrees. */
    float           bearing;
    /** Represents expected accuracy in meters. */
    float           accuracy;
    /** Timestamp for the location fix. */
    int64_t         timestamp;
    /*allows HAL to pass additional information related to the location */
    int             rawDataSize;         /* in # of bytes */
    void            * rawData;
    bool            is_indoor;
    float           floor_number;
    char            map_url[GPS_LOCATION_MAP_URL_SIZE];
    unsigned char   map_index[GPS_LOCATION_MAP_INDEX_SIZE];
} webos_gps_location;

#define GpsLocation             webos_gps_location
#define GpsStatus               nyx_gps_status_t
#define GpsSvInfo               nyx_gps_sv_info_t
#define GpsSvStatus             nyx_gps_sv_status_t

typedef void (* gps_location_callback)(GpsLocation* location);
typedef void (* gps_status_callback)(GpsStatus* status);
typedef void (* gps_sv_status_callback)(GpsSvStatus* sv_info);
typedef void (* gps_nmea_callback)(GpsUtcTime timestamp, const char* nmea, int length);
typedef void (* gps_set_capabilities)(uint32_t capabilities);
typedef void (* gps_acquire_wakelock)();
typedef void (* gps_release_wakelock)();
typedef void (* gps_request_utc_time)();
typedef pthread_t (* gps_create_thread)(const char* name, void (*start)(void *), void* arg);

typedef struct {
    size_t      size;
    gps_location_callback location_cb;
    gps_status_callback status_cb;
    gps_sv_status_callback sv_status_cb;
    gps_nmea_callback nmea_cb;
    gps_set_capabilities set_capabilities_cb;
    gps_acquire_wakelock acquire_wakelock_cb;
    gps_release_wakelock release_wakelock_cb;
    gps_create_thread create_thread_cb;
    gps_request_utc_time request_utc_time_cb;
} webos_gps_callbacks;

#define GpsCallbacks                    webos_gps_callbacks

typedef struct {
    size_t size;
    int   (*init)( webos_gps_callbacks* callbacks );
    int   (*start)( void );
    int   (*stop)( void );
    void  (*cleanup)( void );
} webos_gps_interface;

#define GpsInterface                    webos_gps_interface

const GpsInterface* get_gps_interface();

void parser_loc_cb(GpsLocation* location, void* locExt);
void parser_sv_cb(GpsSvStatus* sv_status, void* svExt);
void parser_status_cb(GpsStatus* gps_status, void* statusExt);
void parser_nmea_cb(GpsUtcTime now, const char *buff, int len);

#ifdef __cplusplus
}
#endif

#endif // _PARSER_INTERFACE_H_
