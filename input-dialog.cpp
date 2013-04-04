#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <exo/exo.h>
#include <errno.h>

#include "tree.h"
#include "utils.h"


GtkListStore * SearchMaskStore;
GtkListStore * SearchTextStore;
GtkListStore * SearchModeStore;

//-------------------------------------------------------------------------------------------------

void InsertNewText(gchar *str)
{
	if(!strcmp(str,"")) return;
	GtkTreeIter   iter;
    gboolean valid;
    gchar * str_data;
    GtkTreeIter   prev_iter;
    int pos;

    valid = gtk_tree_model_get_iter_first ((GtkTreeModel*)SearchTextStore, &iter);
    pos=0;
    while (valid)
    {
      gtk_tree_model_get ((GtkTreeModel*)SearchTextStore, &iter, 0, &str_data, -1);

      if(!strcmp(str,str_data))
      {
       g_free (str_data);
       if(pos) gtk_list_store_move_before(SearchTextStore,&iter,&prev_iter);
       return;
      }

      g_free (str_data);
      pos++;
      if(pos>20)
      {
       valid = gtk_list_store_remove(SearchTextStore, &iter);
      } else
      {
       prev_iter=iter;
       valid = gtk_tree_model_iter_next ((GtkTreeModel*)SearchTextStore, &iter);
      }

    }

	gtk_tree_model_get_iter_first ((GtkTreeModel*)SearchTextStore, &iter);
    gtk_list_store_prepend(SearchTextStore, &iter );
    gtk_list_store_set( SearchTextStore, &iter, 0, str, -1 );

}

//-----------------------------------------------------------------------------

void InsertNewMask(gchar *str)
{
    GtkTreeIter   iter;
    gboolean valid;
    gchar * str_data;
    GtkTreeIter   prev_iter;
    int pos;

    valid = gtk_tree_model_get_iter_first ((GtkTreeModel*)SearchMaskStore, &iter);
    pos=0;
    while (valid)
    {
      gtk_tree_model_get ((GtkTreeModel*)SearchMaskStore, &iter, 0, &str_data, -1);

      if(!strcmp(str,str_data))
      {
       g_free (str_data);
       if(pos) gtk_list_store_move_before((GtkListStore*)SearchMaskStore,&iter,&prev_iter);
       return;
      }

      g_free (str_data);
      pos++;
      if(pos>7)
      {
       valid = gtk_list_store_remove(SearchMaskStore, &iter);
      } else
      {
       prev_iter=iter;
       valid = gtk_tree_model_iter_next ((GtkTreeModel*)SearchMaskStore, &iter);
      }

    }

    gtk_list_store_append(SearchMaskStore, &iter );
    gtk_list_store_set( SearchMaskStore, &iter, 0, str, -1 );

}

//-----------------------------------------------------------------------------

gchar * EditFileName(const char * fullname)
{
	gchar * to_path=NULL;
	ClassString cwd = g_path_get_dirname(fullname);
	ClassString shortname=g_path_get_basename(fullname);

	GtkWidget* dlg = gtk_dialog_new_with_buttons(  "New name of file" ,
                                       NULL,
                                       (GtkDialogFlags)0,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK,
                                       GTK_RESPONSE_OK,
	                                   NULL );
	
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    GtkWidget* box = ( ( GtkDialog* ) dlg )->vbox;
    GtkLabel* prompt = (GtkLabel*)gtk_label_new(  "Please input new file name:"  );
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

    GtkWidget* entry = (GtkWidget*) gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY( entry ), shortname.s);
    gtk_box_pack_start( GTK_BOX( box ), entry, FALSE, FALSE, 4 );

    gtk_dialog_set_default_response( ( GtkDialog* ) dlg, GTK_RESPONSE_OK );
    gtk_entry_set_activates_default ( GTK_ENTRY( entry ), TRUE );

    gtk_widget_show_all( box );

    gtk_window_set_default_size( GTK_WINDOW( dlg ), 360, -1 );
    gtk_widget_show( dlg );

    while ( gtk_dialog_run( GTK_DIALOG( dlg ) ) == GTK_RESPONSE_OK )
    {
        const char * newname = gtk_entry_get_text((GtkEntry*)entry);
        if ( newname )
        {
            to_path = g_build_filename( cwd.s, newname, NULL );
            if ( g_file_test( to_path, G_FILE_TEST_EXISTS ) )
            {
				gtk_label_set_text( prompt,
                                    _( "The file name you specified already exists.\n"
                                       "Please input a new one:" ) );
            }
            else
            {
				gtk_widget_destroy( dlg );
				return to_path;
            }
        }
    }//while
	gtk_widget_destroy( dlg );
	return NULL;
}

