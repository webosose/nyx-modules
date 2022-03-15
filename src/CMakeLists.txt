# Copyright (c) 2010-2022 LG Electronics, Inc.
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.
#
# SPDX-License-Identifier: Apache-2.0

webos_machine_dep()
webos_machine_impl_dep()
webos_core_os_dep()

include(FindPkgConfig)

pkg_check_modules(GLIB2 REQUIRED glib-2.0)
include_directories(${GLIB2_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GLIB2_CFLAGS_OTHER})

pkg_check_modules(NYXLIB REQUIRED nyx>=7.3)
include_directories(${NYXLIB_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${NYXLIB_CFLAGS_OTHER})

pkg_check_modules(LIBCRYPTO REQUIRED libcrypto)
include_directories(${LIBCRYPTO_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${LIBCRYPTO_CFLAGS_OTHER})

pkg_check_modules(GIO REQUIRED gio-2.0)
include_directories(${GIO_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${GIO_CFLAGS_OTHER})

if(NYXMOD_OW_BATTERY OR NYXMOD_OW_CHARGER)
    pkg_check_modules(UDEV REQUIRED libudev)
    include_directories(${UDEV_INCLUDE_DIRS})
    webos_add_compiler_flags(ALL ${UDEV_CFLAGS_OTHER})
endif()

webos_add_linker_options(ALL --no-undefined --as-needed)
webos_add_compiler_flags(ALL -Wall -g -std=c99)
webos_add_compiler_flags(ALL -D_GNU_SOURCE=1)
webos_add_compiler_flags(DEBUG -O0 -DDEBUG -D_DEBUG)
webos_add_compiler_flags(RELEASE -DNDEBUG)

if(NYXMOD_OW_BATTERY)
    add_subdirectory(battery)
endif()

if(NYXMOD_OW_CHARGER)
    add_subdirectory(charger)
endif()

if(NYXMOD_OW_DEVICEINFO)
    add_definitions(-DWEBOS_TARGET_MACHINE="${WEBOS_TARGET_MACHINE}")
    add_definitions(-DDEVICEINFO_PRODUCT_NAME="${WEBOS_TARGET_MACHINE}")
    add_subdirectory(device_info)
endif()

if(NYXMOD_OW_DISPLAY)
    add_subdirectory(display)
endif()

if(NYXMOD_OW_SECURITY)
    add_subdirectory(security)
endif()

if(NYXMOD_OW_SECURITY2)
    add_subdirectory(security2)
endif()

if(NYXMOD_OW_SYSTEM)
    add_subdirectory(system)
endif()

if(NYXMOD_OW_OSINFO)
#    set(ENCRYPTION_KEY_TYPE "" CACHE STRING "Encryption key type")
    add_subdirectory(os_info)
endif()

if(NYXMOD_OW_GPS)
    add_subdirectory(gps)
endif()

if(NYXMOD_OW_CEC)
    add_subdirectory(cec)
endif()
