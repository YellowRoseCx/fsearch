#include <glib.h>
#include <stddef.h>
#include <stdlib.h>
#include <string.h>

#include "fsearch_index.h"

FsearchIndex *
fsearch_index_new(FsearchIndexType type,
                  const char *path,
                  bool search_in,
                  bool update,
                  bool one_filesystem,
                  time_t last_updated) {
    FsearchIndex *index = calloc(1, sizeof(FsearchIndex));
    g_assert(index);

    index->type = type;
    index->path = path ? strdup(path) : strdup("");
    index->enabled = search_in;
    index->update = update;
    index->one_filesystem = one_filesystem;
    index->last_updated = last_updated;

    return index;
}

FsearchIndex *
fsearch_index_copy(FsearchIndex *index) {
    if (!index) {
        return NULL;
    }
    return fsearch_index_new(index->type,
                             index->path,
                             index->enabled,
                             index->update,
                             index->one_filesystem,
                             index->last_updated);
}

void
fsearch_index_free(FsearchIndex *index) {
    if (!index) {
        return;
    }

    g_clear_pointer(&index->path, free);
    g_clear_pointer(&index, free);
}

static bool
read_from_file(void *restrict ptr, size_t size, FILE *restrict stream) {
    return fread(ptr, size, 1, stream) == 1 ? true : false;
}

FsearchIndex *
fsearch_index_load(FILE *fp) {
    uint8_t type = 0;
    if (!read_from_file(&type, 1, fp)) {
        g_debug("[fsearch_index_load] failed to load index type");
        return NULL;
    }

    uint16_t path_len = 0;
    if (!read_from_file(&path_len, 2, fp)) {
        g_debug("[fsearch_index_load] failed to load index path length");
        return NULL;
    }

    g_autofree char *path = NULL;
    if (path_len > 0) {
        path = g_malloc(path_len + 1);
        if (!read_from_file(path, path_len, fp)) {
            g_debug("[fsearch_index_load] failed to load index path");
            return NULL;
        }
        path[path_len] = '\0';
    }

    uint8_t enabled = 0;
    if (!read_from_file(&enabled, 1, fp)) {
        g_debug("[fsearch_index_load] failed to load index enabled flag");
        return NULL;
    }

    uint8_t update = 0;
    if (!read_from_file(&update, 1, fp)) {
        g_debug("[fsearch_index_load] failed to load index update flag");
        return NULL;
    }

    uint8_t one_filesystem = 0;
    if (!read_from_file(&one_filesystem, 1, fp)) {
        g_debug("[fsearch_index_load] failed to load index one_filesystem flag");
        return NULL;
    }

    int64_t last_updated = 0;
    if (!read_from_file(&last_updated, 8, fp)) {
        g_debug("[fsearch_index_load] failed to load index last_updated");
        return NULL;
    }

    return fsearch_index_new(type, path, enabled, update, one_filesystem, last_updated);
}
