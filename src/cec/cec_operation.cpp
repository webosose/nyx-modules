/* @@@LICENSE
*
* Copyright (c) 2023 LG Electronics, Inc.
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

#include "cec_operation.h"
#include <sstream>
#include <stdio.h>

int fd1[2], fd2[2];
std::string trim(std::string str)
{
    std::string res;
    for (std::size_t i=0; i<str.size(); ++i)
    {
        if (i>0 && str[i] == ' ' && str[i-1] == ' ') continue;
        if (i>0 && str[i] == ' ' && str[i-1] == ':') continue;
        if ((str[i] == '\t') || (str[i] == '\n')) continue;
        res += str[i];
    }
    return res;
}

CecHandler::CecHandler() : runFlag(true), reqProcFlag(false), respReadyFlag(false)
{
}

CecHandler::~CecHandler()
{
    deinit();
}

CecHandler& CecHandler::getInst()
{
    static CecHandler cecInstance;
    return cecInstance;
}

void CecHandler::init()
{
    runFlag = true;
    parseConfigData(executeSingleCommand("cec-client -l"));
    mProcThread = std::thread(std::bind(&CecHandler::cmdDispatcher, this));
    //Waiting for 5 secs to initialise the child process
    sleep(5);
    mProcThread.detach();
}

void CecHandler::deinit()
{
    runFlag = false;
    if (mProcThread.joinable())
    {
        mProcThread.join();
    }
}

std::vector<std::string> CecHandler::executeSingleCommand(std::string cmd)
{
    std::vector<std::string> cmdResp;
    if (cmd.empty()) {
        return cmdResp;
    }
    // Execute command and parse response
    FILE *fp = popen(cmd.c_str(), "r");
    if(fp == NULL){
        return cmdResp;
    }
    char buffer[100]={'\0'};
    std::string inp;
    while(fgets(buffer, sizeof(buffer), fp))
    {
        inp = trim(buffer);
        if (!inp.size()) continue;
        // Update local variables
        cmdResp.push_back(inp);
    }
    pclose(fp);
    return cmdResp;
}

void CecHandler::parseConfigData(std::vector<std::string> inp)
{
    for (auto &str : inp)
    {
        if (str.find("device:") != std::string::npos)
            deviceNum = str.substr(str.find(":") + 1);
        else if (str.find("com port:") != std::string::npos)
            comPort = str.substr(str.find(":") + 1);
        else if (str.find("vendor id:") != std::string::npos)
            vendorId = str.substr(str.find(":") + 1);
        else if (str.find("type:") != std::string::npos)
            type = str.substr(str.find(":") + 1);
    }
}

void CecHandler::cmdDispatcher()
{
    pid_t pid;
    if (pipe(fd1) < 0 || pipe(fd2) < 0) return;
    signal(SIGHUP, SIG_IGN);
    if ((pid = fork()) > 0)
    {
        std::thread t1(std::bind(&CecHandler::responseHandler, this));
        wait(NULL);
        t1.join();
    }
    else
    {
        // Child
        dup2(fd1[0], 0);
        dup2(fd2[1], 1);
        close(fd1[1]);
        close(fd2[0]);
        execl(EXE_NAME, EXE_NAME, "-d", "1", "-o", "webOS-CEC", NULL);
    }
}

std::vector<std::string> CecHandler::getResponse()
{
    int maxRetry = 25;
    if (cmdName == "scan" || cmdName == "internalScan")
         maxRetry = 50;
    while (!respReadyFlag)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(200));
        maxRetry--;
        if (maxRetry == 0) break;
    }
    respReadyFlag = false;
    reqProcFlag = false;
    std::vector<std::string> response = resp;
    resp.clear();
    if (cmdName == "scan" || cmdName == "internalScan")
    {
        for (std::size_t i=0; i<response.size(); ++i)
        {
            if (response[i].find("device #" + deviceNum + ":") != std::string::npos)
            {
                deviceType = response[i].substr(response[i].find(":") + 1);
                std::istringstream dType(deviceType);
                dType >> deviceType;
                physicalAddress = response[i+1].substr(response[i+1].find(":") + 1);
                osdName = response[i+4].substr(response[i+4].find(":") + 1);
                version = response[i+5].substr(response[i+5].find(":") + 1);
                powerStatus = response[i+6].substr(response[i+6].find(":") + 1);
                lang = response[i+7].substr(response[i+7].find(":") + 1);
                break;
            }
        }
    }

    return response;
}

void CecHandler::responseHandler()
{
    dup2(fd2[0], 0);
    close(fd2[1]);
    std::string str;
    while(std::getline(std::cin, str))
    {
       if (reqProcFlag)
          resp.push_back(trim(str));
       if (cmdName == "scan" || cmdName == "internalScan")
       {
           if (str.find("currently active source:") != std::string::npos) respReadyFlag = true;
       }
       else respReadyFlag = true;
       if(!runFlag)
          break;
    }
}

void CecHandler::addRequest(std::string name, std::string cmd)
{
    reqProcFlag = true;
    respReadyFlag = false;
    cmdName = cmd;
    dup2(fd1[1], 1);
    close(fd1[0]);
    std::cout << cmd << std::endl;
}

std::vector<std::string> CecHandler::listAdapters()
{
    std::vector<std::string> res;
    res.push_back("com port: " + comPort);
    return res;
}

std::string CecHandler::getAddress()
{
    std::string resp = "address: " + physicalAddress;
    return resp;
}

std::string CecHandler::getVersion()
{
    std::string resp = "CEC version: " + version;
    return resp;
}

std::string CecHandler::getVendorId()
{
    std::string resp = "vendor id: " + vendorId;
    return resp;
}

std::string CecHandler::getOsdName()
{
    std::string resp = "osd string: " + osdName;
    return resp;
}

std::string CecHandler::getPowerStatus()
{
    std::string resp = "power status: " + powerStatus;
    return resp;
}

std::string CecHandler::getLang()
{
    std::string resp = "language: " + lang;
    return resp;
}

std::string CecHandler::getLogicalAddress()
{
    std::string resp = "logical address: " + deviceNum;
    return resp;
}

std::string CecHandler::getDeviceType()
{
    std::string resp = "type: " + deviceType;
    return resp;
}