//-----------------------------------------------------------------------------

gchar * NewPattern(void)
{
    GtkLabel* prompt;
    GtkWidget* dlg;
	GtkWidget* box;
	gchar * mask=NULL;
		
    dlg = gtk_dialog_new_with_buttons( "Find files",
                                       NULL,
                                       (GtkDialogFlags)0,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK,
                                       GTK_RESPONSE_OK,
	                                   NULL );
	
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    box = ( ( GtkDialog* ) dlg )->vbox;
    prompt = (GtkLabel* ) gtk_label_new( "Input a mask file:" );
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

    GtkWidget* entry1 = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY( entry1 ), _( "*.*" ) );
    gtk_box_pack_start( GTK_BOX( box ), entry1, FALSE, FALSE, 4 );

    gtk_dialog_set_default_response( ( GtkDialog* ) dlg, GTK_RESPONSE_OK );
    gtk_entry_set_activates_default ( GTK_ENTRY( entry1 ), TRUE );
    gtk_widget_show_all( box );

/* end create elements */
	while ( gtk_dialog_run( GTK_DIALOG( dlg ) ) == GTK_RESPONSE_OK )
    {
		mask = g_strdup(gtk_entry_get_text( (GtkEntry*)entry1));
		break;
    }

	gtk_widget_destroy( dlg );
		printf("%s\n", mask);
	return mask;

}

//-----------------------------------------------------------------------------

void CreateNewFileDialog(const char * workdir,const char * command)
{
    const char* file_name;
    GtkLabel* prompt;
    GtkWidget* dlg;
	GtkWidget* box;
    GtkWidget* entry;

    dlg = gtk_dialog_new_with_buttons( _("Create new file"),
                                       NULL,
                                       (GtkDialogFlags)0,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK,
                                       GTK_RESPONSE_OK,
	                                   NULL );
	
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    box = ( ( GtkDialog* ) dlg )->vbox;
    prompt = (GtkLabel* ) gtk_label_new(_( "Input a name for the new file:" ));
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

    entry = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY( entry ), _( "newfile" ) );
    gtk_box_pack_start( GTK_BOX( box ), entry, FALSE, FALSE, 4 );

    gtk_dialog_set_default_response( ( GtkDialog* ) dlg, GTK_RESPONSE_OK );
    gtk_entry_set_activates_default ( GTK_ENTRY( entry ), TRUE );
    gtk_widget_show_all( box );

/* */
	while ( gtk_dialog_run( GTK_DIALOG( dlg ) ) == GTK_RESPONSE_OK )
    {
        file_name = gtk_entry_get_text((GtkEntry*)entry);
        ClassString full_path = g_build_filename(  workdir, file_name, NULL );

		if ( g_file_test( full_path.s, (GFileTest)(G_FILE_TEST_EXISTS | G_FILE_TEST_IS_DIR) ) )
        {
            gtk_label_set_text( prompt,
                                _( "The name you specified already used for folder.\n"
                                   "Please input a new one:" ) );
            continue;
        }
		ClassString com=g_strdup_printf("%s %s&",command,full_path.s);
		system( com.s );
		break;
    }

	gtk_widget_destroy( dlg );

}


//-----------------------------------------------------------------------------

