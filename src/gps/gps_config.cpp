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
#include <pbnjson.hpp>
#include "gps_config.h"

GPSConfig::GPSConfig() : mIsInit(false)
{
}

GPSConfig::GPSConfig(const std::string &fileName) : mIsInit(false)
{
    init(fileName);
}

bool GPSConfig::init(const std::string &fileName)
{
    if (!mIsInit)
    {
        mRoot = pbnjson::JDomParser::fromFile(fileName.c_str());
        if (!mRoot.isValid() || mRoot.isNull())
            mIsInit = false;
        else
            mIsInit = true;
    }
    return mIsInit;
}

bool GPSConfig::isInitialized()
{
    return mIsInit;
}

std::string GPSConfig::getValue(const std::string &prop)
{
    std::string propVal;
    if (mIsInit && mRoot.hasKey(prop))
    {
        propVal = mRoot[prop.c_str()].asString();
    }
    return propVal;
}

GPSConfig::~GPSConfig()
{
}

