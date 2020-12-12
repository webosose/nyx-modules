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

#ifndef _PARSER_MOCK_H_
#define _PARSER_MOCK_H_

#include <nmeaparser/NMEAParser.h>
#include "parser_nmea.h"

class ParserInotify;
class ParserThreadPool;
class ParserMock : public ParserNmea
{
public:
    void parserWatchCb(const char *ident);
    static ParserMock *getInstance();
    bool init();
    bool deinit();
    bool startParsing();
    bool stopParsing();
    bool isSourcePresent();
    bool isMockEnabled();
    bool isParserRequested() const { return mParserRequested; }
    ParserThreadPool* getThreadPoolObj() const { return mParserThreadPoolObj; }
private:
    ParserMock();
    ~ParserMock();
    int getMockLatency();
    bool createThreadPool();
    FILE *mNmeaFp;
    uint64_t mSeekOffset;
    ParserThreadPool* mParserThreadPoolObj;
    bool mStopParser;
    bool mParserRequested;
    ParserInotify *mParserInotifyObj;
};

#endif // end _PARSER_MOCK_H_
