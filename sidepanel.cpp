#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "tree.h"
#include "trash.h"
#include "disks.h"
#include "utils.h"

void view_popup_menu_trash(GdkEventButton *event);
void ShowMenuDropbox(GdkEventButton *event);

void ClickMenuDeleteBookmark(GtkWidget *menuitem, gpointer userdata);
void ShowMenuBookmark(GdkEventButton *event);

static void SelectionChangeSideView (GtkTreeView *tree_view, gpointer userdata);

//----------------------------------------------------------------------------- 

void ClassSidePanel::SetDropboxIcon(int mode)
{
	GtkTreeIter iter;
	if(FindIter("Dropbox",&iter,MODEL_TEXT_NAME))
	{
		GtkTreeModel* model=GetModel();
		int mime_type=mode + ICON_GNOME_DROPBOX;
		gtk_list_store_set((GtkListStore*)model, &iter, MODEL_INT_COL_MIME,mime_type,-1);
	}   
}

//-----------------------------------------------------------------------------

static gboolean view_sep_func( GtkTreeModel* model,
                               GtkTreeIter* it, gpointer data )
{
	int i;
	gchar* name=NULL;
	gtk_tree_model_get(model, it, MODEL_TEXT_FULL_NAME, &name, -1);
	i=!strcmp(name,"separator");
	g_free(name);
	return i;
}

//-----------------------------------------------------------------------------

ClassSidePanel::ClassSidePanel()
{
//	DropboxStorePath=0;
}

//-----------------------------------------------------------------------------

void ClassSidePanel::CreatePanel(void) 
{
	treeview = (GtkTreeView*)gtk_tree_view_new();
	g_signal_connect((GtkWidget *)treeview, "cursor-changed", (GCallback) SelectionChangeSideView, this );

	create_icon_column( COL_ICON );

	create_text_column(MODEL_TEXT_NAME);
	gtk_tree_selection_set_mode( gtk_tree_view_get_selection( treeview), GTK_SELECTION_MULTIPLE );

	gtk_tree_view_set_row_separator_func( treeview, ( GtkTreeViewRowSeparatorFunc ) view_sep_func, NULL, NULL );
        gtk_tree_view_set_headers_visible (treeview, false); 
}

//-----------------------------------------------------------------------------

void ClassSidePanel::SetCursor(void)
{

}

//-----------------------------------------------------------------------------

int ClassSidePanel::GetPair(void)
{
	return ActivePanel;
}

//-----------------------------------------------------------------------------

void ClickMenuDeleteBookmark(GtkWidget *menuitem, gpointer userdata)
{

	int mime;
	gchar * fullname;
	gchar * name;		
    side_panel.GetSelectedItem(&fullname, &mime, &name);
	if(fullname)
	{
//    	ClassString link = g_build_filename(g_get_home_dir(),".config/billfm/bookmark",name, NULL );	
    	ClassString link=g_build_filename(config_path,"bookmark",name,NULL);
		unlink(link.s);
		g_free(fullname);
		g_free(name);			
   		side_panel.LoadDevice();		
	}	
}


//-----------------------------------------------------------------------------

void view_popup_menu_clear_trash(GtkWidget *menuitem, gpointer userdata)
{
    gint ret;
	GtkWindow* parent_win = NULL;
	GtkWidget * dlg = gtk_message_dialog_new( parent_win,
                                      GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_YES_NO,
                                      "Really delete files in trash ?");
        gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_YES); //MOD
        ret = gtk_dialog_run( GTK_DIALOG( dlg ) );
        gtk_widget_destroy( dlg );
        if ( ret != GTK_RESPONSE_YES ) return ;
    int mode=(int)(long int)userdata;
	if(mode==0) UtilsClearTrash(); else
	if(mode==1)
	{	
		ExternalClearTrash(g_get_home_dir());
	}
}
 
//-----------------------------------------------------------------------------

