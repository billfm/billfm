#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "tree.h"
#include "utils.h"
#include "icon-cell-renderer.h"

void view_popup_menu_paste (GtkWidget *menuitem, ClassTreePanel * panel);
void view_popup_menu(ClassTreePanel * panel, GdkEventButton *event);

//-----------------------------------------------------------------------------

void view_popup_menu_sort_name (GtkWidget *menuitem,  ClassTreePanel * panel)
{
	panel->SetSort(TYPE_SORT_NAME);
}
 
//-----------------------------------------------------------------------------

void view_popup_menu_sort_ext (GtkWidget *menuitem,  ClassTreePanel * panel)
{
	panel->SetSort(TYPE_SORT_EXT);
}
 
//-----------------------------------------------------------------------------

void view_popup_menu_sort_size (GtkWidget *menuitem,  ClassTreePanel * panel)
{
	panel->SetSort(TYPE_SORT_SIZE);
}
 
//-----------------------------------------------------------------------------

void view_popup_menu_sort_time (GtkWidget *menuitem,  ClassTreePanel * panel)
{
	panel->SetSort(TYPE_SORT_TIME);
}

//----------------------------------------------------------------------------- 

void view_popup_menu_cut (GtkWidget *menuitem, ClassTreePanel * panel)
{
	GList* file_list = panel->GetSelectedFiles();
	ptk_clipboard_cut_or_copy_files( file_list, GDK_ACTION_MOVE );
}

//----------------------------------------------------------------------------- 

void view_popup_menu_copy (GtkWidget *menuitem, ClassTreePanel * panel)
{
	GList* file_list = panel->GetSelectedFiles();
	ptk_clipboard_cut_or_copy_files( file_list , GDK_ACTION_COPY);
}

//----------------------------------------------------------------------------- 

void view_popup_menu_property (GtkWidget *menuitem, ClassTreePanel * panel)
{
	ClassString item = panel->GetSelectedItem();
	DialogFileProperty(item.s);
}

//----------------------------------------------------------------------------- 

void view_popup_menu_infodir(GtkWidget *menuitem, ClassTreePanel * panel)
{
//	ClassString item = panel->GetSelectedDir();
//	if(item.s) 
//	{	
//     ClassString mes=InfoDir(item.s);
//     gtk_statusbar_push (StatusBar,0, mes.s);
//	}			
	GList * l = panel->GetSelectedFiles();
	if(l) 
	{	
     ClassString mes=InfoDir(l);
     gtk_statusbar_push (StatusBar,0, mes.s);
	}			
}

//----------------------------------------------------------------------------- 

void view_popup_menu_eogdir(GtkWidget *menuitem, ClassTreePanel * panel)
{
	ClassString item = panel->GetSelectedDir();
 	if(item.s) 
	{	
		item=g_strdup_printf("eog %s",item.s);
		system(item.s);
	}			
}

//----------------------------------------------------------------------------- 

void view_popup_menu_exec(GtkWidget *menuitem, gchar * com)
{
	system(com);
}

//----------------------------------------------------------------------------- 

static gboolean  OnButtonPressed (GtkWidget *treeview, GdkEventButton *event, gpointer userdata)
{

	if (event->type == GDK_BUTTON_PRESS  &&  event->button == 3)
	{
		g_print ("Single right click on the tree view.\n");
		ClassTreePanel * panel = (ClassSidePanel*)userdata;
		view_popup_menu(panel, event);
		return TRUE; 
	}

	return FALSE;
}


//-----------------------------------------------------------------------------

void  OnDoubleClick_TreeView (GtkTreeView * treeview, GtkTreePath * path, GtkTreeViewColumn  *col, gpointer userdata)
{
	ClassPanel * panel=(ClassTreePanel*)userdata;
	panel->OnDoubleClick( path );
}

void SelectionChangeTreeView (GtkTreeView *tree_view, gpointer userdata)
{
    if(!SelectedSidePanel)
	{	
		ClassPanel * panel=(ClassPanel*)userdata;
		panel->InfoSelectedInStatusBar();
	}	
}

//-----------------------------------------------------------------------------

