#include <glib.h>
#include <stdlib.h>
#include <unistd.h>
#include "../fsearch_database.h"
#include "../fsearch_exclude_path.h"

static void
test_save_load_excludes(void) {
    // 1. Create a DB with excludes
    GList *excludes = NULL;
    excludes = g_list_append(excludes, fsearch_exclude_path_new("/tmp/exclude1", true));
    excludes = g_list_append(excludes, fsearch_exclude_path_new("/tmp/exclude2", false));

    FsearchDatabase *db = db_new(NULL, excludes, NULL, false);
    g_list_free_full(excludes, (GDestroyNotify)fsearch_exclude_path_free);

    // Verify initial state
    GList *db_excludes = db_get_excludes(db);
    g_assert_cmpint(g_list_length(db_excludes), ==, 2);

    // 2. Save DB
    char *tmp_dir = g_dir_make_tmp("fsearch_test_XXXXXX", NULL);
    g_assert_nonnull(tmp_dir);

    g_assert_true(db_save(db, tmp_dir));

    db_unref(db);

    // 3. Load DB into new object
    // Initialize with NO excludes
    db = db_new(NULL, NULL, NULL, false);

    // Construct full path
    char *db_path = g_build_filename(tmp_dir, "fsearch.db", NULL);

    // db_load takes full path
    g_assert_true(db_load(db, db_path, NULL));

    // 4. Verify excludes are loaded
    db_excludes = db_get_excludes(db);

    g_assert_cmpint(g_list_length(db_excludes), ==, 2);

    // Iterate and check values. db_new sorts them.
    FsearchExcludePath *ex1 = g_list_nth_data(db_excludes, 0);
    FsearchExcludePath *ex2 = g_list_nth_data(db_excludes, 1);

    // Sort order depends on path
    g_assert_cmpstr(ex1->path, ==, "/tmp/exclude1");
    g_assert_true(ex1->enabled);

    g_assert_cmpstr(ex2->path, ==, "/tmp/exclude2");
    g_assert_false(ex2->enabled);

    db_unref(db);
    g_free(db_path);

    // cleanup tmp dir
    char *db_file = g_build_filename(tmp_dir, "fsearch.db", NULL);
    unlink(db_file);
    g_free(db_file);
    rmdir(tmp_dir);
    g_free(tmp_dir);
}

int
main(int argc, char *argv[]) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/FSearch/database/save_load_excludes", test_save_load_excludes);
    return g_test_run();
}
