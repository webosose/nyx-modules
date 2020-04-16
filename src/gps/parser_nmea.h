/* @@@LICENSE
 * *
 * * Copyright (c) 2020 LG Electronics, Inc.
 * *
 * * Licensed under the Apache License, Version 2.0 (the "License");
 * * you may not use this file except in compliance with the License.
 * * You may obtain a copy of the License at
 * *
 * * http://www.apache.org/licenses/LICENSE-2.0
 * *
 * * Unless required by applicable law or agreed to in writing, software
 * * distributed under the License is distributed on an "AS IS" BASIS,
 * * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * * See the License for the specific language governing permissions and
 * * limitations under the License.
 * *
 * * LICENSE@@@ */

/*
 * *******************************************************************/

#ifndef _PARSER_NMEA_H_
#define _PARSER_NMEA_H_

#include <string>
#include <nyx/module/nyx_log.h>
#include "parser_interface.h"

class ParserNmea : public ParserInterface {
public:
    ParserNmea(const std::string& nmea_file_path);
    ~ParserNmea();

    bool startParsing();
    bool stopParsing();
};

#endif // end _PARSER_NMEA_H_