void ClassTreePanel::CreatePanel(void) 
{
	treeview = (GtkTreeView*)gtk_tree_view_new();

	g_signal_connect((GtkWidget *)treeview, "row-activated", (GCallback) OnDoubleClick_TreeView, this);
	g_signal_connect((GtkWidget *)treeview, "cursor-changed", (GCallback) SelectionChangeTreeView, this );
	g_signal_connect((GtkWidget *)treeview, "button-press-event", (GCallback) OnButtonPressed, this );
	g_signal_connect((GtkWidget *)treeview, "focus-in-event", (GCallback)ClickActivePanel, this );

	create_icon_column( COL_ICON );

	for(int i=VIEW_COL_NAME; i<=VIEW_COL_DATETIME; i++)
	{
		create_text_column(i); 
	}

	gtk_tree_selection_set_mode( gtk_tree_view_get_selection( treeview), GTK_SELECTION_MULTIPLE ) ;
//	gtk_tree_view_set_headers_clickable (treeview, 1);
        gtk_tree_view_set_enable_search (treeview, TRUE);
	gtk_tree_view_set_search_column(treeview,MODEL_TEXT_NAME);

}

//-----------------------------------------------------------------------------

void user_function(GtkTreeViewColumn * col, gpointer user_data)
{
	ClassTreePanel * panel = (ClassTreePanel *)user_data;
	int sort=(int)(long int)g_object_get_data(G_OBJECT(col), "sort_type");	
	if(panel->SortType!=sort) panel->SetSort(sort);
}

//----------------------------------------------------------------------------- 

void ClassTreePanel::create_text_column( gint index_col )
{
	GtkTreeViewColumn *	col = gtk_tree_view_column_new();
//	printf("col[%d]=%s\n",index_col,ColumnsName[index_col]);
	gtk_tree_view_column_set_title( col, ColumnsName[index_col]);
	GtkCellRenderer * renderer = gtk_cell_renderer_text_new();
	gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( col ), renderer, TRUE );
	gtk_tree_view_column_set_attributes( col, renderer, "text", index_col, NULL );
	gtk_tree_view_append_column( treeview, col );
	int sort=TYPE_SORT_NONE;
	if(index_col==VIEW_COL_NAME) sort=TYPE_SORT_NAME; else
	if(index_col==VIEW_COL_EXT) sort=TYPE_SORT_EXT; else		
	if(index_col==VIEW_COL_SIZE) sort=TYPE_SORT_SIZE; else
	if(index_col==VIEW_COL_DATETIME) sort=TYPE_SORT_TIME; 
	g_object_set_data(G_OBJECT(col), "sort_type", (gpointer)sort);
//	gtk_tree_view_column_set_reorderable (col, 1);

	g_signal_connect(col, "clicked", (GCallback) user_function, this);
	gtk_tree_view_column_set_clickable(col,1);
	gtk_tree_view_column_set_resizable(col,1);
}

//-----------------------------------------------------------------------------

void ClassTreePanel::create_icon_column(gint icon_col )
{
	GtkTreeViewColumn * col = gtk_tree_view_column_new();
	GtkCellRenderer   *render;
	

//	render = gtk_cell_renderer_pixbuf_new();
	render = icon_cell_renderer_new();
	gtk_tree_view_column_pack_start (col, render, TRUE);
//	gtk_tree_view_column_add_attribute (col, render, "pixbuf", icon_col);
	gtk_tree_view_column_add_attribute (col, render, "mime_type", MODEL_INT_COL_MIME);
	gtk_tree_view_column_add_attribute (col, render, "file_type", MODEL_INT_COL_FILE_TYPE);	
	gtk_tree_view_append_column( treeview, col );
//	g_object_set_data(G_OBJECT(col), "my_renderer", renderer);
}

//-----------------------------------------------------------------------------

void ClassTreePanel::DeInit(void)
{
	GObject * g = G_OBJECT( GetWidget());
	if(g)
	{
		g_object_disconnect(g, "any_signal::row-activated", G_CALLBACK(OnDoubleClick_TreeView), this, NULL);
		g_object_disconnect(g, "any_signal::cursor-changed", G_CALLBACK(SelectionChangeTreeView), this, NULL);
		g_object_disconnect(g, "any_signal::focus-in-event", (GCallback)ClickActivePanel, this, NULL );
	//	g_object_unref( g);
		SetModel(0);
		gtk_widget_destroy( GetWidget() );
		treeview = 0;
	}
}

//-----------------------------------------------------------------------------

void ClassTreePanel::SelectAll(void)
{
	gtk_tree_selection_select_all(gtk_tree_view_get_selection( treeview));
	InfoSelectedInStatusBar();
}

