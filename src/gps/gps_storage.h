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

#ifndef GPS_STORAGE_H
#define GPS_STORAGE_H

#include <glib.h>

#include <nyx/module/nyx_log.h>

#define GPS_MOCK_INFO         "GPSMOCK"
#define DEFAULT_LATENCY       2
static const char* mock_conf_path_name = "/etc/location/mock.conf";

static GKeyFile *load_conf_file(const char *file_path_name)
{
    GError *error = NULL;
    GKeyFile *keyfile = g_key_file_new();

    if (!g_key_file_load_from_file(keyfile, file_path_name, G_KEY_FILE_NONE, &error)) {
        nyx_error("MSGID_GPS_STORAGE", 0, "file: %s  load failed: %s\n", file_path_name, error->message);
        g_clear_error(&error);

        g_key_file_free(keyfile);
        keyfile = NULL;
    }

    return keyfile;
}

static bool save_conf_data(GKeyFile *keyfile, const char *file_path_name)
{
    gsize dataStringLen = 0;
    GError *error = NULL;
    bool ret = true;

    gchar *dataString = g_key_file_to_data(keyfile, &dataStringLen, NULL);

    if (!g_file_set_contents(file_path_name, dataString, dataStringLen, &error)) {
        nyx_error("MSGID_GPS_STORAGE", 0, "file: %s  save failed: %s\n", file_path_name, error->message);
        g_error_free(error);
        ret = false;
    }

    g_free(dataString);

    return ret;
}

static GKeyFile *open_conf_file(const char *file_path_name)
{
    GKeyFile *keyfile = load_conf_file(file_path_name);

    if (!keyfile)
        keyfile = g_key_file_new();

    return keyfile;
}

#endif
