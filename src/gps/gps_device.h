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

#ifndef GPSDEVICE_H_
#define GPSDEVICE_H_

#include <string>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <termios.h>
#include <string.h>
#include <iostream>
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <errno.h>
#include <gio/gio.h>
#include "parser_nmea.h"

constexpr char GPS_DEVICE_INFO[] = "GPSDEVICE";
constexpr char GPS_CONFIG_FILE[] = "/etc/location/gpsConfig.conf";
constexpr char DEVICE_DEFAULT_PORT[] = "/dev/ttyUSB0";
constexpr int INVALID_FD = -1;
constexpr int MAX_BUFFER_SIZE = 1024;

class GPSDevice : public ParserNmea
{
public:
    GPSDevice();
    ~GPSDevice();

    static GPSDevice *getInstance();
    bool isGpsDevAvail();
    bool init();
    bool deinit();

private:
    int mFd;
    bool mGpsDevAvail;
    GKeyFile *mKeyfile;
    GIOChannel *mReadChannel;
    guint mIoWatchId;
    std::string mData;
    std::string mPort;
    void handleGpsData();
    bool isGPSConfigured();
    bool loadGPSConfig(const std::string &fileName);
    std::string getValue(const std::string &key);
    bool openPort();
    void setReadChannel();
    void configGPSDevicePort();
    void gpsDeviceDestroyed();
    gboolean readGpsData(GIOChannel *, GIOCondition);
    static gboolean ioCallback(GIOChannel *, GIOCondition, gpointer);
};

#endif /* GPSDEVICE_H_ */