//-----------------------------------------------------------------------------

void ClassTreePanel::UnSelectAll(void)
{
	gtk_tree_selection_unselect_all(gtk_tree_view_get_selection( treeview));
	InfoSelectedInStatusBar();
}

//-----------------------------------------------------------------------------

static gint SortFuncName(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer TypeSort);
static gint SortFuncSize(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data);
static gint SortFuncExt(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data);
static gint SortFuncDateTime(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data);
static gint SortFuncSizeForDir(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data);

void ClassTreePanel::SetSort(int sort)
{
	if(!((sort>TYPE_SORT_NONE) && (sort<TYPE_SORT_UNKNOW))) return;
	SortType=sort;
	GtkTreeSortable * model = (GtkTreeSortable *)GetModel();
	if(!model) return;
	GtkTreeViewColumn *col;
	int	index_col;
	
	switch(SortType)
	{
		case TYPE_SORT_NAME:
		{
			index_col=MODEL_TEXT_NAME;
			col = gtk_tree_view_get_column(treeview,VIEW_COL_NAME);
			gtk_tree_sortable_set_sort_column_id(model, index_col, GTK_SORT_ASCENDING);
			gtk_tree_view_column_set_sort_column_id (col, index_col);

			gtk_tree_sortable_set_sort_func (model,
		                                 index_col, 
		                                 SortFuncName,
		                                 (gpointer)SortType,
		                                 NULL);

			break;
		}

		case TYPE_SORT_EXT:
		{
			index_col=MODEL_TEXT_NAME;
			col = gtk_tree_view_get_column(treeview,VIEW_COL_EXT);
			gtk_tree_sortable_set_sort_column_id(model, index_col, GTK_SORT_ASCENDING);
			gtk_tree_view_column_set_sort_column_id (col, index_col);

			gtk_tree_sortable_set_sort_func (model,
		                                 index_col,
		                                 SortFuncExt,
		                                 (gpointer)SortType,
		                                 NULL);

			break;
		}

		case TYPE_SORT_TIME:
		{
			index_col=MODEL_TEXT_COL_DATETIME;
			col = gtk_tree_view_get_column(treeview, VIEW_COL_DATETIME );// +1 skip ICON
			gtk_tree_sortable_set_sort_column_id(model, index_col, GTK_SORT_ASCENDING);
			gtk_tree_view_column_set_sort_column_id (col, index_col);
			gtk_tree_sortable_set_sort_func (model,index_col, SortFuncDateTime,NULL,NULL);
			break;
		}

		case TYPE_SORT_SIZE:
		{
			index_col=MODEL_TEXT_COL_SIZE;
			col = gtk_tree_view_get_column(treeview, VIEW_COL_SIZE );// +1 skip ICON
			gtk_tree_sortable_set_sort_column_id(model, index_col, GTK_SORT_ASCENDING);
			gtk_tree_view_column_set_sort_column_id (col, index_col);
			if(SizeDirShow) gtk_tree_sortable_set_sort_func (model,index_col, SortFuncSizeForDir,NULL,NULL);
			 else gtk_tree_sortable_set_sort_func (model,index_col, SortFuncSize,NULL,NULL);

			break;
		}
	}

}

//-----------------------------------------------------------------------------

static gint SortFuncName(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data)
{
	gchar * name1;
	gchar * name2;	
	int dir1,dir2;	

	gtk_tree_model_get(model, a, MODEL_TEXT_NAME, &name1, -1);
	gtk_tree_model_get(model, b, MODEL_TEXT_NAME, &name2, -1);

	gtk_tree_model_get(model, a, MODEL_INT_COL_ISDIR, &dir1, -1);
	gtk_tree_model_get(model, b, MODEL_INT_COL_ISDIR, &dir2, -1);	

	if((dir1==1) && (dir2==1)) return strcoll(name1,name2); else
	if((dir1==0) && (dir2==0)) return strcoll(name1,name2); else
	if((dir1==1) && (dir2==0)) return -1; else	return 1;

	return 0;
}

//-----------------------------------------------------------------------------

int strcoll2(gchar * a,	gchar * b) 
{
	ClassString dir1=GetExt(a);
	ClassString  dir2=GetExt(b);	
//	char * dir1=(char*)strstr(a,".");
//	char * dir2=(char*)strstr(b,".");
	if((dir1.s==0) && (dir2.s==0)) return strcoll(a,b); else
	if((dir1.s!=0) && (dir2.s!=0)) return strcoll(dir1.s,dir2.s); else
	if((dir1.s==0) && (dir2.s!=0)) return -1; else	return 1;
	return 0;
}

