// Copyright (c) 2022 LG Electronics, Inc.
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

/*
*******************************************************************
* @file cec.c
*
* @brief The CEC module implementation.
*******************************************************************
*/

#include<string>
#include "cec_operation.h"

#ifdef __cplusplus
extern "C"{
#endif

#include <stdlib.h>
#include <stdio.h>
#include <string.h>

#include <nyx/nyx_module.h>
#include <nyx/module/nyx_utils.h>
#include <nyx/module/nyx_log.h>

nyx_device_t          *nyx_dev = NULL;
nyx_cec_callbacks_t   *nyx_cec_cbs = NULL;

nyx_cec_command_t     *nyx_cec_command = NULL;

NYX_DECLARE_MODULE(NYX_DEVICE_CEC, "Cec");

nyx_error_t nyx_module_open(nyx_instance_t i, nyx_device_t **d)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (NULL == d)
    {
        return NYX_ERROR_INVALID_VALUE;
    }

    *d = NULL;

    if (nyx_dev)
    {
        return NYX_ERROR_TOO_MANY_OPENS;
    }

    nyx_dev = (nyx_device_t *)calloc(sizeof(nyx_device_t), 1);

    if (NULL == nyx_dev)
    {
        return NYX_ERROR_OUT_OF_MEMORY;
    }

    nyx_module_register_method(i, (nyx_device_t *) nyx_dev,
                               NYX_CEC_SET_CALLBACK_MODULE_METHOD,
                               "cec_set_callback");
    nyx_module_register_method(i, (nyx_device_t *) nyx_dev,
                               NYX_CEC_SEND_COMMAND_MODULE_METHOD,
                               "cec_send_command");
    nyx_module_register_method(i, (nyx_device_t *) nyx_dev,
                               NYX_CEC_GET_PHYSICAL_ADDRESS_MODULE_METHOD,
                               "cec_get_physical_address");
    nyx_module_register_method(i, (nyx_device_t *) nyx_dev,
                               NYX_CEC_SET_CONFIG_MODULE_METHOD,
                               "cec_set_config");
    nyx_module_register_method(i, (nyx_device_t *) nyx_dev,
                               NYX_CEC_GET_CONFIG_MODULE_METHOD,
                               "cec_get_config");
    nyx_module_register_method(i, (nyx_device_t *) nyx_dev,
                               NYX_CEC_GET_VERSION_MODULE_METHOD,
                               "cec_get_version");

    *d = (nyx_device_t *)nyx_dev;

    nyx_debug("Called CEC init");
    CecHandler::getInst().init();
    CecHandler::getInst().addRequest("internalScan", "scan");
    CecHandler::getInst().getResponse();
    return NYX_ERROR_NONE;
}

nyx_error_t nyx_module_close(nyx_device_handle_t d)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (NULL == d)
    {
        return NYX_ERROR_INVALID_HANDLE;
    }
    if (nyx_cec_command != NULL) {
        free(nyx_cec_command);
        nyx_cec_command = NULL;
    }
    if (NULL != nyx_dev){
        free(nyx_dev);
        nyx_dev = NULL;
    }

    nyx_debug("Called CEC deinit");
    CecHandler::getInst().addRequest("sendCommand", "q");
    CecHandler::getInst().deinit();

    return NYX_ERROR_NONE;
}

void updateDataCallback(std::vector<std::string> resp)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (!nyx_cec_cbs) return;
    nyx_cec_response_t obj;
    obj.size = resp.size();

    for (std::size_t i=0; i<resp.size(); ++i)
    {
        strcpy(obj.responses[i], resp[i].c_str());
    }
    (* (nyx_cec_cbs->response_cb))(&obj);
}

nyx_error_t cec_set_callback(nyx_device_handle_t handle,
                             nyx_cec_callbacks_t *cec_cbs)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (nyx_dev == NULL)
        return NYX_ERROR_DEVICE_NOT_EXIST;

    if (handle != nyx_dev || cec_cbs == NULL)
        return NYX_ERROR_INVALID_HANDLE;

    nyx_debug("Called CEC Set Callback");

    nyx_cec_cbs = cec_cbs;

    return NYX_ERROR_NONE;
}

