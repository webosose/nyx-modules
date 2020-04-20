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

#include "parser_nmea.h"
#include <nyx/module/nyx_log.h>

#include <cstring>
#include <thread>

#include "parser_interface.h"


const std::string nmea_file_path ="/media/internals/location/gps.nmea";

///
/// \brief This method is redefined from CNMEAParserPacket::ProcessRxCommand(char *pCmd, char *pData)
///
/// Here we are capturing the ProcessRxCommand to set gps location and gps sv and gps capabilities.
///
/// \param pCmd Pointer to the NMEA command string
/// \param pData Comma separated data that belongs to the command
/// \return Returns CNMEAParserData::ERROR_OK If successful
///

bool SetGpsGGA_Data(CNMEAParserData::GGA_DATA_T ggaData)
    {
        GpsLocation location;
        memset(&location, 0, sizeof(GpsLocation));

        location.latitude = ggaData.m_dLatitude;
        location.longitude = ggaData.m_dLongitude;
        location.altitude = ggaData.m_dAltitudeMSL;
        location.speed = ggaData.m_dVertSpeed;
        location.bearing = ggaData.m_dHDOP;

        nmea_loc_cb(&location, nullptr);

        printf("GPGGA Parsed!\n");
        printf("   Time:                %02d:%02d:%02d\n", ggaData.m_nHour, ggaData.m_nMinute, ggaData.m_nSecond);
        printf("   Latitude:            %f\n", ggaData.m_dLatitude);
        printf("   Longitude:           %f\n", ggaData.m_dLongitude);
        printf("   Altitude:            %.01fM\n", ggaData.m_dAltitudeMSL);
        printf("   GPS Quality:         %d\n", ggaData.m_nGPSQuality);
        printf("   Satellites in view:  %d\n", ggaData.m_nSatsInView);
        printf("   HDOP:                %.02f\n", ggaData.m_dHDOP);
        printf("   Differential ID:     %d\n", ggaData.m_nDifferentialID);
        printf("   Differential age:    %f\n", ggaData.m_dDifferentialAge);
        printf("   Geoidal Separation:  %f\n", ggaData.m_dGeoidalSep);
        printf("   Vertical Speed:      %.02f\n", ggaData.m_dVertSpeed);

        return CNMEAParserData::ERROR_OK;
    }

    bool SetGpsGSV_Data(CNMEAParserData::GSV_DATA_T gsvData)
    {
        GpsSvStatus sv_status;
        sv_status.num_svs = gsvData.nSatsInView;
        for(auto i = 0; i < sv_status.num_svs; i++)
        {
            sv_status.sv_list[i].prn = gsvData.SatInfo[i].nPRN;
            sv_status.sv_list[i].snr = gsvData.SatInfo[i].nSNR;
            sv_status.sv_list[i].elevation = gsvData.SatInfo[i].dElevation;
            sv_status.sv_list[i].azimuth = gsvData.SatInfo[i].dAzimuth;

            printf("    GPS No of Satellites: %d\n", sv_status.num_svs);
            printf("    GPS PRN: %d\n", gsvData.SatInfo[i].nPRN);
            printf("    GPS SNR: %d\n", gsvData.SatInfo[i].nSNR);
            printf("    GPS Elevation: %f\n", gsvData.SatInfo[i].dElevation);
            printf("    GPS azimuth: %f\n", gsvData.SatInfo[i].dAzimuth);
        }

        nmea_sv_cb(&sv_status, nullptr);
        return CNMEAParserData::ERROR_OK;
    }

    bool SetGpsGSA_Data(CNMEAParserData::GSA_DATA_T gsaData)
    {
        return CNMEAParserData::ERROR_OK;
    }

    bool SetGpsRMC_Data(CNMEAParserData::RMC_DATA_T rmcData)
    {
        return CNMEAParserData::ERROR_OK;
    }

CNMEAParserData::ERROR_E ParserNmea::ProcessRxCommand(char *pCmd, char *pData) {

    // Call base class to process the command
    CNMEAParser::ProcessRxCommand(pCmd, pData);

    printf("Cmd: %s\nData: %s\n", pCmd, pData);

    // Check if this is the GPGGA command. If it is, then set gps location
    if (strstr(pCmd, "GPGGA") != NULL) {
        CNMEAParserData::GGA_DATA_T ggaData;
        if (GetGPGGA(ggaData) == CNMEAParserData::ERROR_OK) {
            SetGpsGGA_Data(ggaData);
        }
    }
    else if (strstr(pCmd, "GPGSV") != NULL) { //GPS GSV Data
        CNMEAParserData::GSV_DATA_T gsvData;
        if (GetGPGSV(gsvData) == CNMEAParserData::ERROR_OK) {
            SetGpsGSV_Data(gsvData);
        }
    }
    else if (strstr(pCmd, "GPGSA") != NULL) {
        CNMEAParserData::GSA_DATA_T gsaData;
        if (GetGPGSA(gsaData) == CNMEAParserData::ERROR_OK) {
            SetGpsGSA_Data(gsaData);
        }
    }
    else if (strstr(pCmd, "GPRMC") != NULL) {
        CNMEAParserData::RMC_DATA_T rmcData;
        if(GetGPRMC(rmcData) == CNMEAParserData::ERROR_OK) {
            SetGpsRMC_Data(rmcData);
        }
    }

    return CNMEAParserData::ERROR_OK;
}

///
/// \brief This method is called whenever there is a parsing error.
///
/// Redefine this method to capture errors.
///
/// \param pCmd Pointer to NMEA command that caused the error. Please note that this parameter may be NULL of not completely defined. Use with caution.
///
void ParserNmea::OnError(CNMEAParserData::ERROR_E nError, char *pCmd) {
    printf("ERROR for Cmd: %s, Number: %d\n", pCmd, nError);
}

ParserNmea::ParserNmea() :
    fp(nullptr),
    stopParser(false) {
    nyx_info("MSGID_NYX_MOD_GPS_NMEA_PARSER", 0, "Fun: %s, Line: %d", __FUNCTION__, __LINE__);
}

ParserNmea::~ParserNmea() {
    nyx_info("MSGID_NYX_MOD_GPS_NMEA_PARSER", 0, "Fun: %s, Line: %d", __FUNCTION__, __LINE__);
    if (fp) {
        fclose(fp);
    }
}

ParserNmea* ParserNmea::getInstance() {
    static ParserNmea parserNmeaObj;
    return &parserNmeaObj;
}

bool ParserNmea::startParsing() {

    //
    // Read the file and process
    //
    fp = fopen(nmea_file_path.c_str(), "r");
    if (fp == nullptr) {
        printf("Could not open file: %s\n", nmea_file_path.c_str());
        return false;
    }

    char pBuff[1024];
    while (feof(fp) == 0) {

        if (stopParser)
        {
            stopParser = false;
            if (fp) {
                fclose(fp);
                fp = nullptr;
            }
            return true;
        }

        size_t nBytesRead = fread(pBuff, 1, 512, fp);

        CNMEAParserData::ERROR_E nErr;
        if ((nErr = ProcessNMEABuffer(pBuff, nBytesRead)) != CNMEAParserData::ERROR_OK) {
            nyx_error("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d error: %d \n", __FUNCTION__, __LINE__, nErr);
            return false;
        }
    }

    if (fp) {
        fclose(fp);
        fp = nullptr;
    }

    return false;
}

bool ParserNmea::stopParsing() {
    if (fp) {
        stopParser = true;
        fclose(fp);
        fp = nullptr;
    }

    return true;
}
