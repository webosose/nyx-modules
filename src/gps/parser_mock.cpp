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
#include <sys/time.h>
#include <thread>
#include <time.h>

#include <nyx/module/nyx_log.h>
#include "parser_thread_pool.h"
#include "gps_storage.h"
#include "parser_inotify.h"
#include "parser_interface.h"

const std::string nmea_file_path = "/media/internal/location";
const std::string nmea_file_name = "gps.nmea";
const std::string nmea_complete_path = nmea_file_path + "/" + nmea_file_name;

ParserMock::ParserMock()
    : mNmeaFp(nullptr)
    , mSeekOffset(0)
    , mStopParser(false)
    , mParserInotifyObj(nullptr)
    , mParserThreadPoolObj(nullptr)
    , mParserRequested(false)
{

    mParserInotifyObj = new ParserInotify(nmea_file_path, this);
}

ParserMock::~ParserMock()
{

    if (mNmeaFp)
    {
        fclose(mNmeaFp);
    }

    if (mParserInotifyObj)
    {
        delete mParserInotifyObj;
        mParserInotifyObj = nullptr;
    }

    if (mParserThreadPoolObj)
    {
        delete mParserThreadPoolObj;
        mParserThreadPoolObj = nullptr;
    }
}

ParserMock *ParserMock::getInstance()
{
    static ParserMock parserMockObj;
    return &parserMockObj;
}

bool ParserMock::init()
{

    if(!isSourcePresent())
      return false;

    mParserRequested = true;

    return true;
}

bool ParserMock::deinit()
{
    mParserRequested = false;

    if (mParserThreadPoolObj)
    {
        delete mParserThreadPoolObj;
        mParserThreadPoolObj = nullptr;
    }

    if (mNmeaFp)
    {
        mStopParser = true;
        fclose(mNmeaFp);
        mNmeaFp = nullptr;
    }

    mSeekOffset = 0;

    SetGpsStatus(NYX_GPS_STATUS_SESSION_END);
    return true;
}

bool ParserMock::isMockEnabled()
{
    //check mock enabled or not
    GKeyFile *keyfile = load_conf_file(mock_conf_path_name);
    if (!keyfile)
    {
        nyx_error("MSGID_NMEA_PARSER_MOCK", 0, "mock config file loading failed");
        return false;
    }

    bool value = g_key_file_get_boolean(keyfile, GPS_MOCK_INFO, "MOCK", NULL);
    if (!value)
    {
        g_key_file_free(keyfile);
        return false;
    }

    g_key_file_free(keyfile);
    return true;
}

bool ParserMock::isSourcePresent()
{
    FILE *fp = fopen(nmea_complete_path.c_str(), "r");
    if (fp == nullptr)
    {
        nyx_error("MSGID_NMEA_PARSER_MOCK", 0, "Fun: %s, Line: %d Could not open file: %s \n", __FUNCTION__, __LINE__, nmea_complete_path.c_str());
        return false;
    }
    fclose(fp);
    fp = nullptr;
    return true;
}

bool ParserMock::createThreadPool()
{
    if (!mParserThreadPoolObj)
    {
        int latency, interval;
        latency = getMockLatency();
        interval = latency/2;

        mParserThreadPoolObj = new ParserThreadPool(1, interval);

        if(!mParserThreadPoolObj)
        {
          return false;
        }
        nyx_info("MSGID_NMEA_PARSER_MOCK", 0, "Created Mock ThreadPool with interval: %d \n", interval);
    }
    return true;
}

int ParserMock::getMockLatency()
{
    int latency;

    GKeyFile *keyfile = load_conf_file(mock_conf_path_name);
    if (!keyfile)
    {
        nyx_error("MSGID_NMEA_PARSER_MOCK", 0, "mock config file loading failed");
        return false;
    }

    latency = g_key_file_get_integer(keyfile, GPS_MOCK_INFO, "LATENCY", NULL);

    if (!latency)
    {
        nyx_info("MSGID_NMEA_PARSER_MOCK", 0, "config file latency not available so default latency:%d\n", DEFAULT_LATENCY);
        latency = DEFAULT_LATENCY;
    }

    g_key_file_free(keyfile);
    return latency;
}

bool ParserMock::startParsing()
{
    nyx_info("MSGID_NMEA_PARSER_MOCK", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    if (!isMockEnabled())
    {
        nyx_info("MSGID_NMEA_PARSER_MOCK", 0, "Mock is disabled\n");
        return false;
    }

    mNmeaFp = fopen(nmea_complete_path.c_str(), "r");
    if (mNmeaFp == nullptr)
    {
        nyx_error("MSGID_NMEA_PARSER_MOCK", 0, "Fun: %s, Line: %d Could not open file: %s \n", __FUNCTION__, __LINE__, nmea_complete_path.c_str());
        return false;
    }

    createThreadPool();

    SetGpsStatus(NYX_GPS_STATUS_SESSION_BEGIN);

    if (mSeekOffset)
    {
        (void)fseek(mNmeaFp, mSeekOffset, SEEK_SET);
    }

    char pBuff[1024];
    while (mNmeaFp && feof(mNmeaFp) == 0)
    {

        if (mStopParser)
        {
            mStopParser = false;
            fclose(mNmeaFp);
            mNmeaFp = nullptr;
            return true;
        }

        memset(&pBuff, 0, sizeof(pBuff));
        size_t nBytesRead = fread(pBuff, 1, 512, mNmeaFp);
        CNMEAParserData::ERROR_E nErr;
        if ((nErr = CNMEAParser::ProcessNMEABuffer(pBuff, nBytesRead)) != CNMEAParserData::ERROR_OK)
        {
            nyx_error("MSGID_NMEA_PARSER_MOCK", 0, "Fun: %s, Line: %d error: %d \n", __FUNCTION__, __LINE__, nErr);
            return false;
        }
        mSeekOffset += nBytesRead;
    }

    if (mNmeaFp)
    {
        fclose(mNmeaFp);
        mNmeaFp = nullptr;
    }

    if (mParserInotifyObj)
        mParserInotifyObj->startWatch();

    return false;
}

bool ParserMock::stopParsing()
{

    if (mParserInotifyObj)
        mParserInotifyObj->stopWatch();

    return true;
}

void ParserMock::parserWatchCb(const char *ident)
{
    if (!ident)
        return;

    if ((strlen(ident) != nmea_file_name.size()) || strncmp(ident, nmea_file_name.c_str(), nmea_file_name.size()) != 0)
        return;

    if (mParserInotifyObj)
        mParserInotifyObj->stopWatch();
    startParsing();
}
