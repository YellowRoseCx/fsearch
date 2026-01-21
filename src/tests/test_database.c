#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>

#include <src/fsearch_database.h>
#include <src/fsearch_exclude_path.h>

static void
test_db_excludes_persistence(void) {
    char *tmp_dir = g_dir_make_tmp("fsearch_test_XXXXXX", NULL);
    g_assert_nonnull(tmp_dir);

    // Create excludes list
    GList *excludes = NULL;
    excludes = g_list_append(excludes, fsearch_exclude_path_new("/tmp/foo", true));
    excludes = g_list_append(excludes, fsearch_exclude_path_new("/tmp/bar", false));
    excludes = g_list_append(excludes, fsearch_exclude_path_new("/home/user/test", true));

    // Create DB with excludes
    FsearchDatabase *db = db_new(NULL, excludes, NULL, false);
    g_assert_nonnull(db);

    // Save DB
    bool save_res = db_save(db, tmp_dir);
    g_assert_true(save_res);

    db_unref(db);
    // excludes list is copied by db_new, so we need to free our local list
    g_list_free_full(excludes, (GDestroyNotify)fsearch_exclude_path_free);

    // Create new empty DB
    FsearchDatabase *new_db = db_new(NULL, NULL, NULL, false);
    g_assert_nonnull(new_db);

    // Load DB
    char *db_path = g_build_filename(tmp_dir, "fsearch.db", NULL);
    bool load_res = db_load(new_db, db_path, NULL);
    g_assert_true(load_res);

    // Check excludes
    GList *loaded_excludes = db_get_excludes(new_db);
    g_assert_cmpint(g_list_length(loaded_excludes), ==, 3);

    // Verify content (order should be preserved because db_new sorts them? No, db_new sorts them!)
    // db_new does: db->excludes = g_list_sort(db->excludes, (GCompareFunc)compare_exclude_path);
    // compare_exclude_path compares paths.

    // "/home/user/test" comes first
    FsearchExcludePath *ep0 = g_list_nth_data(loaded_excludes, 0);
    g_assert_cmpstr(ep0->path, ==, "/home/user/test");
    g_assert_true(ep0->enabled);

    // "/tmp/bar"
    FsearchExcludePath *ep1 = g_list_nth_data(loaded_excludes, 1);
    g_assert_cmpstr(ep1->path, ==, "/tmp/bar");
    g_assert_false(ep1->enabled);

    // "/tmp/foo"
    FsearchExcludePath *ep2 = g_list_nth_data(loaded_excludes, 2);
    g_assert_cmpstr(ep2->path, ==, "/tmp/foo");
    g_assert_true(ep2->enabled);

    // Cleanup
    db_unref(new_db);
    g_free(db_path);

    // Remove temporary directory and files
    char *db_file = g_build_filename(tmp_dir, "fsearch.db", NULL);
    unlink(db_file);
    g_free(db_file);
    rmdir(tmp_dir);
    g_free(tmp_dir);
}

int
main(int argc, char *argv[]) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/FSearch/database/excludes_persistence", test_db_excludes_persistence);
    return g_test_run();
}
