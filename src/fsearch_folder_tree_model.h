#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>
#include "fsearch_database.h"

G_BEGIN_DECLS

#define FSEARCH_TYPE_FOLDER_TREE_MODEL (fsearch_folder_tree_model_get_type())
G_DECLARE_FINAL_TYPE(FsearchFolderTreeModel, fsearch_folder_tree_model, FSEARCH, FOLDER_TREE_MODEL, GObject)

FsearchFolderTreeModel *fsearch_folder_tree_model_new(FsearchDatabase *db);

FsearchDatabaseEntryFolder *fsearch_folder_tree_model_get_folder(FsearchFolderTreeModel *model, GtkTreeIter *iter);

G_END_DECLS
