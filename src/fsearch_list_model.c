#include "fsearch_list_model.h"
#include "fsearch_file_utils.h"

struct _FsearchListModel {
    GObject parent_instance;
    FsearchDatabaseView *view;
    gint stamp;
};

static void fsearch_list_model_tree_model_init(GtkTreeModelIface *iface);

G_DEFINE_TYPE_WITH_CODE(FsearchListModel, fsearch_list_model, G_TYPE_OBJECT,
                        G_IMPLEMENT_INTERFACE(GTK_TYPE_TREE_MODEL, fsearch_list_model_tree_model_init))

static void
fsearch_list_model_finalize(GObject *object) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(object);
    if (self->view) {
        db_view_unref(self->view);
    }
    G_OBJECT_CLASS(fsearch_list_model_parent_class)->finalize(object);
}

static void
fsearch_list_model_init(FsearchListModel *self) {
    self->stamp = g_random_int();
}

static void
fsearch_list_model_class_init(FsearchListModelClass *klass) {
    GObjectClass *object_class = G_OBJECT_CLASS(klass);
    object_class->finalize = fsearch_list_model_finalize;
}

FsearchListModel *
fsearch_list_model_new(FsearchDatabaseView *view) {
    FsearchListModel *self = g_object_new(FSEARCH_TYPE_LIST_MODEL, NULL);
    self->view = db_view_ref(view);
    return self;
}

static GtkTreeModelFlags
fsearch_list_model_get_flags(GtkTreeModel *tree_model) {
    return GTK_TREE_MODEL_LIST_ONLY;
}

static gint
fsearch_list_model_get_n_columns(GtkTreeModel *tree_model) {
    return FSEARCH_LIST_MODEL_N_COLS;
}

static GType
fsearch_list_model_get_column_type(GtkTreeModel *tree_model, gint index) {
    if (index == FSEARCH_LIST_MODEL_COL_NAME) return G_TYPE_STRING;
    if (index == FSEARCH_LIST_MODEL_COL_ICON) return G_TYPE_ICON;
    return G_TYPE_INVALID;
}

static gboolean
fsearch_list_model_get_iter(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreePath *path) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    gint *indices = gtk_tree_path_get_indices(path);
    gint depth = gtk_tree_path_get_depth(path);

    if (depth != 1) return FALSE;

    gint idx = indices[0];
    db_view_lock(self->view);
    guint num = db_view_get_num_entries(self->view);
    db_view_unlock(self->view);

    if (idx >= 0 && idx < num) {
        iter->stamp = self->stamp;
        iter->user_data = GINT_TO_POINTER(idx);
        return TRUE;
    }
    return FALSE;
}

static GtkTreePath *
fsearch_list_model_get_path(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    g_return_val_if_fail(iter->stamp == self->stamp, NULL);

    gint idx = GPOINTER_TO_INT(iter->user_data);
    GtkTreePath *path = gtk_tree_path_new();
    gtk_tree_path_append_index(path, idx);
    return path;
}

static void
fsearch_list_model_get_value(GtkTreeModel *tree_model, GtkTreeIter *iter, gint column, GValue *value) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    g_return_if_fail(iter->stamp == self->stamp);
    gint idx = GPOINTER_TO_INT(iter->user_data);

    if (column == FSEARCH_LIST_MODEL_COL_NAME) {
        g_value_init(value, G_TYPE_STRING);
        db_view_lock(self->view);
        GString *name = db_view_entry_get_name_for_idx(self->view, idx);
        db_view_unlock(self->view);
        if (name) {
            g_value_take_string(value, g_string_free(name, FALSE));
        } else {
            g_value_set_string(value, "");
        }
    } else if (column == FSEARCH_LIST_MODEL_COL_ICON) {
        g_value_init(value, G_TYPE_ICON);
        db_view_lock(self->view);
        GString *path = db_view_entry_get_path_full_for_idx(self->view, idx);
        GString *name = db_view_entry_get_name_for_idx(self->view, idx);
        FsearchDatabaseEntryType type = db_view_entry_get_type_for_idx(self->view, idx);
        db_view_unlock(self->view);

        GIcon *icon = NULL;
        if (path && name) {
            icon = fsearch_file_utils_guess_icon(name->str, path->str, type == DATABASE_ENTRY_TYPE_FOLDER);
        }

        if (icon) {
            g_value_take_object(value, icon);
        }

        if (path) g_string_free(path, TRUE);
        if (name) g_string_free(name, TRUE);
    }
}

static gboolean
fsearch_list_model_iter_next(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    g_return_val_if_fail(iter->stamp == self->stamp, FALSE);

    gint idx = GPOINTER_TO_INT(iter->user_data);
    idx++;

    db_view_lock(self->view);
    guint num = db_view_get_num_entries(self->view);
    db_view_unlock(self->view);

    if (idx < num) {
        iter->user_data = GINT_TO_POINTER(idx);
        return TRUE;
    }
    return FALSE;
}

static gboolean
fsearch_list_model_iter_children(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    if (parent) return FALSE; // Flat list

    db_view_lock(self->view);
    guint num = db_view_get_num_entries(self->view);
    db_view_unlock(self->view);

    if (num > 0) {
        iter->stamp = self->stamp;
        iter->user_data = GINT_TO_POINTER(0);
        return TRUE;
    }
    return FALSE;
}

static gboolean
fsearch_list_model_iter_has_child(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    return FALSE;
}

static gint
fsearch_list_model_iter_n_children(GtkTreeModel *tree_model, GtkTreeIter *iter) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    if (iter) return 0;

    db_view_lock(self->view);
    guint num = db_view_get_num_entries(self->view);
    db_view_unlock(self->view);
    return num;
}

static gboolean
fsearch_list_model_iter_nth_child(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *parent, gint n) {
    FsearchListModel *self = FSEARCH_LIST_MODEL(tree_model);
    if (parent) return FALSE;

    db_view_lock(self->view);
    guint num = db_view_get_num_entries(self->view);
    db_view_unlock(self->view);

    if (n < num) {
        iter->stamp = self->stamp;
        iter->user_data = GINT_TO_POINTER(n);
        return TRUE;
    }
    return FALSE;
}

static gboolean
fsearch_list_model_iter_parent(GtkTreeModel *tree_model, GtkTreeIter *iter, GtkTreeIter *child) {
    return FALSE;
}

static void
fsearch_list_model_tree_model_init(GtkTreeModelIface *iface) {
    iface->get_flags = fsearch_list_model_get_flags;
    iface->get_n_columns = fsearch_list_model_get_n_columns;
    iface->get_column_type = fsearch_list_model_get_column_type;
    iface->get_iter = fsearch_list_model_get_iter;
    iface->get_path = fsearch_list_model_get_path;
    iface->get_value = fsearch_list_model_get_value;
    iface->iter_next = fsearch_list_model_iter_next;
    iface->iter_children = fsearch_list_model_iter_children;
    iface->iter_has_child = fsearch_list_model_iter_has_child;
    iface->iter_n_children = fsearch_list_model_iter_n_children;
    iface->iter_nth_child = fsearch_list_model_iter_nth_child;
    iface->iter_parent = fsearch_list_model_iter_parent;
}
