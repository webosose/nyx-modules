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

#ifndef GPSCONFIG_H_
#define GPSCONFIG_H_

#include <pbnjson.hpp>

class GPSConfig
{
public:
    GPSConfig();
    GPSConfig(const std::string &fileName);
    ~GPSConfig();
    bool isGPSConfigured();
    bool loadGPSConfig(const std::string &fileName);
    std::string getValue(const std::string &prop);

private:
    bool mGPSConfig;
    pbnjson::JValue mRoot;
};

#endif /* GPSCONFIG_H_ */

