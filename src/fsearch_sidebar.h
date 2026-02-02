#pragma once

#include <gtk/gtk.h>
#include "fsearch_folder_tree_model.h"

G_BEGIN_DECLS

#define FSEARCH_TYPE_SIDEBAR (fsearch_sidebar_get_type())
G_DECLARE_FINAL_TYPE(FsearchSidebar, fsearch_sidebar, FSEARCH, SIDEBAR, GtkTreeView)

GtkWidget *fsearch_sidebar_new(FsearchFolderTreeModel *model);

G_END_DECLS