nyx_error_t cec_send_command(nyx_device_handle_t handle, nyx_cec_command_t *command)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (nyx_dev == NULL)
        return NYX_ERROR_DEVICE_NOT_EXIST;

    if (handle != nyx_dev)
        return NYX_ERROR_INVALID_HANDLE;

    nyx_debug("Called CEC Send Command");
    std::string cmdName = command->name;
    std::vector<std::string> response;
    if(cmdName == "listAdapters"){
        response = CecHandler::getInst().listAdapters();
    }
    else{
        std::string cmd;
        std::string cmdType = "sendCommand";
        std::map<std::string, std::string> params;
        for (int i=0; i<command->size; ++i)
        {
            params[command->params[i].name] = command->params[i].value;
        }
        std::string address = params["destAddress"];

        if (cmdName == "scan")
            cmd = "scan";
        else if (cmdName == "internalScan")
        {
            cmd = "scan";
            cmdType = "internalScan";
        }
        else if (cmdName == "report-power-status" && params["pwr-state"] == "on")
            cmd = "on " + address;
        else if (cmdName == "report-power-status" && params["pwr-state"] == "standby")
            cmd = "standby " + address;
        else if (cmdName == "report-power-status" && params["pwr-state"] == "")
            cmd = "pow " + address;
        else if (cmdName == "report-audio-status" && params["aud-mute-status"] == "on")
            cmd = "mute";
        else if (cmdName == "report-audio-status" && params["aud-mute-status"] == "off")
            cmd = "volup";
        else if (cmdName == "set-volume" && params["volume"] == "up")
            cmd = "volup";
        else if (cmdName == "set-volume" && params["volume"] == "down")
            cmd = "voldown";
        else if (cmdName == "osd-display")
            cmd = "osd " + address + " " + params["osd"];
        else if (cmdName == "active")
            cmd = "as";
        else if (cmdName == "one-touch-play")
            return NYX_ERROR_NOT_IMPLEMENTED;
        else if (cmdName == "vendor-commands")
            cmd = "tx " + params["payload"];
        else if (cmdName == "get-power-state")
            cmd = "pow " + address;
        else if (cmdName == "system-information" && params.count("vendor-id"))
            cmd = "ven " + address;
        else if (cmdName == "system-information" && params.count("version"))
            cmd = "ver " + address;
        else if (cmdName == "system-information" && params.count("name"))
            cmd = "name " + address;
        else if (cmdName == "system-information" && params.count("language"))
            cmd = "lang " + address;
        else if (cmdName == "system-information" && params.count("is-active"))
            cmd = "ad " + address;
        else
            return NYX_ERROR_NOT_IMPLEMENTED;
        if (!cmd.empty())
        {
            CecHandler::getInst().addRequest(cmdType, cmd);
            response = CecHandler::getInst().getResponse();
        if(response.empty())
        response.push_back("response: success");
    }
    }
    if (!response.empty())
        updateDataCallback(response);
    return NYX_ERROR_NONE;
}

nyx_error_t cec_get_physical_address(nyx_device_handle_t handle, char **address)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (nyx_dev == NULL)
        return NYX_ERROR_DEVICE_NOT_EXIST;

    if (handle != nyx_dev )
        return NYX_ERROR_INVALID_HANDLE;

    nyx_debug("Called CEC physical_address");

    std::string addr = CecHandler::getInst().getAddress();
    strcpy(*address, addr.c_str());
    return NYX_ERROR_NONE;
}

nyx_error_t cec_set_config(nyx_device_handle_t handle, char *type, char *value)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    return NYX_ERROR_NOT_IMPLEMENTED;
}

nyx_error_t cec_get_config(nyx_device_handle_t handle, char *type, char **value)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (nyx_dev == NULL)
        return NYX_ERROR_DEVICE_NOT_EXIST;

    if (handle != nyx_dev)
        return NYX_ERROR_INVALID_HANDLE;

    nyx_debug("Called CEC get_config");
    std::string cmdType = type;
    std::string resp;
    if(cmdType == "version")
        resp = CecHandler::getInst().getVersion();
    else if(cmdType == "vendorId")
        resp = CecHandler::getInst().getVendorId();
    else if(cmdType == "osd")
        resp = CecHandler::getInst().getOsdName();
    else if(cmdType == "language")
        resp = CecHandler::getInst().getLang();
    else if(cmdType == "powerState")
        resp = CecHandler::getInst().getPowerStatus();
    else if(cmdType == "physicalAddress")
        resp = CecHandler::getInst().getAddress();
    else if(cmdType == "logicalAddress")
        resp = CecHandler::getInst().getLogicalAddress();
    else if(cmdType == "deviceType")
        resp = CecHandler::getInst().getDeviceType();
    else
        return NYX_ERROR_NOT_IMPLEMENTED;

    if (!resp.empty())
        strcpy(*value, resp.c_str());
    return NYX_ERROR_NONE;
}

nyx_error_t cec_get_version(nyx_device_handle_t handle, char **version)
{
    nyx_info("CEC_NYX_DEVICE", 0, "%s", __FUNCTION__);
    if (nyx_dev == NULL)
        return NYX_ERROR_DEVICE_NOT_EXIST;

    if (handle != nyx_dev)
        return NYX_ERROR_INVALID_HANDLE;

    nyx_debug("Called CEC get_config");

    std::string ver = CecHandler::getInst().getVersion();
    strcpy(*version, ver.c_str());

    return NYX_ERROR_NONE;
}
#ifdef __cplusplus
}
#endif
