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

#include <nmeaparser/NMEAParser.h>

class ParserThreadPool;

class ParserNmea : public CNMEAParser {
public:

    static ParserNmea* getInstance();

    bool startParsing();
    bool stopParsing();

private:
    FILE *fp;
    bool stopParser;

    ParserThreadPool* parserThreadPoolObj;

    ParserNmea();
    ~ParserNmea();

    virtual CNMEAParserData::ERROR_E ProcessRxCommand(char *pCmd, char *pData);
    virtual void OnError(CNMEAParserData::ERROR_E nError, char *pCmd);

    bool SetGpsRMC_Data(CNMEAParserData::RMC_DATA_T& rmcData);
    bool SetGpsGSA_Data(CNMEAParserData::GSA_DATA_T& gsaData);
    bool SetGpsGSV_Data(CNMEAParserData::GSV_DATA_T& gsvData);
    bool SetGpsGGA_Data(CNMEAParserData::GGA_DATA_T& ggaData);
};

#endif // _PARSER_NMEA_H_
