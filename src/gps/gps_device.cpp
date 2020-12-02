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

#include <nyx/module/nyx_log.h>
#include "gps_device.h"
#include "gps_storage.h"

GPSDevice::GPSDevice()
    : mFd(INVALID_FD)
    , mGpsDevAvail(false)
    , mReadChannel(nullptr)
    , mIoWatchId(0)
    , mKeyfile(nullptr)
{
}

GPSDevice::~GPSDevice()
{
    if (mReadChannel)
    {
        g_io_channel_shutdown(mReadChannel, true, NULL);
        g_io_channel_unref(mReadChannel);
        mReadChannel = NULL;
    }
    if (mFd != INVALID_FD)
    {
        close(mFd);
    }
    mData.clear();
}

GPSDevice *GPSDevice::getInstance()
{
    static GPSDevice GPSDeviceObj;
    return &GPSDeviceObj;
}

bool GPSDevice::isGpsDevAvail()
{
    return mGpsDevAvail;
}

void GPSDevice::handleGpsData()
{
    CNMEAParserData::ERROR_E nErr;

    if (!mData.empty() && mData.find("\r") != std::string::npos)
    {
        std::string recvData = mData.replace(mData.find("\r"), mData.length(), "");
        if ((nErr = CNMEAParser::ProcessNMEABuffer(const_cast<char *>(recvData.c_str()), recvData.length())) != CNMEAParserData::ERROR_OK)
        {
            nyx_error("GPS_DEVICE", 0, "ProcessNMEABuffer failed, error: %d \n", nErr);
        }
    }
}

gboolean GPSDevice::readGpsData(GIOChannel *io, GIOCondition condition)
{

    char msg[MAX_BUFFER_SIZE + 1] = {0};
    GError *err = NULL;
    gsize len = 0;
    if (G_IO_STATUS_NORMAL == g_io_channel_read_chars(io, msg, sizeof(msg), &len, &err))
    {
        std::string temp = msg;
        nyx_debug("GPS_DEVICE : %s msg:[%s]", __FUNCTION__, msg);
        mData += temp;
        if (!mData.empty() && mData.find("\n") != std::string::npos)
        {
            nyx_debug("GPS_DEVICE : %s received gps data:[%s]", __FUNCTION__, mData.c_str());
            handleGpsData();
            mData.clear();
        }
        return TRUE;
    }
    return FALSE;
}

gboolean GPSDevice::ioCallback(GIOChannel *io, GIOCondition condition, gpointer user_data)
{

    GPSDevice *ptr = (GPSDevice *)user_data;
    return ptr->readGpsData(io, condition);
}

bool GPSDevice::openPort()
{

    struct termios tty;
    memset(&tty, 0, sizeof(termios));
    bool openPort = false;

    if ((mFd = open(mPort.c_str(), O_RDONLY | O_NOCTTY | O_NONBLOCK)) != -1)
    {
        tty.c_iflag = 0;
        tty.c_cflag |= CLOCAL | CREAD;
        tcflush(mFd, TCIOFLUSH);
        tcsetattr(mFd, TCSANOW, &tty);
        tcflush(mFd, TCIOFLUSH);
        tcflush(mFd, TCIOFLUSH);
        cfsetospeed(&tty, B4800);
        cfsetispeed(&tty, B4800);
        cfmakeraw(&tty);
        tcsetattr(mFd, TCSANOW, &tty);
        openPort = true;
        nyx_info("GPS_DEVICE", 0, "%s Port Open Success", mPort.c_str());
    }
    else
    {
        nyx_error("GPS_DEVICE", 0, "%s Port Open failed", mPort.c_str());
    }
    return openPort;
}

void GPSDevice::setReadChannel()
{
    if (mFd == INVALID_FD)
    {
        nyx_error("GPS_DEVICE", 0, "Invalid fd[%d]", mFd);
        return;
    }

    if (mReadChannel == NULL)
    {
        mReadChannel = g_io_channel_unix_new(mFd);
        g_io_channel_set_close_on_unref(mReadChannel, TRUE);
        g_io_channel_set_encoding(mReadChannel, NULL, NULL);
        g_io_channel_set_buffered(mReadChannel, FALSE);
    }

    if (mReadChannel && !mIoWatchId)
    {
        GIOCondition watchCond = static_cast<GIOCondition>(G_IO_IN | G_IO_PRI);
        mIoWatchId = g_io_add_watch_full(mReadChannel, G_PRIORITY_HIGH, watchCond, ioCallback, this, NULL);
    }

    nyx_info("GPS_DEVICE", 0, "setReadChannel Success");
}

void GPSDevice::gpsDeviceDestroyed()
{
    nyx_info("GPS_DEVICE", 0, "gpsDeviceDestroyed called");
}

void GPSDevice::configGPSDevicePort()
{

    if (loadGPSConfig(GPS_CONFIG_FILE))
        mPort = getValue("PORT");
    else
        mPort = DEVICE_DEFAULT_PORT;
}

bool GPSDevice::init()
{
    nyx_info("GPS_DEVICE", 0, "%s", __FUNCTION__);

    configGPSDevicePort();

    if (!mGpsDevAvail && openPort())
    {
        setReadChannel();
        mGpsDevAvail = true;
    }

    return mGpsDevAvail;
}

bool GPSDevice::deinit()
{
    nyx_info("GPS_DEVICE", 0, "%s", __FUNCTION__);
    if (mReadChannel)
    {
        mIoWatchId = 0;
        g_io_channel_shutdown(mReadChannel, true, NULL);
        g_io_channel_unref(mReadChannel);
        mReadChannel = nullptr;
    }
    if (mFd != INVALID_FD)
    {
        close(mFd);
        mFd = INVALID_FD;
    }

    mData.clear();
    mGpsDevAvail = false;
    return true;
}

bool GPSDevice::loadGPSConfig(const std::string &fileName)
{

    mKeyfile = load_conf_file(fileName.c_str());
    if (mKeyfile)
    {
        nyx_info("MSGID_GPS_CONFIG", 0, "GPS conf file:%s loading success\n", fileName.c_str());
        return true;
    }
    nyx_error("MSGID_GPS_CONFIG", 0, "GPS conf file:%s load failed\n", fileName.c_str());
    return false;


}

std::string GPSDevice::getValue(const std::string &key)
{

    std::string data;
    if (mKeyfile)
    {
        gchar *value = g_key_file_get_string(mKeyfile, GPS_DEVICE_INFO, key.c_str(), NULL);
        if (!value)
        {
            nyx_error("MSGID_GPS_CONFIG", 0, "key:%s not present\n", key.c_str());
            return data;
        }
        data = value;
        g_free(value);
    }
    else
    {
        nyx_error("MSGID_GPS_CONFIG", 0, "GPS conf file not present\n");
    }

    return data;
}
