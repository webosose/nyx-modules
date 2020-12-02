# @@@LICENSE
#
#      Copyright (c) 2020 LG Electronics, Inc.
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
# LICENSE@@@

pkg_check_modules(NMEAPARSER REQUIRED nmeaparser)
include_directories(${NMEAPARSER_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${NMEAPARSER_CFLAGS_OTHER})

webos_build_nyx_module(GpsMain
                       SOURCES gps.c parser_interface.cpp parser_nmea.cpp gps_device.cpp parser_mock.cpp parser_hw.cpp
                       LIBRARIES ${MODULE_LIBRARIES} ${PMLOG_LDFLAGS} ${NYXLIB_LDFLAGS} ${NMEAPARSER_LDFLAGS} ${GLIB2_LDFLAGS} -lrt -lpthread -lNMEAParserLib)