//-----------------------------------------------------------------------------

static gint SortFuncExt(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data)
{
	gchar * name1;
	gchar * name2;	
	int dir1,dir2;	

	gtk_tree_model_get(model, a, MODEL_TEXT_NAME, &name1, -1);
	gtk_tree_model_get(model, b, MODEL_TEXT_NAME, &name2, -1);

	gtk_tree_model_get(model, a, MODEL_INT_COL_ISDIR, &dir1, -1);
	gtk_tree_model_get(model, b, MODEL_INT_COL_ISDIR, &dir2, -1);	

	if((dir1==1) && (dir2==1)) return strcoll(name1,name2); else
	if((dir1==0) && (dir2==0)) return strcoll2(name1,name2); else
	if((dir1==1) && (dir2==0)) return -1; else	return 1;

	return 0;
}

//-----------------------------------------------------------------------------

static gint SortFuncSize(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data)
{

	gchar * name1;
	gchar *	name2;	
	int dir1,dir2;	
	long int size1,size2;	

	gtk_tree_model_get(model, a, MODEL_TEXT_COL_SIZE, &name1, -1);
	gtk_tree_model_get(model, b, MODEL_TEXT_COL_SIZE, &name2, -1);

	gtk_tree_model_get(model, a, MODEL_INT_COL_ISDIR, &dir1, -1);
	gtk_tree_model_get(model, b, MODEL_INT_COL_ISDIR, &dir2, -1);	

	if((dir1==1) && (dir2==1)) return strcoll(name1,name2); else
	if((dir1==0) && (dir2==0))
	{
		gtk_tree_model_get(model, a, MODEL_INT_COL_SIZE, &size1, -1);
		gtk_tree_model_get(model, b, MODEL_INT_COL_SIZE, &size2, -1);	
		if(size1<size2) return -1; else
		if(size1>size2) return 1; else return 0;
		return 0;
	} else
	if((dir1==1) && (dir2==0)) return -1; else	return 1;
	return 0;
}

//-----------------------------------------------------------------------------

static gint SortFuncSizeForDir(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data)
{
	int size1,size2;	
	gtk_tree_model_get(model, a, MODEL_INT_COL_SIZE, &size1, -1);
	gtk_tree_model_get(model, b, MODEL_INT_COL_SIZE, &size2, -1);	

	if(size1<size2) return -1; else
	if(size1>size2) return 1; else return 0;
}

//-----------------------------------------------------------------------------

static gint SortFuncDateTime(GtkTreeModel *model, GtkTreeIter  *a,GtkTreeIter  *b, gpointer user_data)
{
	int size1,size2;	
	gtk_tree_model_get(model, a, MODEL_INT_COL_DATETIME, &size1, -1);
	gtk_tree_model_get(model, b, MODEL_INT_COL_DATETIME, &size2, -1);	

	if(size1<size2) return -1; else
	if(size1>size2) return 1; else return 0;
}

//-----------------------------------------------------------------------------

void ClassTreePanel::SetActive(void)
{
	gtk_widget_modify_base ((GtkWidget*)treeview, GTK_STATE_NORMAL, NULL);
}

void ClassTreePanel::SetNotActive(void)
{
	GtkStyle *style;
	GdkColor color;

    style = gtk_widget_get_style ((GtkWidget*)treeview);
//GTK_STATE_INSENSITIVE
//GTK_STATE_SELECTED	
	color = style->base[9];
	gtk_widget_modify_base ((GtkWidget*)treeview, GTK_STATE_NORMAL, &color);
}

//-----------------------------------------------------------------------------

void ClassTreePanel::SetCursor(void)
{
	GtkTreeIter   iter;

	if(gtk_tree_model_get_iter_first ( GetModel(), &iter))
	{
		GtkTreePath *  path =    gtk_tree_model_get_path ( GetModel(), &iter);
		if(path)	gtk_tree_view_set_cursor(treeview, path , NULL, 0 );
	}
}


//-----------------------------------------------------------------------------

