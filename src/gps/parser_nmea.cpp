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

#include "parser_nmea.h"

#include <cstring>
#include <sys/time.h>
#include <thread>
#include <time.h>

#include <nyx/module/nyx_log.h>
#include "parser_thread_pool.h"
#include "parser_interface.h"
#include "parser_mock.h"
#include "parser_hw.h"

int64_t getCurrentTime() {
    struct timeval tval;
    gettimeofday(&tval, (struct timezone *) NULL);
    return (tval.tv_sec * 1000LL + tval.tv_usec/1000);
}

void ParserNmea::sendLocationUpdates() {
    GpsLocation location;
    memset(&location, 0, sizeof(GpsLocation));

    location.latitude = mGpsData.latitude;
    location.longitude = mGpsData.longitude;
    location.altitude = mGpsData.altitude;
    location.speed = mGpsData.speed;
    location.accuracy = mGpsData.horizAccuracy;
    location.timestamp = getCurrentTime();

    parser_loc_cb(&location, nullptr);
}

void ParserNmea::sendNmeaUpdates(char * rawNmea) {
    if (rawNmea)
        parser_nmea_cb(getCurrentTime(), rawNmea, (int)strlen(rawNmea));
}

bool ParserNmea::SetGpsGGA_Data(CNMEAParserData::GGA_DATA_T* ggaData, char *nmea_data) {

    nyx_debug("GPGGA Parsed!\n");
    nyx_debug("   Time:                %02d:%02d:%02d\n", ggaData->m_nHour, ggaData->m_nMinute, ggaData->m_nSecond);
    nyx_debug("   Latitude:            %f\n", ggaData->m_dLatitude);
    nyx_debug("   Longitude:           %f\n", ggaData->m_dLongitude);
    nyx_debug("   Altitude:            %.01fM\n", ggaData->m_dAltitudeMSL);
    nyx_debug("   GPS Quality:         %d\n", ggaData->m_nGPSQuality);
    nyx_debug("   Satellites in view:  %d\n", ggaData->m_nSatsInView);
    nyx_debug("   HDOP:                %.02f\n", ggaData->m_dHDOP);
    nyx_debug("   Differential ID:     %d\n", ggaData->m_nDifferentialID);
    nyx_debug("   Differential age:    %f\n", ggaData->m_dDifferentialAge);
    nyx_debug("   Geoidal Separation:  %f\n", ggaData->m_dGeoidalSep);
    nyx_debug("   Vertical Speed:      %.02f\n", ggaData->m_dVertSpeed);

    mGpsData.latitude = ggaData->m_dLatitude;
    mGpsData.longitude = ggaData->m_dLongitude;
    mGpsData.altitude = ggaData->m_dAltitudeMSL;
    mGpsData.horizAccuracy = ggaData->m_dHDOP;

    sendLocationUpdates();
    sendNmeaUpdates(nmea_data);

    free(ggaData);
    free(nmea_data);

    return CNMEAParserData::ERROR_OK;
}

bool ParserNmea::SetGpsGSV_Data(CNMEAParserData::GSV_DATA_T* gsvData, char *nmea_data) {
    GpsSvStatus sv_status;
    memset(&sv_status, 0, sizeof(GpsSvStatus));

    sv_status.num_svs = gsvData->nSatsInView;
    for(auto i = 0; i < sv_status.num_svs; i++)
    {
        sv_status.sv_list[i].prn = gsvData->SatInfo[i].nPRN;
        sv_status.sv_list[i].snr = gsvData->SatInfo[i].nSNR;
        sv_status.sv_list[i].elevation = gsvData->SatInfo[i].dElevation;
        sv_status.sv_list[i].azimuth = gsvData->SatInfo[i].dAzimuth;

        nyx_debug("    GPS No of Satellites: %d\n", sv_status.num_svs);
        nyx_debug("    GPS PRN: %d\n", gsvData->SatInfo[i].nPRN);
        nyx_debug("    GPS SNR: %d\n", gsvData->SatInfo[i].nSNR);
        nyx_debug("    GPS Elevation: %f\n", gsvData->SatInfo[i].dElevation);
        nyx_debug("    GPS azimuth: %f\n", gsvData->SatInfo[i].dAzimuth);
    }

    parser_sv_cb(&sv_status, nullptr);

    sendNmeaUpdates(nmea_data);
    free(gsvData);
    free(nmea_data);

    return CNMEAParserData::ERROR_OK;
}

