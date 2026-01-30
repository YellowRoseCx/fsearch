#include <glib.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <glib/gstdio.h>

#include <src/fsearch_database.h>
#include <src/fsearch_index.h>
#include <src/fsearch_array.h>
#include <src/fsearch_database_entry.h>

static void
create_dummy_file(const char *dir, const char *filename) {
    char *path = g_build_filename(dir, filename, NULL);
    FILE *fp = fopen(path, "w");
    g_assert_nonnull(fp);
    fprintf(fp, "dummy content");
    fclose(fp);
    g_free(path);
}

static bool
db_contains_file(FsearchDatabase *db, const char *filename) {
    DynamicArray *files = db_get_files(db);
    uint32_t count = darray_get_num_items(files);
    for (uint32_t i = 0; i < count; i++) {
        FsearchDatabaseEntry *entry = darray_get_item(files, i);
        if (g_strcmp0(db_entry_get_name_raw(entry), filename) == 0) {
            darray_unref(files);
            return true;
        }
    }
    darray_unref(files);
    return false;
}

static void
test_database_include_change(void) {
    char *tmp_dir_root = g_dir_make_tmp("fsearch_test_root_XXXXXX", NULL);
    g_assert_nonnull(tmp_dir_root);

    char *dir_a = g_build_filename(tmp_dir_root, "dir_a", NULL);
    g_mkdir(dir_a, 0700);
    create_dummy_file(dir_a, "file_a");

    char *dir_b = g_build_filename(tmp_dir_root, "dir_b", NULL);
    g_mkdir(dir_b, 0700);
    create_dummy_file(dir_b, "file_b");

    // 1. Initialize DB with dir_a
    GList *indexes_a = NULL;
    indexes_a = g_list_append(indexes_a, fsearch_index_new(FSEARCH_INDEX_FOLDER_TYPE, dir_a, true, true, false, 0));

    FsearchDatabase *db_a = db_new(indexes_a, NULL, NULL, false);
    g_assert_nonnull(db_a);

    // Scan
    db_scan(db_a, NULL, NULL);

    // Verify file_a is present
    g_assert_true(db_contains_file(db_a, "file_a"));
    g_assert_false(db_contains_file(db_a, "file_b"));

    // Save DB
    char *db_path = g_build_filename(tmp_dir_root, "fsearch_test", NULL); // Using a subdir to simulate db dir
    g_mkdir(db_path, 0700);
    bool save_res = db_save(db_a, db_path);
    g_assert_true(save_res);

    db_unref(db_a);
    g_list_free_full(indexes_a, (GDestroyNotify)fsearch_index_free);

    // 2. Initialize new DB with dir_b (simulating config change)
    GList *indexes_b = NULL;
    indexes_b = g_list_append(indexes_b, fsearch_index_new(FSEARCH_INDEX_FOLDER_TYPE, dir_b, true, true, false, 0));

    FsearchDatabase *db_b = db_new(indexes_b, NULL, NULL, false);
    g_assert_nonnull(db_b);

    // Scan
    db_scan(db_b, NULL, NULL);

    // Verify file_b is present in memory
    g_assert_true(db_contains_file(db_b, "file_b"));
    g_assert_false(db_contains_file(db_b, "file_a"));

    // Save DB (overwriting previous one)
    save_res = db_save(db_b, db_path);
    g_assert_true(save_res);

    db_unref(db_b);
    g_list_free_full(indexes_b, (GDestroyNotify)fsearch_index_free);

    // 3. Load DB to verify persistence state
    FsearchDatabase *db_loaded = db_new(NULL, NULL, NULL, false);
    char *db_file = g_build_filename(db_path, "fsearch.db", NULL);
    bool load_res = db_load(db_loaded, db_file, NULL);
    g_assert_true(load_res);

    // Verify file_a is GONE and file_b is PRESENT
    g_assert_false(db_contains_file(db_loaded, "file_a"));
    g_assert_true(db_contains_file(db_loaded, "file_b"));

    db_unref(db_loaded);

    // Cleanup
    char *file_a_path = g_build_filename(dir_a, "file_a", NULL);
    unlink(file_a_path);
    g_free(file_a_path);
    rmdir(dir_a);

    char *file_b_path = g_build_filename(dir_b, "file_b", NULL);
    unlink(file_b_path);
    g_free(file_b_path);
    rmdir(dir_b);

    unlink(db_file);
    rmdir(db_path);
    rmdir(tmp_dir_root);

    g_free(dir_a);
    g_free(dir_b);
    g_free(db_path);
    g_free(db_file);
    g_free(tmp_dir_root);
}

int
main(int argc, char *argv[]) {
    g_test_init(&argc, &argv, NULL);
    g_test_add_func("/FSearch/database/include_change", test_database_include_change);
    return g_test_run();
}
