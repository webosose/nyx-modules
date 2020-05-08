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


#include <errno.h>
#include <unistd.h>
#include <sys/stat.h>
#include <dirent.h>
#include <sys/inotify.h>
#include <glib-2.0/glib.h>

struct inotify_event;

typedef void (* inotify_event_cb) (struct inotify_event *event,
                                   const char *ident);

struct parser_inotify {
    unsigned int refcount;
    GIOChannel *channel;
    uint watch;
    int wd;
    GSList *list;
};

static void cleanup_inotify(gpointer user_data);

static void parser_inotify_ref(struct parser_inotify *i)
{
    __sync_fetch_and_add(&i->refcount, 1);
}

static void parser_inotify_unref(gpointer data)
{
    struct parser_inotify *i = (parser_inotify *) data;

    if (__sync_fetch_and_sub(&i->refcount, 1) != 1)
        return;

    cleanup_inotify(data);
}

static GHashTable *inotify_hash;

static gboolean inotify_data(GIOChannel *channel, GIOCondition cond,
                             gpointer user_data)
{
    struct parser_inotify *inotify = (parser_inotify *) user_data;
    char buffer[sizeof(struct inotify_event) + NAME_MAX + 1];
    char *next_event;
    gsize bytes_read;
    GIOStatus status;
    GSList *list;

    if (cond & (G_IO_NVAL | G_IO_ERR | G_IO_HUP)) {
        inotify->watch = 0;
        return FALSE;
    }

    status = g_io_channel_read_chars(channel, buffer,
                   sizeof(buffer), &bytes_read, NULL);

    switch (status) {
        case G_IO_STATUS_NORMAL:
    break;
        case G_IO_STATUS_AGAIN:
    return TRUE;
        default:
    inotify->watch = 0;
    return FALSE;
    }

    next_event = buffer;

    parser_inotify_ref(inotify);

    while (bytes_read > 0) {
        struct inotify_event *event;
        gchar *ident;
        gsize len;

        event = (struct inotify_event *) next_event;
        if (event->len)
            ident = next_event + sizeof(struct inotify_event);
        else
            ident = NULL;

        len = sizeof(struct inotify_event) + event->len;

        /* check if inotify_event block fit */
        if (len > bytes_read)
            break;

        next_event += len;
        bytes_read -= len;

        for (list = inotify->list; list; list = list->next) {
            inotify_event_cb callback = (inotify_event_cb)list->data;
            (*callback)(event, ident);
        }
    }

    parser_inotify_unref(inotify);

    return TRUE;
}

static int create_watch(const char *path, struct parser_inotify *inotify)
{
    int fd = inotify_init();
    if (fd < 0)
        return -EIO;

    inotify->wd = inotify_add_watch(fd, path,
            IN_MODIFY | IN_CREATE | IN_DELETE |
            IN_MOVED_TO | IN_MOVED_FROM);
    if (inotify->wd < 0) {
        close(fd);
        return -EIO;
    }

    inotify->channel = g_io_channel_unix_new(fd);
    if (!inotify->channel) {
        inotify_rm_watch(fd, inotify->wd);
        inotify->wd = 0;
        close(fd);
        return -EIO;
    }

    g_io_channel_set_close_on_unref(inotify->channel, TRUE);
    g_io_channel_set_encoding(inotify->channel, NULL, NULL);
    g_io_channel_set_buffered(inotify->channel, FALSE);

    inotify->watch = g_io_add_watch(inotify->channel,
                    (GIOCondition)(G_IO_IN | G_IO_HUP | G_IO_NVAL | G_IO_ERR),
                    inotify_data, inotify);

    return 0;
}

static void remove_watch(struct parser_inotify *inotify)
{
    int fd;

    if (!inotify->channel)
        return;

    if (inotify->watch > 0)
        g_source_remove(inotify->watch);

    fd = g_io_channel_unix_get_fd(inotify->channel);

    if (inotify->wd >= 0)
        inotify_rm_watch(fd, inotify->wd);

    g_io_channel_unref(inotify->channel);
}

int parser_inotify_register(const char *path, inotify_event_cb callback)
{
    struct parser_inotify *inotify;
    int err;

    if (!callback)
        return -EINVAL;

    inotify = (parser_inotify *)g_hash_table_lookup(inotify_hash, path);
    if (inotify)
        goto update;

    inotify = g_try_new0(struct parser_inotify, 1);
    if (!inotify)
        return -ENOMEM;

    inotify->refcount = 1;
    inotify->wd = -1;

    err = create_watch(path, inotify);
    if (err < 0) {
        g_free(inotify);
        return err;
    }

    g_hash_table_replace(inotify_hash, g_strdup(path), inotify);

update:
    inotify->list = g_slist_prepend(inotify->list, (gpointer)callback);

    return 0;
}

static void cleanup_inotify(gpointer user_data)
{
    struct parser_inotify *inotify = (parser_inotify *)user_data;

    g_slist_free(inotify->list);

    remove_watch(inotify);
    g_free(inotify);
}

void parser_inotify_unregister(const char *path, inotify_event_cb callback)
{
    struct parser_inotify *inotify;

    inotify = (parser_inotify *)g_hash_table_lookup(inotify_hash, path);
    if (!inotify)
        return;

    inotify->list = g_slist_remove(inotify->list, (gconstpointer)callback);
    if (inotify->list)
        return;

    g_hash_table_remove(inotify_hash, path);
}

int parser_inotify_init(void)
{
    inotify_hash = g_hash_table_new_full(g_str_hash, g_str_equal,
                                         g_free, parser_inotify_unref);
    return 0;
}

void parser_inotify_cleanup(void)
{
    g_hash_table_destroy(inotify_hash);
}
