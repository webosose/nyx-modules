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
#include "gps_config.h"

constexpr char GPS_CONFIG_FILE[] = "/etc/location/gpsConfig.conf";
constexpr char DEVICE_DEFAULT_PORT[] = "/dev/ttyUSB0";
constexpr int INVALID_FD = -1;
constexpr int MAX_BUFFER_SIZE = 1024;

class GPSDevice
{
public:
    GPSDevice();
    ~GPSDevice();

    static GPSDevice *getInstance();
    bool isGpsDevAvail();
    bool init();
    bool deinit();
    void gpsDeviceDestroyed();
    gboolean readGpsData(GIOChannel *, GIOCondition);
    static gboolean ioCallback(GIOChannel *, GIOCondition, gpointer);

private:

    int mFd;
    bool mGpsDevAvail;
    GIOChannel *mReadChannel;
    guint mIoWatchId;
    GPSConfig mGPSConfig;
    std::string mData;
    std::string mPort;
    void handleGpsData();

    bool openPort();
    void setReadChannel();
    void configGPSDevicePort();

};

#endif /* GPSDEVICE_H_ */