void view_popup_menu_trash(GdkEventButton *event)
{
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label("Clear trash");
 	g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_clear_trash, (gpointer)0);
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	if(getuid())
	{	
	menuitem = gtk_menu_item_new_with_label("Sudo clear trash");
 	g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_clear_trash, (gpointer)1);
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
	}
	gtk_widget_show_all(menu);
 	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL, (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}

//-----------------------------------------------------------------------------

void ClickMenuConnectDropbox(GtkWidget *menuitem, gpointer userdata)
{
	ConnectDropbox();
}

void ClickMenuStartDropbox(GtkWidget *menuitem, gpointer userdata)
{
	system("dropbox start &");
}

void ClickMenuStopDropbox(GtkWidget *menuitem, gpointer userdata)
{
	system("dropbox stop &");
}

void ShowMenuDropbox(GdkEventButton *event)
{
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();

	menuitem = gtk_menu_item_new_with_label("Connect Dropbox");
	g_signal_connect(menuitem, "activate", (GCallback)ClickMenuConnectDropbox, NULL);
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
 	gtk_widget_show_all(menu);
 	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,(event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));

	menuitem = gtk_menu_item_new_with_label("Start Dropbox");
	g_signal_connect(menuitem, "activate", (GCallback)ClickMenuStartDropbox, NULL);
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
 	gtk_widget_show_all(menu);
 	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,(event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));

	menuitem = gtk_menu_item_new_with_label("Stop Dropbox");
	g_signal_connect(menuitem, "activate", (GCallback)ClickMenuStopDropbox, NULL);
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
 	gtk_widget_show_all(menu);
 	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,(event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
	
}

//-----------------------------------------------------------------------------

void ShowMenuBookmark(GdkEventButton *event)
{
	GtkWidget *menu, *menuitem;
	menu = gtk_menu_new();
	menuitem = gtk_menu_item_new_with_label("Удалить закладку");
	g_signal_connect(menuitem, "activate", (GCallback) ClickMenuDeleteBookmark, NULL);
 	gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
 	gtk_widget_show_all(menu);
 	gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,(event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}

//----------------------------------------------------------------------------- 

static int sel(const struct dirent * d)
{
  return 1; // всегда подтверждаем
}

void add_separator(GtkListStore * store)
{
	GtkTreeIter   iter;
	gtk_list_store_append(store, &iter );
	gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "separator",	-1 );
	gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME, "separator" ,	-1 );
	gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME, ICON_GNOME_FOLDER ,	-1 );			
}

//----------------------------------------------------------------------------- 

void ClassSidePanel::ScanBookmark(GtkListStore * store)
{
	GtkTreeIter   iter;

   	ClassString book=g_build_filename(config_path,"bookmark",NULL);
	struct dirent ** entry;
	int n = scandir(book.s, &entry, sel, alphasort);
	if (n < 0) 
	{
		printf("Error read directory bookmark\n");
		return;
	  }

	for (int i = 0; i < n; i++)
	{
		if(entry[i]->d_name[0]=='.') continue;

		gtk_list_store_append( store, &iter );
		gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, entry[i]->d_name,	-1 );
		ClassString dest = g_build_filename(book.s, entry[i]->d_name, NULL );	
		ClassString link=g_file_read_link(dest.s,NULL);

		int mime_type=ICON_GNOME_FOLDER;
		struct stat file_stat;
		if(!lstat( link.s, &file_stat ) ) 
		{
			mime_type=ICON_GNOME_FOLDER;
		}	else
		{
			mime_type=ICON_GNOME_BREAK_LINK;
		}
        if(!strcmp(entry[i]->d_name,"applications")) mime_type=ICON_GNOME_APPLICATION;
		gtk_list_store_set(store,&iter,MODEL_TEXT_FULL_NAME,link.s,-1);
		gtk_list_store_set(store,&iter,MODEL_INT_COL_MIME,mime_type,-1);			
	}
	
}

//----------------------------------------------------------------------------- 

