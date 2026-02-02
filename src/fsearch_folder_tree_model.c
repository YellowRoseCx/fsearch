#include "fsearch_folder_tree_model.h"
#include "fsearch_database_entry.h"
#include "fsearch_array.h"

struct _FsearchFolderTreeModel {
    GObject parent_instance;
    FsearchDatabase *db;
    gint stamp;
};

static void fsearch_folder_tree_model_tree_model_init(GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE(FsearchFolderTreeModel, fsearch_folder_tree_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, fsearch_folder_tree_model_tree_model_init))

static void
fsearch_folder_tree_model_finalize(GObject *object) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(object);
    if (self->db) {
        db_unref(self->db);
    }
    G_OBJECT_CLASS(fsearch_folder_tree_model_parent_class)->finalize(object);
}

static void
fsearch_folder_tree_model_init(FsearchFolderTreeModel *self) {
    self->stamp = g_random_int();
}

static void
fsearch_folder_tree_model_class_init(FsearchFolderTreeModelClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = fsearch_folder_tree_model_finalize;
}

FsearchFolderTreeModel *
fsearch_folder_tree_model_new(FsearchDatabase *db) {
    FsearchFolderTreeModel *self = g_object_new(FSEARCH_TYPE_FOLDER_TREE_MODEL, NULL);
    self->db = db_ref(db);
    return self;
}

static GtkTreeModelFlags
fsearch_folder_tree_model_get_flags(GtkTreeModel *tree_model) {
    return GTK_TREE_MODEL_ITERS_PERSIST; // Pointers to folders are persistent as long as DB is valid
}

static gint
fsearch_folder_tree_model_get_n_columns(GtkTreeModel *tree_model) {
    return 1;
}

static GType
fsearch_folder_tree_model_get_column_type(GtkTreeModel *tree_model, gint index) {
    return G_TYPE_STRING;
}

static gboolean
fsearch_folder_tree_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);
    gint *indices = gtk_tree_path_get_indices(path);
    gint depth = gtk_tree_path_get_depth(path);

    FsearchDatabaseEntryFolder *parent = NULL;
    FsearchDatabaseEntryFolder *child = NULL;

    for (int i = 0; i < depth; i++) {
        DynamicArray *children = db_get_children_folders(self->db, parent);
        if (!children) return FALSE;

        if (indices[i] >= darray_get_num_items(children)) {
            darray_unref(children);
            return FALSE;
        }

        child = darray_get_item(children, indices[i]);
        darray_unref(children);
        parent = child;
    }

    if (child) {
        iter->stamp = self->stamp;
        iter->user_data = child;
        return TRUE;
    }
    return FALSE;
}

static GtkTreePath *
fsearch_folder_tree_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);
    g_return_val_if_fail(iter->stamp == self->stamp, NULL);

    FsearchDatabaseEntryFolder *folder = iter->user_data;
    GtkTreePath *path = gtk_tree_path_new();

    while (folder) {
        FsearchDatabaseEntryFolder *parent = db_entry_get_parent((FsearchDatabaseEntry *)folder);
        DynamicArray *siblings = db_get_children_folders(self->db, parent);

        uint32_t idx = 0;
        // Find index of folder in siblings
        // Using pointer comparison
        bool found = false;
        for (uint32_t i = 0; i < darray_get_num_items(siblings); i++) {
            if (darray_get_item(siblings, i) == folder) {
                idx = i;
                found = true;
                break;
            }
        }
        darray_unref(siblings);

        if (found) {
            gtk_tree_path_prepend_index(path, idx);
        } else {
            // Should not happen if DB is consistent
            gtk_tree_path_free(path);
            return NULL;
        }
        folder = parent;
    }
    return path;
}

static void
fsearch_folder_tree_model_get_value(GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);
    g_return_if_fail(iter->stamp == self->stamp);
    FsearchDatabaseEntryFolder *folder = iter->user_data;

    if (column == 0) {
        g_value_init(value, G_TYPE_STRING);
        GString *name = db_entry_get_name_for_display((FsearchDatabaseEntry *)folder);
        if (name) {
            g_value_take_string(value, g_string_free(name, FALSE));
        } else {
            g_value_set_string(value, "");
        }
    }
}

