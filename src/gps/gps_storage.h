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

#include <dirent.h>
#include <errno.h>
#include <glib.h>
#include <glib/gprintf.h>
#include <sys/stat.h>
#include <stdbool.h>
#include <unistd.h>

#include <nyx/module/nyx_log.h>

#define GPS_MOCK_INFO         "GPSMOCK"
#define DEFAULT_LATENCY       2
#define MOCK_CONFIG_FILE_NAME "mock.conf"
#define LOC_CONFIG_DIR        "/etc/location"

#define MODE            (S_IRUSR | S_IWUSR | S_IXUSR | S_IRGRP | \
                        S_IXGRP | S_IROTH | S_IXOTH)

static GKeyFile *gps_config_load(const char *pathname)
{
    GKeyFile *keyfile = NULL;
    GError *error = NULL;

    keyfile = g_key_file_new();

    if (!g_key_file_load_from_file(keyfile, pathname, (GKeyFileFlags)0, &error)) {
        nyx_error("MSGID_NMEA_PARSER", 0, "Unable to load %s: %s\n", pathname, error->message);
        g_clear_error(&error);

        g_key_file_free(keyfile);
        keyfile = NULL;
    }

    return keyfile;
}

static int gps_config_save(GKeyFile *keyfile, char *pathname)
{
    gchar *data = NULL;
    gsize length = 0;
    GError *error = NULL;
    int ret = 0;

    data = g_key_file_to_data(keyfile, &length, NULL);

    if (!g_file_set_contents(pathname, data, length, &error)) {
        nyx_error("MSGID_NMEA_PARSER", 0, "Failed to store information: %s\n", error->message);
        g_error_free(error);
        ret = -EIO;
    }

    g_free(data);

    return ret;
}

GKeyFile *gps_config_open_file()
{
    gchar *pathname;
    GKeyFile *keyfile = NULL;

    pathname = g_strdup_printf("%s/%s", LOC_CONFIG_DIR, MOCK_CONFIG_FILE_NAME);
    if (!pathname)
        return NULL;

    keyfile =  gps_config_load(pathname);
    if (keyfile) {
        g_free(pathname);
        return keyfile;
    }

    g_free(pathname);

    keyfile = g_key_file_new();

    return keyfile;
}

GKeyFile *gps_config_load_file()
{
    gchar *pathname;
    GKeyFile *keyfile = NULL;

    pathname = g_strdup_printf("%s/%s", LOC_CONFIG_DIR, MOCK_CONFIG_FILE_NAME);
    if (!pathname)
        return NULL;

    keyfile =  gps_config_load(pathname);
    g_free(pathname);

    return keyfile;
}

int gps_config_save_file(GKeyFile *keyfile)
{
    int ret = 0;

    gchar *pathname, *dirname;

    dirname = g_strdup_printf("%s", LOC_CONFIG_DIR);
    if (!dirname)
        return -ENOMEM;

    /* If the dir doesn't exist, create it */
    if (!g_file_test(dirname, G_FILE_TEST_IS_DIR)) {
        if (mkdir(dirname, MODE) < 0) {
            if (errno != EEXIST) {
                g_free(dirname);
                return -errno;
            }
        }
    }

    pathname = g_strdup_printf("%s/%s", dirname, MOCK_CONFIG_FILE_NAME);

    g_free(dirname);

    ret = gps_config_save(keyfile, pathname);

    g_free(pathname);

    return ret;
}

#endif