void ClassTreePanel::SetCursor(const char * tag)
{
    if(!tag) return;
	GtkTreeIter iter;
	GtkTreeModel* model=GetModel();

	gboolean valid = gtk_tree_model_get_iter_first (model, &iter);

    int exit_flag=0;
	while (valid)
    {
		gchar *name=NULL;
		gtk_tree_model_get (model, &iter, MODEL_TEXT_FULL_NAME, &name, -1);
		if(name)
		{
			gchar * basename = g_path_get_basename(name);
			if(!strcmp(basename,tag))
			{
				GtkTreePath * path = gtk_tree_model_get_path ( model, &iter);
				if(path) gtk_tree_view_set_cursor(treeview, path , NULL, 0 );
				exit_flag=1;
			}
			g_free (basename);
			g_free (name);
		}
	  if(exit_flag) break;
      valid = gtk_tree_model_iter_next (model, &iter);
    }
}

//-----------------------------------------------------------------------------
///usr/share/locale-langpack/ru/LC_MESSAGES/diffutils.mo
void ClassTreePanel::Diff(const char * p1, const char * p2, int flags)
{
#define ONLY_IN   1
#define CHANGE_IN 2	
	
	GtkListStore * store=CreateNewModel();
//	GtkTreeIter   iter;

	FILE *read_fp;
	#define		SIZE_BUF		256 
	char buffer[SIZE_BUF];

	gchar * command = g_strdup_printf("diff -q '%s' '%s'",p1,p2);

	read_fp = popen(command, "r");
    ClassString key1=g_strdup_printf("Only in %s",p1);	
    ClassString key2=g_strdup_printf("Files %s",p1);
    ClassString key3=g_strdup_printf("and %s",p2);		
	
	while(fgets(buffer, SIZE_BUF, read_fp))
	{
		buffer[strlen(buffer)-1]=0;
		if(!strncmp(buffer,key1.s,strlen(key1.s)) && (flags&ONLY_IN))
		{
			char * name=&buffer[strlen(key1.s)+2];
//			gtk_list_store_append( store, &iter );
			ClassString fullname = g_build_filename(p1,name, NULL );	
//			if(SetPropertyItem(store,&iter,fullname.s))
//			{
//				gtk_list_store_remove(store,&iter);
//			}	

			SetPropertyItem(store,fullname.s);
		}

		if(!strncmp(buffer,key2.s,strlen(key2.s)) && (flags&CHANGE_IN))
		{
			char * name=&buffer[strlen(key2.s)+1];
			char * tail=strstr(name,key3.s);
			tail[-1]=0;
//			gtk_list_store_append( store, &iter );
			ClassString fullname = g_build_filename(p1,name, NULL );	
//			if(SetPropertyItem(store,&iter,fullname.s))
//			{
//				gtk_list_store_remove(store,&iter);
//			}	
			SetPropertyItem(store,fullname.s);	
		}

	}

	pclose(read_fp);   
	SetModel(GTK_TREE_MODEL(store));
	OkSearchFlag=1;
}

//-----------------------------------------------------------------------------

void ClassTreePanel::LoadFromFile(const char * source)
{
	SetMyPath("");
	GtkListStore * store=CreateNewModel();
	
	char buffer[1024];//#define		SIZE_BUF		1024

	FILE *  f=fopen(source,"rt");
	if(!f) return;

		while(fgets(buffer, SIZE_BUF, f))
		{
			buffer[strlen(buffer)-1]=0;
			SetPropertyItem(store,buffer);
		}

	pclose(f);   
	SetModel(GTK_TREE_MODEL(store));
	OkSearchFlag=1;
	SetSort(SortType);
}

//----------------------------------------------------------------------------- 

void view_popup_menu_paste (GtkWidget *menuitem, ClassTreePanel * panel)
{
	const char * destdir = Panels[ActivePanel]->get_path();
	int action=GDK_ACTION_COPY;
	GList* l = ptk_clipboard_paste_files(&action);
    if(l) 
	{	
		if(action==GDK_ACTION_COPY)
		{
			ExternalFileCopy4(getuid(),TASK_COPY,l,destdir);
		} else
		if(action==GDK_ACTION_MOVE)
		{
			ExternalFileCopy4(getuid(),TASK_MOVE,l,destdir);		
		}
	}	
}

//----------------------------------------------------------------------------- 