void ClassSidePanel::LoadDevice(void)
{
	GtkTreeIter   iter;
	GtkListStore * store = CreateNewModel();
  
	gtk_list_store_append( store, &iter );
	gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "Home",	-1 );
	gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME, g_get_home_dir() ,	-1 );
	gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,ICON_GNOME_FS_HOME,-1);			

	gtk_list_store_append( store, &iter );
	gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "Desktop",	-1 );
   	ClassString str = g_build_filename(g_get_home_dir(),"Рабочий стол", NULL );	
	gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME,str.s,-1);
	gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,ICON_GNOME_FS_DESKTOP,-1);			

	if(DropboxFolder)
	{	
		gtk_list_store_append( store, &iter );
		gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "Dropbox",	-1 );
		gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME, DropboxFolder ,	-1 );
		gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,ICON_GNOME_DROPBOX,-1);
	}


   	ClassString book=g_build_filename(config_path,"bookmark",NULL);
	gtk_list_store_append( store, &iter );
	gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "Bookmark",	-1 );
	gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME, book.s ,	-1 );
	gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,ICON_GNOME_BOOKMARK,-1);			


   	ClassString appdir=g_build_filename(config_path,"menu",NULL);
	gtk_list_store_append( store, &iter );
	gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "Программы",	-1 );
	gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME,appdir.s,	-1 );
	gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,ICON_GNOME_APPLICATION,-1);			

	ClassString net_path=GetNetworkPath();
	if(net_path.s)
	{	
		gtk_list_store_append( store, &iter );
		gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, "Network",	-1 );
		gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME,net_path.s,	-1 );
		gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,ICON_GNOME_NETWORK,-1);
	}	

	

	add_separator(store);
	ScanDiskByLabel(store);	
	ScanGvfs(store);
	add_separator(store);
	ScanTrash(store);	
	add_separator(store);
    ScanBookmark(store);
	SetModel(GTK_TREE_MODEL( store ));
}


//-----------------------------------------------------------------------------

static void SelectionChangeSideView (GtkTreeView *tree_view, gpointer userdata)
{
	ClassPanel * panel = (ClassPanel*)userdata;
	ClassPanel * target = (ClassPanel *) Panels[ActivePanel];
	
	ClassString name=panel->GetSelectedItem();
	if(!name.s) return;

	SidePanelSelectDropbox=!strcmp(name.s,"Dropbox");
	InfoDisk * info=Mount2Info(name.s);
//	int full=GetFreeSpace(name.s);
//	ClassString str;
//	ClassString dev=Mount2Dev(name.s);
	if(info)
	{
		ClassString total_size=showfilesize(info->total_size);
		ClassString free_size=showfilesize(info->free_size);			

		ClassString str=g_strdup_printf("%s,  mount %s,  used space %d%s (total %s, free %s)",
		                    info->path, name.s, info->full,"%",total_size.s,free_size.s);
		gtk_statusbar_push (StatusBar,0,str.s);
	}	else 	gtk_statusbar_push (StatusBar,0,name.s);

	if( strcmp( name.s, target->get_path()) || target->OkSearchFlag)
	{
		struct stat file_stat;
		if(lstat(name.s, &file_stat)) return;

		if(S_ISDIR(file_stat.st_mode))
		{
 			target->LoadDir(name.s);	
		}	
		return;
	} 	
}

//----------------------------------------------------------------------------- 

gboolean  OnButtonPressedSidePanel (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{
	SelectedSidePanel=1;
	ClassSidePanel * panel = (ClassSidePanel*)userdata;
	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		int mime;
		gchar * fullname;
		gchar * name;		
        panel->GetSelectedItem(&fullname, &mime, &name);
		if(fullname)
		{
//  			printf("mime_type=%d\n",mime);
			mime&=0xFF;

			if( (mime==ICON_GNOME_FS_EMPTY_TRASH) || (mime==ICON_GNOME_FS_FULL_TRASH))	
				view_popup_menu_trash(event);
			if(mime==ICON_GNOME_DROPBOX) ShowMenuDropbox(event);
			if(mime==ICON_GNOME_FOLDER) ShowMenuBookmark(event);
			g_free(fullname);
			g_free(name);			
		}	
    }
    return FALSE;
}

