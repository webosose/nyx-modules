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

#include <cstring>
#include "parser_interface.h"

#include <future>

#include "parser_nmea.h"
#include "parser_thread_pool.h"


#ifdef __cplusplus
extern "C" {
#endif

static bool parsing_engine_on = false;
static ParserNmea* parserNmeaObj = nullptr;
static ParserThreadPool* parserThreadPoolObj = nullptr;

static gps_location_callback gps_loc_cb = nullptr;
static gps_status_callback gps_status_cb = nullptr;
static gps_sv_status_callback gps_sv_cb = nullptr;
static gps_nmea_callback gps_nmea_cb = nullptr;
static gps_set_capabilities  gps_set_capa_cb = nullptr;
static gps_acquire_wakelock gps_acq_lock_cb = nullptr;
static gps_release_wakelock gps_rel_lock_cb = nullptr;
static gps_request_utc_time gps_req_utc_cb = nullptr;
static gps_create_thread gps_cre_thr_cb = nullptr;

void parser_loc_cb(GpsLocation* location, void* locExt) {
    if (parsing_engine_on && gps_loc_cb)
        gps_loc_cb(location);
}

void parser_sv_cb(GpsSvStatus* sv_status, void* svExt) {
    if (parsing_engine_on && gps_sv_cb)
        gps_sv_cb(sv_status);
}

void parser_status_cb(GpsStatus *gps_status, void* statusExt) {
    if(gps_status_cb)
        gps_status_cb(gps_status);
}

void parser_nmea_cb(GpsUtcTime now, const char *buff, int len) {
    if(parsing_engine_on && gps_nmea_cb){
        gps_nmea_cb(now, buff, len);
    }
}

// Function declarations for sLocEngInterface
static int  loc_init(GpsCallbacks* callbacks);
static int  loc_start();
static int  loc_stop();
static void loc_cleanup();

// Defines the GpsInterface
static const GpsInterface sLocEngInterface =
{
   sizeof(GpsInterface),
   loc_init,
   loc_start,
   loc_stop,
   loc_cleanup
};

const GpsInterface* get_gps_interface() {
    return &sLocEngInterface;
}

ParserThreadPool* getThreadInstance() {
    static ParserThreadPool threadObj(1);
    return &threadObj;
}

static int loc_init(GpsCallbacks* callbacks)
{
    if (callbacks) {
        gps_loc_cb = callbacks->location_cb;
        gps_sv_cb = callbacks->sv_status_cb;
        gps_status_cb = callbacks->status_cb;
        gps_nmea_cb = callbacks->nmea_cb;
        gps_set_capa_cb = callbacks->set_capabilities_cb;
        gps_acq_lock_cb = callbacks->acquire_wakelock_cb;
        gps_rel_lock_cb = callbacks->release_wakelock_cb;
        gps_req_utc_cb = callbacks->request_utc_time_cb;
        gps_cre_thr_cb = callbacks->create_thread_cb;
    }

    parserNmeaObj = ParserNmea::getInstance();
    parserThreadPoolObj = getThreadInstance();

    //parserThreadPoolObj->enqueue(&SetGpsStatus, NYX_GPS_STATUS_ENGINE_ON);
    return 0;
}

static int  loc_start() {

    if (parserNmeaObj && !parserNmeaObj->initParsingModule())
        return -1;

    parserThreadPoolObj->enqueue(&startParsing);
    return 0;
}

static int  loc_stop() {

    if (parserNmeaObj && !parserNmeaObj->deinitParsingModule())
        return 0;

    parserThreadPoolObj->enqueue(&stopParsing);
    return 0;
}

static void loc_cleanup() {
    SetGpsStatus(NYX_GPS_STATUS_ENGINE_OFF);
    parsing_engine_on = false;
    gps_loc_cb = nullptr;
    gps_sv_cb = nullptr;
    gps_status_cb = nullptr;
    gps_nmea_cb = nullptr;
    gps_set_capa_cb = nullptr;
    gps_acq_lock_cb = nullptr;
    gps_rel_lock_cb = nullptr;
    gps_req_utc_cb = nullptr;
    gps_cre_thr_cb = nullptr;
}

bool startParsing() {
    parsing_engine_on = true;

    if (parserNmeaObj)
        return parserNmeaObj->startParsing();
    return false;
}

bool stopParsing() {
    parsing_engine_on = false;

    if (parserNmeaObj)
        return parserNmeaObj->stopParsing();
   return false;
}

#ifdef __cplusplus
}
#endif
