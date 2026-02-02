#include "fsearch_sidebar.h"
#include "fsearch_database_entry.h"

struct _FsearchSidebar {
    GtkTreeView parent_instance;
};

enum {
    SIGNAL_FOLDER_SELECTED,
    N_SIGNALS
};

static guint signals[N_SIGNALS] = { 0 };

G_DEFINE_TYPE(FsearchSidebar, fsearch_sidebar, GTK_TYPE_TREE_VIEW)

static void
on_selection_changed(GtkTreeSelection *selection, FsearchSidebar *self) {
    GtkTreeIter iter;
    GtkTreeModel *model;

    if (gtk_tree_selection_get_selected(selection, &model, &iter)) {
        if (FSEARCH_IS_FOLDER_TREE_MODEL(model)) {
            FsearchDatabaseEntryFolder *folder = fsearch_folder_tree_model_get_folder(FSEARCH_FOLDER_TREE_MODEL(model), &iter);
            g_signal_emit(self, signals[SIGNAL_FOLDER_SELECTED], 0, folder);
        }
    }
}

static void
fsearch_sidebar_init(FsearchSidebar *self) {
    gtk_tree_view_set_headers_visible(GTK_TREE_VIEW(self), FALSE);

    GtkCellRenderer *renderer = gtk_cell_renderer_text_new();
    GtkTreeViewColumn *column = gtk_tree_view_column_new_with_attributes("Name", renderer, "text", 0, NULL);
    gtk_tree_view_append_column(GTK_TREE_VIEW(self), column);

    GtkTreeSelection *selection = gtk_tree_view_get_selection(GTK_TREE_VIEW(self));
    g_signal_connect(selection, "changed", G_CALLBACK(on_selection_changed), self);
}

static void
fsearch_sidebar_class_init(FsearchSidebarClass *klass) {
    signals[SIGNAL_FOLDER_SELECTED] = g_signal_new("folder-selected",
                                                   G_TYPE_FROM_CLASS(klass),
                                                   G_SIGNAL_RUN_LAST,
                                                   0,
                                                   NULL, NULL,
                                                   NULL,
                                                   G_TYPE_NONE,
                                                   1, G_TYPE_POINTER);
}

GtkWidget *
fsearch_sidebar_new(FsearchFolderTreeModel *model) {
    return g_object_new(FSEARCH_TYPE_SIDEBAR, "model", model, NULL);
}