void NewSearch(const char * workdir)
{
	int mode;
	GtkLabel* prompt;
    GtkWidget* dlg;
	GtkWidget* box;
    GtkTreeIter      iter;
	
	dlg = gtk_dialog_new_with_buttons( _("Find files"),
                                       NULL,
                                       (GtkDialogFlags)0,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK,
                                       GTK_RESPONSE_OK,
	                                   NULL );
	
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    box = ( ( GtkDialog* ) dlg )->vbox;
    prompt = (GtkLabel* ) gtk_label_new(_( "Input a mask file:" ));
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

    if(!SearchMaskStore)
	{	
		SearchMaskStore=gtk_list_store_new(1, G_TYPE_STRING);
		gtk_list_store_append( SearchMaskStore, &iter );
		gtk_list_store_set( SearchMaskStore, &iter, 0, "*.cpp", -1 );
		gtk_list_store_append( SearchMaskStore, &iter );
		gtk_list_store_set( SearchMaskStore, &iter, 0, "*.c", -1 );
		gtk_list_store_append( SearchMaskStore, &iter );
		gtk_list_store_set( SearchMaskStore, &iter, 0, "*.*", -1 );
		gtk_list_store_append( SearchMaskStore, &iter );
		gtk_list_store_set( SearchMaskStore, &iter, 0, "*", -1 );
	}	
    if(!SearchModeStore)
	{	
		SearchModeStore=gtk_list_store_new(1, G_TYPE_STRING);
		gtk_list_store_append( SearchModeStore, &iter );
		gtk_list_store_set( SearchModeStore, &iter, 0, "Folders", -1 );
		gtk_list_store_append( SearchModeStore, &iter );
		gtk_list_store_set( SearchModeStore, &iter, 0, "List", -1 );
	}	
		
	if(!SearchTextStore)
	{	
		SearchTextStore=gtk_list_store_new(1, G_TYPE_STRING);
	}	

	GtkWidget* EntryMask = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(SearchMaskStore),0);
   	gtk_box_pack_start( GTK_BOX( box ), EntryMask, FALSE, FALSE, 4 );
	gtk_tree_model_get_iter_first ((GtkTreeModel*)SearchMaskStore, &iter);	
    gtk_combo_box_set_active_iter( (GtkComboBox*)EntryMask, &iter  );	

	box = ( ( GtkDialog* ) dlg )->vbox;
    prompt = (GtkLabel* ) gtk_label_new(_( "Input a text :" ));
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

    GtkWidget* EntryText = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(SearchTextStore),0);
    gtk_box_pack_start( GTK_BOX( box ), EntryText, FALSE, FALSE, 4 );
	if(gtk_tree_model_get_iter_first ((GtkTreeModel*)SearchTextStore, &iter))
	{
	    gtk_combo_box_set_active_iter( (GtkComboBox*)EntryText, &iter  );
	}
	
    gtk_dialog_set_default_response( ( GtkDialog* ) dlg, GTK_RESPONSE_OK );

	prompt = (GtkLabel* ) gtk_label_new(_( "Отображать ввиде" ));
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

	GtkWidget* EntryMode = gtk_combo_box_entry_new_with_model(GTK_TREE_MODEL(SearchModeStore),0);
    gtk_box_pack_start( GTK_BOX( box ), EntryMode, FALSE, FALSE, 4 );
	if(gtk_tree_model_get_iter_first ((GtkTreeModel*)SearchModeStore, &iter))
	{
	    gtk_combo_box_set_active_iter( (GtkComboBox*)EntryMode, &iter  );
	}

	gtk_widget_show_all( box );

	/* end create elements */
	while ( gtk_dialog_run( GTK_DIALOG( dlg ) ) == GTK_RESPONSE_OK )
    {
	    gchar * mask = gtk_combo_box_get_active_text((GtkComboBox*)EntryMask);
		gchar * text = gtk_combo_box_get_active_text((GtkComboBox*)EntryText);
		gchar * smode = gtk_combo_box_get_active_text((GtkComboBox*)EntryMode);		
        InsertNewMask(mask);
        InsertNewText(text);
        mode=!strcmp(smode,"Folders");
		ExternalFind(mask,text,Panels[ActivePanel]->get_path(),mode);

		ClassString dest;
		if(mode)
		{	
			dest = g_build_filename(PATH_FIND,workdir, NULL );
			CreateDirInDir(dest.s);
		} else
		{
			dest = g_build_filename(PATH_FIND, NULL );
		}

		Panels[1-ActivePanel]->LoadDir(dest.s);
		g_free(mask);
		g_free(text);
		g_free(smode);
		break;
    }

	gtk_widget_destroy( dlg );

}

//-----------------------------------------------------------------------------