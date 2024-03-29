# Copyright (c) 2013-2018 LG Electronics, Inc.
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

# Set default values (for standalone build). Using CACHE allows each
# variable to be overridden from the command line during, say, an OE build.
set(DISTRO_VERSION "(unknown)" CACHE STRING "Distribution version number")
set(DISTRO_NAME "(unknown)" CACHE STRING "Distribution name")
set(WEBOS_DISTRO_API_VERSION "(unknown)" CACHE STRING "Distribution API version")
set(WEBOS_DISTRO_RELEASE_CODENAME "(unknown)" CACHE STRING "Distribution release codename")
set(WEBOS_DISTRO_BUILD_ID unofficial CACHE STRING "Distribution build ID (build number with optional prefix)")
set(WEBOS_DISTRO_MANUFACTURING_VERSION "(unknown)" CACHE STRING "Distribution manufacturing version")
set(WEBOS_DISTRO_PRERELEASE "" CACHE STRING "Distribution prerelease")
set(WEBOS_DISTRO_RELEASE_PLATFORMCODE "(unknown)" CACHE STRING "Distribution platform version code")

# date sends its output to stderr for some reason, hence the redirection.
execute_process(COMMAND bash -c "date -u +'%Y%m%d%H%M%S' 2>&1" OUTPUT_VARIABLE BUILD_TIME)
# Remove any trailing newlines from the output
STRING(REGEX REPLACE "(\r?\n)+$" "" BUILD_TIME "${BUILD_TIME}")

set(WEBOS_BUILD_DATETIME ${BUILD_TIME} CACHE STRING "DATETIME stamp for the build")

webos_configure_source_files(sourcelist os_info.c)
webos_build_nyx_module(OSInfoMain
		       SOURCES ${sourcelist}
                       LIBRARIES ${GLIB2_LDFLAGS} ${PMLOG_LDFLAGS} ${NYXLIB_LDFLAGS} -lrt -lpthread)
