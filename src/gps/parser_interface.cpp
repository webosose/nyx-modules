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

#include "parser_interface.h"

#include <future>

#include "parser_nmea.h"
#include "parser_thread_pool.h"


#ifdef __cplusplus
extern "C" {
#endif

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

void nmea_loc_cb(GpsLocation* location, void* locExt) {
    if (gps_loc_cb)
        gps_loc_cb(location);
}

void nmea_sv_cb(GpsSvStatus* sv_status, void* svExt) {
    if (gps_sv_cb)
        gps_sv_cb(sv_status);
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
    nyx_debug("MSGID_NMEA_PARSER Fun: %s, Line: %d", __FUNCTION__, __LINE__);
    return &sLocEngInterface;
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
    parserThreadPoolObj = new ParserThreadPool(1);
    return 1;
}

static int  loc_start() {
     parserThreadPoolObj->enqueue(&startParsing);
    return 0;
}

static int  loc_stop() {
    parserThreadPoolObj->enqueue(&stopParsing);
    return 0;
}

static void loc_cleanup() {
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
    nyx_debug("MSGID_NMEA_PARSER Fun: %s, Line: %d", __FUNCTION__, __LINE__);
    if (parserNmeaObj)
        return parserNmeaObj->startParsing();
    return false;
}

bool stopParsing() {
    nyx_debug("MSGID_NMEA_PARSER Fun: %s, Line: %d", __FUNCTION__, __LINE__);
    if (parserNmeaObj)
        return parserNmeaObj->stopParsing();
   return false;
}

#ifdef __cplusplus
}
#endif
