#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tree.h"

//-------------------------------------------------------------------------------------------------

void insert_if_unique(GtkTreeModel* model, char *str )
{
    GtkTreeIter   iter;
    gboolean valid;
//    gchar * str_data;
    GtkTreeIter   prev_iter;
    int pos;

    valid = gtk_tree_model_get_iter_first (model, &iter);
    pos=0;
    while (valid)
    {
      /* проходим через список, читаем каждую строку */
      gchar *str_data;
  //    gint   int_data;

      /* удостоверьтесь что вы закончили вызов gtk_tree_model_get()
       * значением '-1' */
      gtk_tree_model_get (model, &iter, 
                          1, &str_data,
                          -1);

      /* делаем кое-что с данными */
      g_print ("row (%s)\n", str_data);
      
      if(!strcmp(str,str_data))
      {
       g_free (str_data);
       if(pos) gtk_list_store_move_before((GtkListStore*)model,&iter,&prev_iter);
       return;
      }

      g_free (str_data);
      pos++;
      if(pos>7)
      {
       valid = gtk_list_store_remove((GtkListStore*)model, &iter);
      } else
      {
       prev_iter=iter;
       valid = gtk_tree_model_iter_next (model, &iter);
      }

    }

    gtk_list_store_append((GtkListStore*)model, &iter );
    gtk_list_store_set((GtkListStore*) model, &iter, 1, str, -1 );

}

//-----------------------------------------------------------------------------

void cb_changed( GtkComboBox *combo, gpointer     data )
{
    GtkTreeIter   iter;
    gchar        *string = NULL;
    GtkTreeModel *model;

    if( gtk_combo_box_get_active_iter( combo, &iter ) )
    {
        model = gtk_combo_box_get_model( combo );
        gtk_tree_model_get( model, &iter, 0, &string, -1 );
    }

    g_print( "Selected (complex): >> %s <<\n", ( string ? string : "NULL" ) );
    Panels[ActivePanel]->load_sort(string);
//    if( string )     g_free( string );
}

//-----------------------------------------------------------------------------

void 	InitSortCombo(void)
{
	GtkWidget *combo = (GtkWidget*) gtk_builder_get_object( builder, "combobox1");

	GtkListStore    *store;
	GtkTreeIter      iter;
	GtkCellRenderer *cell;

	store = gtk_list_store_new( 1, G_TYPE_STRING );


	gtk_list_store_append( store, &iter );
	gtk_list_store_set( store, &iter, 0, "name", -1 );
	gtk_list_store_prepend( store, &iter );
	gtk_list_store_set( store, &iter, 0, "ext", -1 );
	gtk_list_store_insert( store, &iter, 1 );
	gtk_list_store_set( store, &iter, 0, "size", -1 );
	gtk_list_store_insert( store, &iter, 1 );
	gtk_list_store_set( store, &iter, 0, "time", -1 );


	gtk_combo_box_set_model((GtkComboBox*)combo,(GtkTreeModel*) store );
	g_object_unref( G_OBJECT( store ) );
	cell = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( combo ), cell, TRUE );
	gtk_cell_layout_set_attributes( GTK_CELL_LAYOUT( combo ), cell, "text", 0, NULL );
	g_signal_connect( G_OBJECT( combo ), "changed",  G_CALLBACK( cb_changed ), NULL );
}

//-----------------------------------------------------------------------------
