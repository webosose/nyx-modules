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

#include <glib-2.0/glib.h>
#include <sys/inotify.h>
#include <string>
#include <unistd.h>
#include <stdlib.h>

#include <nyx/module/nyx_log.h>

#include "parser_mock.h"

#ifndef _PARSER_INOTIFY_H_
#define _PARSER_INOTIFY_H_

class ParserInotify
{
public:
    ParserInotify(std::string dirPath, ParserMock *obj);
    ~ParserInotify();

    bool startWatch();
    void stopWatch();

private:
    std::string mDirPath;
    int mWatchDescriptor;
    int mFileDescriptor;
    GIOChannel *mChannel;
    uint mWatch;

    ParserMock *mParserMockObj;

    static gboolean watch_cb(GIOChannel *source, GIOCondition condition, gpointer data);
};

ParserInotify::ParserInotify(std::string dirPath, ParserMock* obj)
    : mDirPath(dirPath)
    , mWatchDescriptor(-1)
    , mFileDescriptor(-1)
    , mChannel(nullptr)
    , mWatch(0)
    , mParserMockObj(obj) {
}

ParserInotify::~ParserInotify() {
    if (mFileDescriptor >= 0)
        close(mFileDescriptor);
}

gboolean ParserInotify::watch_cb(GIOChannel *source, GIOCondition condition, gpointer data) {
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    if (condition & (G_IO_NVAL | G_IO_ERR | G_IO_HUP))
        return FALSE;

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    ParserInotify* parserInotifyObj = static_cast<ParserInotify*>(data);

    char buf [sizeof(struct inotify_event) + NAME_MAX + 1];
    gsize bytes_read;

    GIOStatus status = g_io_channel_read_chars(source, buf, sizeof(buf), &bytes_read, NULL);
    if (status != G_IO_STATUS_NORMAL)
        return FALSE;

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    guint currentLen = 0;
    while (currentLen < bytes_read) {
        struct inotify_event* event = (struct inotify_event*) &buf[currentLen];
        char* fileName = nullptr;

        if (event->len)
            fileName = event->name;

        currentLen += sizeof(struct inotify_event) + event->len;

        nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
        if ((event->mask & IN_MODIFY) && fileName) {
            nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
            parserInotifyObj->mParserMockObj->parserWatchCb(fileName);
        }
    }
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    return TRUE;
}

bool ParserInotify::startWatch() {
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    mFileDescriptor = inotify_init();

    if (mFileDescriptor < 0)
        return false;

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    mWatchDescriptor = inotify_add_watch(mFileDescriptor, mDirPath.c_str(), IN_MODIFY);

    if (mWatchDescriptor < 0) {
        close(mFileDescriptor);
        mFileDescriptor = -1;
        return false;
    }
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    mChannel = g_io_channel_unix_new(mFileDescriptor);
    if (!mChannel) {
        inotify_rm_watch(mFileDescriptor, mWatchDescriptor);
        mWatchDescriptor = -1;
        close(mFileDescriptor);
        mFileDescriptor = -1;
        return false;
    }
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    g_io_channel_set_close_on_unref(mChannel, TRUE);
    g_io_channel_set_encoding(mChannel, NULL, NULL);
    g_io_channel_set_buffered(mChannel, FALSE);

    mWatch = g_io_add_watch(mChannel, (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_NVAL | G_IO_ERR), watch_cb, this);

    return 0;
}

void ParserInotify::stopWatch() {
    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    if (mWatch > 0)
        g_source_remove(mWatch);

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);
    if (mWatchDescriptor >= 0)
        inotify_rm_watch(mFileDescriptor, mWatchDescriptor);

    nyx_info("MSGID_NMEA_PARSER", 0, "Fun: %s, Line: %d \n", __FUNCTION__, __LINE__);

    if (mChannel) {
        g_io_channel_shutdown(mChannel, true, NULL);
        g_io_channel_unref(mChannel);
    }

    close(mFileDescriptor);
}

#endif //end _PARSER_INOTIFY_H_
