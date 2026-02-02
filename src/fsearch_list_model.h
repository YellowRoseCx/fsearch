#pragma once

#include <glib-object.h>
#include <gtk/gtk.h>
#include "fsearch_database_view.h"

G_BEGIN_DECLS

#define FSEARCH_TYPE_LIST_MODEL (fsearch_list_model_get_type())
G_DECLARE_FINAL_TYPE(FsearchListModel, fsearch_list_model, FSEARCH, LIST_MODEL, GObject)

FsearchListModel *fsearch_list_model_new(FsearchDatabaseView *view);

enum {
    FSEARCH_LIST_MODEL_COL_NAME,
    FSEARCH_LIST_MODEL_COL_ICON,
    FSEARCH_LIST_MODEL_N_COLS
};

G_END_DECLS