bool ParserNmea::SetGpsGSA_Data(CNMEAParserData::GSA_DATA_T* gsaData, char *nmea_data) {
    nyx_debug("    nAutoMode: %d\n", gsaData->nAutoMode);
    nyx_debug("    nMode: %d\n", gsaData->nMode);
    nyx_debug("    GPS dPDOP: %f\n", gsaData->dPDOP);
    nyx_debug("    GPS dHDOP: %f\n", gsaData->dHDOP);
    nyx_debug("    GPS dVDOP: %f\n", gsaData->dVDOP);
    nyx_debug("    GPS uGGACount: %u\n", gsaData->uGGACount);

    sendNmeaUpdates(nmea_data);
    free(gsaData);
    free(nmea_data);

    return CNMEAParserData::ERROR_OK;
}

bool ParserNmea::SetGpsRMC_Data(CNMEAParserData::RMC_DATA_T* rmcData, char *nmea_data) {
    nyx_debug("GPRMC Parsed!\n");
    nyx_debug("   m_timeGGA:            %ld\n", rmcData->m_timeGGA);
    nyx_debug("   Time:                %02d:%02d:%02d\n", rmcData->m_nHour, rmcData->m_nMinute, rmcData->m_nSecond);
    nyx_debug("   Seconds:            %f\n", rmcData->m_dSecond);
    nyx_debug("   Latitude:            %f\n", rmcData->m_dLatitude);
    nyx_debug("   Longitude:           %f\n", rmcData->m_dLongitude);
    nyx_debug("   Altitude:            %.01fM\n", rmcData->m_dAltitudeMSL);
    nyx_debug("   Speed:           %f\n", rmcData->m_dSpeedKnots);
    nyx_debug("   TrackAngle:           %f\n", rmcData->m_dTrackAngle);

    nyx_debug("   m_nMonth:         %d\n", rmcData->m_nMonth);
    nyx_debug("   m_nDay:  %d\n", rmcData->m_nDay);
    nyx_debug("   m_nYear :     %d\n", rmcData->m_nYear);
    nyx_debug("   m_dMagneticVariation:    %f\n", rmcData->m_dMagneticVariation);

/*    struct tm timeStamp = {rmcData.m_nSecond, rmcData.m_nMinute, rmcData.m_nHour,
                                                rmcData.m_nDay, rmcData.m_nMonth, (rmcData.m_nYear-1900)};
    nyx_debug("   timeStamp:    %ld\n", ((mktime(&timeStamp)+(long)(rmcData.m_dSecond*1000))*1000));
*/
    mGpsData.latitude = rmcData->m_dLatitude;
    mGpsData.longitude = rmcData->m_dLongitude;
    mGpsData.speed = rmcData->m_dSpeedKnots*0.514;
    mGpsData.direction = rmcData->m_dTrackAngle;

    sendLocationUpdates();
    sendNmeaUpdates(nmea_data);
    free(rmcData);
    free(nmea_data);

    return CNMEAParserData::ERROR_OK;
}

void SetGpsStatus(int status)
{
    GpsStatus gps_status;
    memset(&gps_status, 0, sizeof(GpsStatus));
    gps_status.status = status;
    parser_status_cb(&gps_status, nullptr);
}

ParserNmea::ParserNmea()
{
    memset(&mGpsData, 0, sizeof(mGpsData));

}

ParserNmea::~ParserNmea()
{

}


