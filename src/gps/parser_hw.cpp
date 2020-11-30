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
 * * SPDX-License-Identifier: Apache-2.0
 * *
 * * Author(s)    : Ashish Patel
 * * Email ID.    : ashish23.patel@lge.com
 * * LICENSE@@@ */

/*
 * *******************************************************************/

#include "parser_hw.h"
#include "parser_nmea.h"
#include <nyx/common/nyx_gps_common.h>
#include <cstring>
#include <sys/time.h>
#include <thread>
#include <time.h>

#include <nyx/module/nyx_log.h>

ParserHW::ParserHW()
{
    mGPSDeviceObj = GPSDevice::getInstance();
}

ParserHW::~ParserHW()
{
}
ParserHW *ParserHW::getInstance()
{
    static ParserHW ParserHWObj;
    return &ParserHWObj;
}

bool ParserHW::init()
{
  return mGPSDeviceObj->init();
}

bool ParserHW::deinit()
{
    return mGPSDeviceObj->isGpsDevAvail()?mGPSDeviceObj->deinit():false;
}

bool ParserHW::startParsing()
{
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    if (init())
        SetGpsStatus(NYX_GPS_STATUS_SESSION_BEGIN);

    return false;
}

bool ParserHW::stopParsing()
{
    if (deinit())
        SetGpsStatus(NYX_GPS_STATUS_SESSION_END);

    return false;
}