void view_popup_menu(ClassTreePanel * panel, GdkEventButton *event)
{
    GtkWidget *menu, *menuitem;
 
    menu = gtk_menu_new();
	const char* items[] =
    {
		"Copy",
		"Cut",
		"Paste",
		"-",
		"Sort by name",
		"Sort by extention",
		"Sort by size",
		"Sort by time",
		"-",
		"Property",
		"InfoDir",
		"Просмотр изображений",
		0
	};

	GCallback sig[] =
	{
		(GCallback) view_popup_menu_copy,
		(GCallback) view_popup_menu_cut,
		(GCallback) view_popup_menu_paste,
		0,
		(GCallback) view_popup_menu_sort_name,
		(GCallback) view_popup_menu_sort_ext,
		(GCallback) view_popup_menu_sort_size,
		(GCallback) view_popup_menu_sort_time,
		0,
		(GCallback) view_popup_menu_property,
		(GCallback) view_popup_menu_infodir,
		(GCallback) view_popup_menu_eogdir,
		0		
	};

	for(int i=0; items[i]; i++)
	{	
		if(!strcmp("-",items[i]))
		{
		    gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new() ); 
		}	else
		{
			menuitem = gtk_menu_item_new_with_label(items[i]);
			if(menuitem) g_signal_connect((GtkWidget*) menuitem, "activate", sig[i],panel);
		    gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);
		}	
	}	


    const char * name=panel->GetSelectedFile();

	GList * list=GetMimeListCommand(name);

    int flag_sep=0;

	for ( ; list; list = list->next )
	{
		const char * fullname=(const char *)list->data;
		ClassString exe_name=GetDesktopCommand(fullname);
		if(!exe_name.s) continue;
//		printf("%s %s\n",(const char *)list->data,exe_name.s);

		gchar * com;
		char gvfs[]="/usr/lib/gvfs/gvfsd-archive";
		if(!strncmp(exe_name.s,gvfs,strlen(gvfs)))
		{
			com = g_strdup_printf("%s'%s' &",exe_name.s,name);
			exe_name=g_strdup("Монтировать архив");
			printf("%s \n",com);
		}	else 
		{	
			com = g_strdup_printf("%s \"%s\" &",exe_name.s,name);
			exe_name= g_path_get_basename(exe_name.s);
		}	
		
		ClassString icon_name=GetDesktopKey(fullname,"Icon");

		GdkPixbuf * pixbuf=0;
		if(icon_name.s) pixbuf=GetIconByName(icon_name.s,ICON_SIZE);
		
		menuitem = gtk_image_menu_item_new_with_label(exe_name.s);

		if(!pixbuf)
		{
			ClassString name=panel->GetSelectedFile();
			printf("Not application icon %s\n",name.s);
			pixbuf=IconPixbuf[0];
		}	
		GtkWidget * app_img = gtk_image_new_from_pixbuf(pixbuf);
        gtk_image_menu_item_set_image ( GTK_IMAGE_MENU_ITEM(menuitem), app_img );

		g_signal_connect(menuitem, "activate", (GCallback) view_popup_menu_exec, com);

		if(!flag_sep++)   gtk_menu_shell_append(GTK_MENU_SHELL(menu), gtk_separator_menu_item_new() ); 

		gtk_menu_shell_append(GTK_MENU_SHELL(menu), menuitem);

	}
	
    gtk_widget_show_all(menu);
    gtk_menu_popup(GTK_MENU(menu), NULL, NULL, NULL, NULL,
                   (event != NULL) ? event->button : 0,
                   gdk_event_get_time((GdkEvent*)event));
}

//-----------------------------------------------------------------------------

void ClassTreePanel::SelectPattern(const char * pattern)
{
	GPatternSpec *	pspec = g_pattern_spec_new (pattern);  
    GtkTreeIter iter;
	GtkTreeModel * model = GetModel();	

	GtkTreeSelection * tree_sel = gtk_tree_view_get_selection( treeview );
	gboolean valid = gtk_tree_model_get_iter_first(model,&iter);
	while(valid)
	{
		GtkTreePath* path =gtk_tree_model_get_path(model,&iter);
		gchar * name;
		gtk_tree_model_get(model, &iter, MODEL_TEXT_FULL_NAME, &name, -1);
		if (g_pattern_match_string (pspec, name)) 	gtk_tree_selection_select_path ( tree_sel, path );
		g_free(name);
		valid=gtk_tree_model_iter_next(model,&iter);		
	}
	InfoSelectedInStatusBar();
}

//-----------------------------------------------------------------------------