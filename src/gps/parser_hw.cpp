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
#include <nyx/common/nyx_gps_common.h>
#include <cstring>
#include <sys/time.h>
#include <thread>
#include <time.h>

#include <nyx/module/nyx_log.h>
#include "parser_thread_pool.h"
#include "gps_device.h"

ParserHW::ParserHW()
  : mParserThreadPoolObj(nullptr)
  , mParserRequested(false)
{
    mGPSDeviceObj = GPSDevice::getInstance();
}

ParserHW::~ParserHW()
{
    if (mParserThreadPoolObj)
    {
        delete mParserThreadPoolObj;
        mParserThreadPoolObj = nullptr;
    }
}
ParserHW *ParserHW::getInstance()
{
    static ParserHW ParserHWObj;
    return &ParserHWObj;
}

bool ParserHW::init()
{

    if(!mGPSDeviceObj->init())
      return false;

    mParserRequested = true;

    return true;
}

bool ParserHW::isSourcePresent()
{
    return mGPSDeviceObj->isGpsDevAvail();
}

bool ParserHW::deinit()
{
    mParserRequested = false;

    SetGpsStatus(NYX_GPS_STATUS_SESSION_END);

    return mGPSDeviceObj->isGpsDevAvail()?mGPSDeviceObj->deinit():false;

}

bool ParserHW::startParsing()
{
    nyx_info("MSGID_NMEA_PARSER_HW", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    if (isSourcePresent())
    {
        createThreadPool();
        SetGpsStatus(NYX_GPS_STATUS_SESSION_BEGIN);
    }
    return false;
}

bool ParserHW::stopParsing()
{

    if (mParserThreadPoolObj)
    {
        delete mParserThreadPoolObj;
        mParserThreadPoolObj = nullptr;
    }
    return false;
}

bool ParserHW::createThreadPool()
{
    if (!mParserThreadPoolObj)
    {
        unsigned int interval = 0;
        mParserThreadPoolObj = new ParserThreadPool(1, interval);
        if(!mParserThreadPoolObj)
          return false;
        nyx_info("MSGID_NMEA_PARSER_HW", 0, "Created HW ThreadPool with interval: %d \n", interval);
    }
    return true;
}