CNMEAParserData::ERROR_E ParserNmea::ProcessRxCommand(char *pCmd, char *pData, char *checksum) {
    // Call base class to process the command
    CNMEAParser::ProcessRxCommand(pCmd, pData);

    ParserThreadPool* parserThreadPoolObj = ParserHW::getInstance()->getThreadPoolObj();
    if (ParserMock::getInstance()->isParserRequested())
    {
      parserThreadPoolObj = ParserMock::getInstance()->getThreadPoolObj();
    }

    if(!parserThreadPoolObj)
        return CNMEAParserData::ERROR_OK;

    nyx_info("MSGID_NMEA_PARSER", 0, "Cmd: %s Data: %s, checksum:%.2s\n", pCmd, pData, checksum);
    bool nmeaDataParsed = false;
    int len = strlen(pCmd) + strlen(pData) + 7;
    char *nmea_data = (char *)malloc(len);
    if (!nmea_data)
        return CNMEAParserData::ERROR_FAIL;

    snprintf(nmea_data, len, "$%.5s,%s*%.2s", pCmd, pData, checksum);

    // Check if this is the GPGGA command. If it is, then set gps location
    if (strstr(pCmd, "GPGGA") != NULL) {
        CNMEAParserData::GGA_DATA_T* ggaData =  (CNMEAParserData::GGA_DATA_T*)malloc(sizeof(CNMEAParserData::GGA_DATA_T));

        if (!ggaData) {
            free(nmea_data);
            return CNMEAParserData::ERROR_FAIL;
        }

        if (GetGPGGA(*ggaData) == CNMEAParserData::ERROR_OK) {
            nmeaDataParsed = true;
            parserThreadPoolObj->enqueue([=](){
                SetGpsGGA_Data(ggaData, nmea_data);
            });
        }
    }
    else if (strstr(pCmd, "GPGSV") != NULL) { //GPS GSV Data
        CNMEAParserData::GSV_DATA_T* gsvData = (CNMEAParserData::GSV_DATA_T*)malloc(sizeof(CNMEAParserData::GSV_DATA_T));

        if (!gsvData) {
            free(nmea_data);
            return CNMEAParserData::ERROR_FAIL;
        }

        if (GetGPGSV(*gsvData) == CNMEAParserData::ERROR_OK) {
            nmeaDataParsed = true;
            parserThreadPoolObj->enqueue([=](){
                SetGpsGSV_Data(gsvData, nmea_data);
            });
           }
    }
    else if (strstr(pCmd, "GPGSA") != NULL) {
        CNMEAParserData::GSA_DATA_T* gsaData = (CNMEAParserData::GSA_DATA_T*)malloc(sizeof(CNMEAParserData::GSA_DATA_T));

        if (!gsaData) {
            free(nmea_data);
            return CNMEAParserData::ERROR_FAIL;
        }

        if (GetGPGSA(*gsaData) == CNMEAParserData::ERROR_OK) {
            nmeaDataParsed = true;
            parserThreadPoolObj->enqueue([=](){
                SetGpsGSA_Data(gsaData, nmea_data);
            });
         }
    }
    else if (strstr(pCmd, "GPRMC") != NULL) {
        CNMEAParserData::RMC_DATA_T* rmcData = (CNMEAParserData::RMC_DATA_T*)malloc(sizeof(CNMEAParserData::RMC_DATA_T));

        if (!rmcData) {
            free(nmea_data);
            return CNMEAParserData::ERROR_FAIL;
        }

        if (GetGPRMC(*rmcData) == CNMEAParserData::ERROR_OK) {
            nmeaDataParsed = true;
            parserThreadPoolObj->enqueue([=](){
                SetGpsRMC_Data(rmcData, nmea_data);
            });
         }
    }

    if ((nmeaDataParsed == false)  && nmea_data)
            free(nmea_data);

    return CNMEAParserData::ERROR_OK;
}

void ParserNmea::OnError(CNMEAParserData::ERROR_E nError, char *pCmd)
{
}

ParserNmea *ParserNmea::getInstance()
{
    static ParserNmea parserNmeaObj;
    return &parserNmeaObj;
}

void ParserNmea::init() {
    mGpsData.altitude = -1;
    mGpsData.speed = -1;
    mGpsData.direction = -1;
    mGpsData.horizAccuracy = -1;
}

void ParserNmea::deinit() {
    ResetData();
    memset(&mGpsData, 0, sizeof(mGpsData));
}

bool ParserNmea::initParsingModule() {

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    init();
    if (ParserMock::getInstance()->isMockEnabled())
    {
        return ParserMock::getInstance()->init();
    }
    return ParserHW::getInstance()->init();
}

bool ParserNmea::deinitParsingModule() {

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    deinit();
    if (ParserMock::getInstance()->isParserRequested())
    {
        return ParserMock::getInstance()->deinit();
    }
    return ParserHW::getInstance()->deinit();
}

bool ParserNmea::startParsing()
{
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    if (ParserMock::getInstance()->isParserRequested())
    {
        return ParserMock::getInstance()->startParsing();
    }
    return ParserHW::getInstance()->startParsing();
}

bool ParserNmea::stopParsing()
{
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    if (ParserMock::getInstance()->isParserRequested())
    {
      return ParserMock::getInstance()->stopParsing();
    }
    return ParserHW::getInstance()->stopParsing();
}
