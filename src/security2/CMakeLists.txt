# Copyright (c) 2016-2018 LG Electronics, Inc.
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

pkg_check_modules(SSL REQUIRED openssl)
include_directories(${SSL_INCLUDE_DIRS})
webos_add_compiler_flags(ALL ${SSL_CFLAGS_OTHER})

webos_build_nyx_module(Security2Main
                       SOURCES 3des.c aes.c hmac.c rsa.c security2.c
                       LIBRARIES ${GLIB2_LDFLAGS} ${PMLOG_LDFLAGS} ${NYXLIB_LDFLAGS} ${SSL_LDFLAGS} -lrt -lpthread)

add_subdirectory(test)
