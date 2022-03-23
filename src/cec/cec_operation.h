/* @@@LICENSE
*
* Copyright (c) 2022 LG Electronics, Inc.
*
* Licensed under the Apache License, Version 2.0 (the "License");
* you may not use this file except in compliance with the License.
* You may obtain a copy of the License at
*
* http://www.apache.org/licenses/LICENSE-2.0
*
* Unless required by applicable law or agreed to in writing, software
* distributed under the License is distributed on an "AS IS" BASIS,
* WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
* See the License for the specific language governing permissions and
* limitations under the License.
*
* LICENSE@@@ */

/*
*******************************************************************/

#ifndef _CEC_OPERATION_H_
#define _CEC_OPERATION_H_

#include <unistd.h>
#include <sys/file.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <sys/wait.h>

#include <iostream>
#include <string>
#include <functional>
#include <vector>
#include <map>
#include <thread>
#include <mutex>
#include <condition_variable>

#define EXE_NAME "/usr/bin/cec-client"

class CecHandler
{
private:
    CecHandler();
    void parseConfigData(std::vector<std::string>);
    std::thread mProcThread;
    std::vector<std::string> resp;
    bool runFlag;
    bool reqProcFlag;
    bool respReadyFlag;
    std::string deviceNum;
    std::string physicalAddress;
    std::string version;
    std::string comPort;
    std::string vendorId;
    std::string osdName;
    std::string powerStatus;
    std::string lang;
    std::string type;
    std::string cmdName;
    std::string deviceType;
public:
    ~CecHandler();
    static CecHandler& getInst();
    void cmdDispatcher();
    void init();
    void deinit();
    void responseHandler();
    std::vector<std::string> getResponse();
    void addRequest(std::string, std::string);
    std::vector<std::string> executeSingleCommand(std::string);
    std::vector<std::string> listAdapters();
    std::string getAddress();
    std::string getVersion();
    std::string getVendorId();
    std::string getOsdName();
    std::string getPowerStatus();
    std::string getLang();
    std::string getLogicalAddress();
    std::string getDeviceType();
};
#endif
