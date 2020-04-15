// Copyright (c) 2016-2020 LG Electronics, Inc.
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

#ifndef __NYX__MODULES__MESSAGE__LOG_H__
#define __NYX__MODULES__MESSAGE__LOG_H__

/** Utils*/
#define MSGID_NYX_MOD_GET_STRING_ERR                                        "NYXUTIL_GET_STRING_ERR"
#define MSGID_NYX_MOD_GET_DOUBLE_ERR                                        "NYXUTIL_GET_DOUBLE_ERR"
#define MSGID_NYX_MOD_GET_STRTOD_ERR                                        "NYXUTIL_GET_STRTOD_ERR"
#define MSGID_NYX_MOD_SYSFS_ERR                                             "NYXUTIL_SYSFS_ERR"
#define MSGID_NYX_MOD_GET_DIR_ERR                                           "NYXUTIL_GET_DIR_ERR"

/** Battery*/
#define MSGID_NYX_MOD_UDEV_ERR                                              "NYXBAT_UDEV_ERR"
#define MSGID_NYX_MOD_UDEV_MONITOR_ERR                                      "NYXBAT_UDEV_MONITOR_ERR"
#define MSGID_NYX_MOD_UDEV_SUBSYSTEM_ERR                                    "NYXBAT_UDEV_SUBSYSTEM_ERR"
#define MSGID_NYX_MOD_UDEV_RECV_ERR                                         "NYXBAT_UDEV_RECV_ERR"
#define MSGID_NYX_MOD_BATT_OPEN_ALREADY_ERR                                 "NYXBAT_OPEN_ALREADY_ERR"
#define MSGID_NYX_MOD_BATT_OPEN_ERR                                         "NYXBAT_OPEN_ERR"
#define MSGID_NYX_MOD_BATT_OUT_OF_MEMORY                                    "NYXBAT_OUT_OF_MEM"

/** Charger*/
#define MSGID_NYX_MOD_CHARG_ERR                                             "NYXCHG_ERR"
#define MSGID_NYX_MOD_NETLINK_ERR                                           "NYXCHG_NETLINK_ERR"
#define MSGID_NYX_MOD_CHR_SUB_ERR                                           "NYXCHR_SUB_ERR"
#define MSGID_NYX_MOD_ENABLE_REV_ERR                                        "NYXCHG_ENABLE_REV_ERR"
#define MSGID_NYX_MOD_CHARG_OPEN_ERR                                        "NYXCHG_OPEN_ERR"
#define MSGID_NYX_MOD_CHARG_OUT_OF_MEMORY                                   "NYXCHG_OUT_OF_MEM"

/** Device info generic*/
#define MSGID_NYX_MOD_OPEN_NDUID_ERR                                        "NYXDEV_OPEN_NDUID_ERR"
#define MSGID_NYX_MOD_READ_NDUID_ERR                                        "NYXDEV_READ_NDUID_ERR"
#define MSGID_NYX_MOD_WRITE_NDUID_ERR                                       "NYXDEV_WRITE_NDUID_ERR"
#define MSGID_NYX_MOD_CHMOD_ERR                                             "NYXDEV_CHMOD_ERR"
#define MSGID_NYX_MOD_MALLOC_ERR1                                           "NYXDEV_OOM1_ERR"
#define MSGID_NYX_MOD_MALLOC_ERR2                                           "NYXDEV_OOM2_ERR"
#define MSGID_NYX_MOD_URANDOM_ERR                                           "NYXDEV_URANDOM_ERR"
#define MSGID_NYX_MOD_URANDOM_OPEN_ERR                                      "NYXDEV_URANDOM_OPEN_ERR"
#define MSGID_NYX_MOD_MEMINFO_OPEN_ERR                                      "NYXDEV_MEMINFO_OPEN_ERR"
#define MSGID_NYX_MOD_STORAGE_ERR                                           "NYXDEV_STORAGE_ERR"
#define MSGID_NYX_MOD_DEV_INFO_OPEN_ERR                                     "NYXDEV_INFO_OPEN_ERR"
#define MSGID_NYX_MOD_DEVICEID_OPEN_ERR                                     "NYXDEV_DEVICEID_OPEN_ERR"

/*Display lib open*/
#define MSGID_NYX_MOD_DISP_OPEN_ALREADY_ERR                                 "NYXDIS_OPEN_ALREADY_ERR"
#define MSGID_NYX_MOD_DISP_OPEN_ERR                                         "NYXDIS_OPEN_ERR"
#define MSGID_NYX_MOD_DISP_OUT_OF_MEMORY                                    "NYXDIS_OUT_OF_MEM"

/*Security lib open*/
#define MSGID_NYX_MOD_SECU_OPEN_ERR                                         "NYXSEC_OPEN_ERR"
#define MSGID_NYX_MOD_SECU_OUT_OF_MEMORY                                    "NYXSEC_OUT_OF_MEM"

/*System */
#define MSGID_NYX_MOD_SYSTEM_OUT_OF_MEMORY                                  "NYXSYS_OUT_OF_MEM"
#define MSGID_NYX_MOD_SYSTEM_OPEN_ERR                                       "NYXSYS_OPEN_ERR"


#endif // __NYX__MODULES__MESSAGE__LOG_H__
