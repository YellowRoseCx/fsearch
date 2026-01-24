#include <glib.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <sys/stat.h>

#include "fsearch_database.h"
#include "fsearch_index.h"

static void
test_db_save_load_indexes(void) {
    // 1. Setup temp directory for indexing
    g_autofree char *tmp_dir = g_dir_make_tmp("fsearch-test-idx-XXXXXX", NULL);
    g_assert_nonnull(tmp_dir);

    // 2. Setup database file path
    g_autofree char *tmp_db_dir = g_dir_make_tmp("fsearch-test-db-XXXXXX", NULL);
    g_assert_nonnull(tmp_db_dir);

    // 3. Create indexes
    GList *indexes = NULL;
    FsearchIndex *idx1 = fsearch_index_new(FSEARCH_INDEX_FOLDER_TYPE, tmp_dir, true, true, false, 123456789);
    indexes = g_list_append(indexes, idx1);

    // Add another index
    g_autofree char *tmp_dir2 = g_dir_make_tmp("fsearch-test-idx2-XXXXXX", NULL);
    FsearchIndex *idx2 = fsearch_index_new(FSEARCH_INDEX_FOLDER_TYPE, tmp_dir2, false, true, true, 987654321);
    indexes = g_list_append(indexes, idx2);

    // 4. Create Database
    FsearchDatabase *db = db_new(indexes, NULL, NULL, false);

    // 5. Scan (to initialize internal structures needed for save)
    bool scan_res = db_scan(db, NULL, NULL);
    g_assert_true(scan_res);

    // 6. Save Database
    bool save_res = db_save(db, tmp_db_dir);
    g_assert_true(save_res);

    // 7. Load Database into new object
    FsearchDatabase *db_loaded = db_new(NULL, NULL, NULL, false);

    g_autofree char *db_path = g_build_filename(tmp_db_dir, "fsearch.db", NULL);
    bool load_res = db_load(db_loaded, db_path, NULL);
    g_assert_true(load_res);

    // 8. Verify indexes
    GList *loaded_indexes = db_get_indexes(db_loaded);
    g_assert_cmpint(g_list_length(loaded_indexes), ==, 2);

    FsearchIndex *l_idx1 = loaded_indexes->data;
    FsearchIndex *l_idx2 = loaded_indexes->next->data;

    if (strcmp(l_idx1->path, tmp_dir) != 0) {
        // swap
        FsearchIndex *tmp = l_idx1;
        l_idx1 = l_idx2;
        l_idx2 = tmp;
    }

    g_assert_cmpstr(l_idx1->path, ==, tmp_dir);
    g_assert_cmpint(l_idx1->type, ==, FSEARCH_INDEX_FOLDER_TYPE);
    g_assert_cmpint(l_idx1->enabled, ==, true);
    g_assert_cmpint(l_idx1->update, ==, true);
    g_assert_cmpint(l_idx1->one_filesystem, ==, false);
    g_assert_cmpint(l_idx1->last_updated, ==, 123456789);

    g_assert_cmpstr(l_idx2->path, ==, tmp_dir2);
    g_assert_cmpint(l_idx2->type, ==, FSEARCH_INDEX_FOLDER_TYPE);
    g_assert_cmpint(l_idx2->enabled, ==, false);
    g_assert_cmpint(l_idx2->update, ==, true);
    g_assert_cmpint(l_idx2->one_filesystem, ==, true);
    g_assert_cmpint(l_idx2->last_updated, ==, 987654321);

    // Cleanup
    g_list_free_full(indexes, (GDestroyNotify)fsearch_index_free);
    db_unref(db);
    db_unref(db_loaded);

    rmdir(tmp_dir);
    rmdir(tmp_dir2);
    remove(db_path);
    rmdir(tmp_db_dir);
}

int
main(int argc, char *argv[]) {
    g_test_init(&argc, &argv, NULL);

    g_test_add_func("/database/save_load_indexes", test_db_save_load_indexes);

    return g_test_run();
}
