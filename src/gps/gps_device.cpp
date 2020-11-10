// Copyright (c) 2020 LG Electronics, Inc.
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

#include <nyx/module/nyx_log.h>
#include "gps_device.h"
#include "parser_nmea.h"

GPSDevice::GPSDevice()
    : mFd(INVALID_FD)
    , mInit(false)
    , mReadChannel(nullptr)
{
    init();
}

GPSDevice::~GPSDevice() {
    if (mReadChannel) {
        g_io_channel_shutdown(mReadChannel, true, NULL);
        g_io_channel_unref(mReadChannel);
    }
    if (mFd != INVALID_FD) {
        close(mFd);
    }
}

bool GPSDevice::isGpsDevAvail()
{
    return mInit;
}

void GPSDevice::handleGpsData()
{
    std::string cmd;
    std::string chsum;
    std::string gpsData;
    nyx_info("MSGID_NMEA_PARSER", 0, "%s", __FUNCTION__);
    if (!mData.empty() && mData.find(",") != std::string::npos)
    {
        cmd = mData.substr(1, mData.find(",")-1);
    }
    nyx_debug("GPS_DEVICE : %s -> Len:%zu cmd: [%s]", __FUNCTION__, cmd.size(), cmd.c_str());
    if (!mData.empty() && mData.find("\r") != std::string::npos)
    {
        std::string tmp = mData.replace(mData.find("\r"), mData.size(), "");
        nyx_debug("GPS_DEVICE : %s -> Len:%zu tmp: [%s]", __FUNCTION__, tmp.size(), tmp.c_str());
        if (!tmp.empty() && tmp.find("*") != std::string::npos)
        {
            int pos = tmp.find("*");
            int pos1 = tmp.find(",");
            chsum = tmp.substr(pos+1);
            gpsData= tmp.substr(pos1+1);
            nyx_debug("GPS_DEVICE : %s -> Len:%zu chsum: [%s]", __FUNCTION__, chsum.size(), chsum.c_str());
            gpsData = gpsData.substr(0, gpsData.find("*"));
            nyx_debug("GPS_DEVICE : %s -> Len:%zu gpsData: [%s]", __FUNCTION__, gpsData.size(), gpsData.c_str());
            ParserNmea::getInstance()->ProcessCommand((cmd.c_str()),
                    (gpsData.c_str()), (chsum.c_str()));
        }
    }
}

gboolean GPSDevice::readGpsData(GIOChannel *io, GIOCondition condition) {
    nyx_info("MSGID_NMEA_PARSER", 0, "%s", __FUNCTION__);
    char msg[MAX_BUFFER_SIZE + 1] = { 0 };
    GError *err = NULL;
    gsize len = 0;
    if (G_IO_STATUS_NORMAL == g_io_channel_read_chars (io, msg, sizeof(msg), &len, &err))
    {
        std::string temp = msg;
        nyx_debug("GPS_DEVICE : %s msg:[%s]", __FUNCTION__,msg);
        mData += temp;
        if (!mData.empty() && mData.find("\n") != std::string::npos)
        {
            nyx_debug("GPS_DEVICE : %s before mData:[%s]", __FUNCTION__,mData.c_str());
            handleGpsData();
            mData.clear();
        }
        return TRUE;
    }
    return FALSE;
}

gboolean GPSDevice::ioCallback(GIOChannel *io, GIOCondition condition, gpointer user_data)
{
    nyx_info("MSGID_NMEA_PARSER", 0, "%s", __FUNCTION__);
    GPSDevice *ptr = (GPSDevice *)user_data;
    return ptr->readGpsData(io, condition);
}

bool GPSDevice::init() {
    nyx_info("MSGID_NMEA_PARSER", 0, "%s", __FUNCTION__);
    if ((mFd == INVALID_FD) && (false == mInit))
    {
        mInit = true;
        struct termios tty;
        memset(&tty, 0, sizeof(termios));
        if ((mFd = open(DEVICE_PORT, O_RDONLY | O_NOCTTY | O_NONBLOCK )) != -1)
        {
            tty.c_iflag = 0;
            tty.c_cflag |= CLOCAL | CREAD;
            tcflush(mFd, TCIOFLUSH);
            tcsetattr(mFd, TCSANOW, &tty);
            tcflush(mFd, TCIOFLUSH);
            tcflush(mFd, TCIOFLUSH);
            cfsetospeed(&tty, B4800);
            cfsetispeed(&tty, B4800);
            cfmakeraw (&tty);
            tcsetattr(mFd, TCSANOW, &tty);

            mReadChannel = g_io_channel_unix_new(mFd);
            g_io_channel_set_close_on_unref(mReadChannel, TRUE);
            g_io_channel_set_encoding(mReadChannel, NULL, NULL);
            g_io_channel_set_buffered(mReadChannel, FALSE);
            g_io_add_watch (mReadChannel, G_IO_IN, ioCallback, this);
            nyx_info("MSGID_NMEA_PARSER", 0, "g_io_add_watch");
            return true;
        }
        else
        {
            mInit = false;
            nyx_info("MSGID_NMEA_PARSER", 0,  "%s open failed", DEVICE_PORT);
            return false;
        }
    }
    return true;
}


bool GPSDevice::deinit() {
    nyx_info("MSGID_NMEA_PARSER", 0,  "%s", __FUNCTION__);
    if (mReadChannel) {
        g_io_channel_shutdown(mReadChannel, true, NULL);
        g_io_channel_unref(mReadChannel);
    }
    if (mFd != INVALID_FD)
    {
        close(mFd);
    }
    mReadChannel = nullptr;
    mFd = INVALID_FD;
    mData.clear();
    mInit = false;

    return true;
}