//----------------------------------------------------------------------------- 

void ClassSidePanel::ScanDiskByLabel(GtkListStore * store)
{
	ClassString uid=g_strdup_printf("%d",geteuid());
	InitListDisk(g_get_home_dir(),uid.s);

	GtkTreeIter   iter;
	InfoDisk * info;

	GList* l=GetListDisk();

	while (l!= NULL)
	{
		info=(InfoDisk *)l->data;
		if(!info) continue;
		gtk_list_store_append( store, &iter );
		ClassString label;
		if(info->label) label=g_strdup(info->label); else label=g_strdup(info->mount);

		ClassString net_path=GetNetworkPath();
		int mime_type;
		if(net_path.s && !strncmp(info->mount,net_path.s,strlen(net_path.s)))
		{
			mime_type=ICON_GNOME_SMB_SHARE;
			label=g_strdup(&label.s[strlen(net_path.s)+1]);
		} else mime_type=GetDeviceType(info->path);

		gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, label.s,	-1 );
		gtk_list_store_set(store,&iter,MODEL_TEXT_FULL_NAME,info->mount,-1);
		gtk_list_store_set(store,&iter,MODEL_INT_COL_MIME,mime_type,-1);			
		l = (GList*)g_slist_next(l);
	}
}

//----------------------------------------------------------------------------- 

void ClassSidePanel::ScanTrash(GtkListStore * store)
{
	GtkTreeIter   iter;
	InfoDisk * info;
	GList* list=g_list_first (List_disk);

	for(;list!= NULL; list = (GList*)g_slist_next(list))
	{
		info=(InfoDisk *)list->data;
		if(!info->path_trash) continue;

		gtk_list_store_append( store, &iter );
		ClassString name=g_strdup(info->path_trash);
		ClassString home_trash=g_build_filename(g_get_home_dir(),".local/share/Trash",NULL);

		if(!strcmp(name.s,home_trash.s)) name="Trash Home";  else
		{
			char * p=strstr(name.s,".Trash");
			if(p) *(p--)=0;
			name=g_path_get_basename(name.s);
			name=g_strdup_printf("Trash %s",name.s);
		}
		   
		gtk_list_store_set( store, &iter,  MODEL_TEXT_NAME, name.s,	-1 );
		ClassString trash=g_build_filename(info->path_trash, "files", NULL );	
		gtk_list_store_set( store, &iter, MODEL_TEXT_FULL_NAME, trash.s ,	-1 );

        int icon_trash=ICON_GNOME_FS_EMPTY_TRASH;
		if(IsEmptyDir(trash.s)) icon_trash=ICON_GNOME_FS_FULL_TRASH;
		gtk_list_store_set( store, &iter, MODEL_INT_COL_MIME,icon_trash,-1);			
		
	}
}

//----------------------------------------------------------------------------- 

void ClassSidePanel::ScanGvfs(GtkListStore * store)
{
	GtkTreeIter   iter;

	ClassString dir = g_build_filename(g_get_home_dir(),".gvfs", NULL );	

	struct dirent ** entry;
	int n = scandir(dir.s, &entry, sel, alphasort);
	if (n < 0) 
	{
		printf("Not read %s\n",dir.s);
		return;
	}

	for (int i = 0; i < n; i++)
	{
		if(entry[i]->d_name[0]=='.') continue;
		gtk_list_store_append( store, &iter );
		gtk_list_store_set( store, &iter, MODEL_TEXT_NAME, entry[i]->d_name,	-1 );
		ClassString dest = g_build_filename(dir.s, entry[i]->d_name, NULL );	
		int mime_type=ICON_GNOME_REMOVABLE;
		gtk_list_store_set(store,&iter,MODEL_TEXT_FULL_NAME,dest.s,-1);
		gtk_list_store_set(store,&iter,MODEL_INT_COL_MIME,mime_type,-1);			
	}

}

//----------------------------------------------------------------------------- 