static gboolean
fsearch_folder_tree_model_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);
    g_return_val_if_fail(iter->stamp == self->stamp, FALSE);

    FsearchDatabaseEntryFolder *folder = iter->user_data;
    FsearchDatabaseEntryFolder *parent = db_entry_get_parent((FsearchDatabaseEntry *)folder);

    DynamicArray *siblings = db_get_children_folders(self->db, parent);
    uint32_t num = darray_get_num_items(siblings);

    uint32_t idx = 0;
    bool found = false;
    for (uint32_t i = 0; i < num; i++) {
        if (darray_get_item(siblings, i) == folder) {
            idx = i;
            found = true;
            break;
        }
    }

    gboolean success = FALSE;
    if (found && idx + 1 < num) {
        iter->user_data = darray_get_item(siblings, idx + 1);
        success = TRUE;
    }

    darray_unref(siblings);
    return success;
}

static gboolean
fsearch_folder_tree_model_iter_children(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);

    FsearchDatabaseEntryFolder *parent_folder = NULL;
    if (parent) {
        g_return_val_if_fail(parent->stamp == self->stamp, FALSE);
        parent_folder = parent->user_data;
    }

    DynamicArray *children = db_get_children_folders(self->db, parent_folder);
    if (darray_get_num_items(children) > 0) {
        iter->stamp = self->stamp;
        iter->user_data = darray_get_item(children, 0);
        darray_unref(children);
        return TRUE;
    }
    darray_unref(children);
    return FALSE;
}

static gboolean
fsearch_folder_tree_model_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);
    g_return_val_if_fail(iter->stamp == self->stamp, FALSE);

    FsearchDatabaseEntryFolder *folder = iter->user_data;
    return db_entry_folder_get_num_folders(folder) > 0;
}

static gint
fsearch_folder_tree_model_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);

    if (!iter) {
        // Root children count
        DynamicArray *roots = db_get_children_folders(self->db, NULL);
        int n = darray_get_num_items(roots);
        darray_unref(roots);
        return n;
    }

    g_return_val_if_fail(iter->stamp == self->stamp, 0);
    FsearchDatabaseEntryFolder *folder = iter->user_data;
    return db_entry_folder_get_num_folders(folder);
}

static gboolean
fsearch_folder_tree_model_iter_nth_child(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);

    FsearchDatabaseEntryFolder *parent_folder = NULL;
    if (parent) {
        g_return_val_if_fail(parent->stamp == self->stamp, FALSE);
        parent_folder = parent->user_data;
    }

    DynamicArray *children = db_get_children_folders(self->db, parent_folder);
    if (n < darray_get_num_items(children)) {
        iter->stamp = self->stamp;
        iter->user_data = darray_get_item(children, n);
        darray_unref(children);
        return TRUE;
    }
    darray_unref(children);
    return FALSE;
}

static gboolean
fsearch_folder_tree_model_iter_parent(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child) {
    FsearchFolderTreeModel *self = FSEARCH_FOLDER_TREE_MODEL(tree_model);
    g_return_val_if_fail(child->stamp == self->stamp, FALSE);

    FsearchDatabaseEntryFolder *folder = child->user_data;
    FsearchDatabaseEntryFolder *parent = db_entry_get_parent((FsearchDatabaseEntry *)folder);

    if (parent) {
        iter->stamp = self->stamp;
        iter->user_data = parent;
        return TRUE;
    }
    return FALSE;
}

static void
fsearch_folder_tree_model_tree_model_init(GtkTreeModelIface *iface) {
    iface->get_flags = fsearch_folder_tree_model_get_flags;
    iface->get_n_columns = fsearch_folder_tree_model_get_n_columns;
    iface->get_column_type = fsearch_folder_tree_model_get_column_type;
    iface->get_iter = fsearch_folder_tree_model_get_iter;
    iface->get_path = fsearch_folder_tree_model_get_path;
    iface->get_value = fsearch_folder_tree_model_get_value;
    iface->iter_next = fsearch_folder_tree_model_iter_next;
    iface->iter_children = fsearch_folder_tree_model_iter_children;
    iface->iter_has_child = fsearch_folder_tree_model_iter_has_child;
    iface->iter_n_children = fsearch_folder_tree_model_iter_n_children;
    iface->iter_nth_child = fsearch_folder_tree_model_iter_nth_child;
    iface->iter_parent = fsearch_folder_tree_model_iter_parent;
}

FsearchDatabaseEntryFolder *
fsearch_folder_tree_model_get_folder(FsearchFolderTreeModel *model, GtkTreeIter *iter) {
    g_return_val_if_fail(FSEARCH_IS_FOLDER_TREE_MODEL(model), NULL);
    g_return_val_if_fail(iter != NULL, NULL);
    return (FsearchDatabaseEntryFolder *)iter->user_data;
}
