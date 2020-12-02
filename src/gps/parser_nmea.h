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
 * * SPDX-License-Identifier: Apache-2.0
 * *
 * * Author(s)    : Ashish Patel
 * * Email ID.    : ashish23.patel@lge.com
 * * LICENSE@@@ */

/*
 * *******************************************************************/

#ifndef _PARSER_NMEA_H_
#define _PARSER_NMEA_H_

#include <nmeaparser/NMEAParser.h>

typedef struct {
    //for getLocationUpdates
    int64_t timestamp; // in milli seconds
    double latitude; // -180.0 ~ 180.0
    double longitude; // -180.0 ~ 180.0
    double altitude;
    double direction; //0.0 ~ 360.0
    double speed; // in meters/seconds
    double horizAccuracy;
    double vertAccuracy;
} gps_data;

class ParserNmea : public CNMEAParser {
public:
    static ParserNmea *getInstance();
    bool initParsingModule();
    bool deinitParsingModule();
    bool startParsing();
    bool stopParsing();
    ParserNmea();
    ~ParserNmea();


private:

    gps_data mGpsData;
    virtual CNMEAParserData::ERROR_E ProcessRxCommand(char *pCmd, char *pData, char *checksum);
    virtual void OnError(CNMEAParserData::ERROR_E nError, char *pCmd);
    void init();
    void deinit();
    void sendLocationUpdates();
    void sendNmeaUpdates(char *rawNmea);
    bool SetGpsRMC_Data(CNMEAParserData::RMC_DATA_T *rmcData, char *nmea_data);
    bool SetGpsGSA_Data(CNMEAParserData::GSA_DATA_T *gsaData, char *nmea_data);
    bool SetGpsGSV_Data(CNMEAParserData::GSV_DATA_T *gsvData, char *nmea_data);
    bool SetGpsGGA_Data(CNMEAParserData::GGA_DATA_T *ggaData, char *nmea_data);
};
void SetGpsStatus(int status);

#endif // end _PARSER_NMEA_H_
