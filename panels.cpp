#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>


#include "tree.h"
#include "disks.h"
#include "utils.h"
#include "fsmonitor.h"

static int sel(const struct dirent * d);

//------------------------------------------------------------------------------

ClassPanel::ClassPanel()
{
	OnlyHiddenList=0;
	OnlyBlackList=0;
	SavePath=0;
	ShowHidden=1;
	InDropboxFlag=0;
	list_black_files=NULL;
	SortType = TYPE_SORT_NAME;
	treeview = 0;
	MyIndex =0;
	MyPath =0;
	MyMode = 0;
	but_path = 0;
	file_list = NULL;
	paste_list = NULL;
	history = NULL;
    ColumnsName[0] = g_strdup("icon");
	ColumnsName[1] = g_strdup("name");
	ColumnsName[2] = g_strdup("ext");
	ColumnsName[3] = g_strdup("right");
   	ColumnsName[4] = g_strdup("size");
   	ColumnsName[5] = g_strdup("datetime");
}

//-----------------------------------------------------------------------------

int ClassPanel::GetSelectedCount(void)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection( treeview );
	return gtk_tree_selection_count_selected_rows(selection);
}

//-----------------------------------------------------------------------------

int ClassPanel::GetSelectedItem(gchar ** fullname, int * mime, gchar ** name)
{
	*fullname=0;
	*mime=0;
	*name=0;
	GtkTreeIter  iter;
	GtkTreeModel * model;
	GList * row = get_selected(&model);
	if(!row) return 0;
	GtkTreePath* tp = (GtkTreePath*)row->data;	
	gtk_tree_model_get_iter(model,&iter,tp);
	gtk_tree_model_get(model,&iter,MODEL_TEXT_FULL_NAME,fullname, 
	                               MODEL_INT_COL_MIME, mime, 
				                   MODEL_TEXT_NAME,name, 
	                   -1);
	gtk_tree_path_free(tp);
	g_list_free(row);
	return 1;
}

//-----------------------------------------------------------------------------

gchar* ClassPanel::GetSelectedItem(void)
{
	GtkTreeIter  iter;
	GtkTreeModel * model;
	GList * row = get_selected(&model);
	if(!row) return NULL;
	GtkTreePath* tp = (GtkTreePath*)row->data;	
	gchar * fullname;
	gtk_tree_model_get_iter(model,&iter,tp);
	gtk_tree_model_get(model,&iter,MODEL_TEXT_FULL_NAME,&fullname,-1);
	gtk_tree_path_free(tp);
	g_list_free(row);
	return fullname;
}

//-----------------------------------------------------------------------------

gchar* ClassPanel::GetSelectedFile(void)
{
	gchar *name=GetSelectedItem();
	if(name)
	{
		struct stat file_stat;
		if(!lstat( name, &file_stat ) ) 
		{
			if(S_ISDIR(file_stat.st_mode))
			{
				g_free(name);
				return NULL;
			} else	return name;
		}
	}
	return	NULL;
}

//-----------------------------------------------------------------------------

gchar* ClassPanel::GetSelectedDir(void)
{
	gchar *name=GetSelectedItem();
	if(name)
	{
		struct stat file_stat;
		if(!lstat( name, &file_stat ) ) 
		{
			if(!S_ISDIR(file_stat.st_mode))
			{
				g_free(name);
				return NULL;
			} else
			{	
				ClassString short_name = g_path_get_basename(name);
				if(!strcmp(short_name.s,"..")) return NULL;
				return name;
			}	
		}
	}
	return	NULL;
}

//-----------------------------------------------------------------------------

GList * ClassPanel::GetSelectedFiles(void)
{
	if(file_list)
	{
	        g_list_foreach( file_list, ( GFunc ) g_free, NULL );
	        g_list_free( file_list );
	        file_list = NULL;
	}

	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GList *row, *rows;
	gchar* name=NULL;

	rows = get_selected(&model);

	for( row = rows; row; row = row->next )
	{
		GtkTreePath* tp = (GtkTreePath*)row->data;

		if( gtk_tree_model_get_iter( model, &iter, tp ) )
		{
			name=NULL;
			gtk_tree_model_get(model, &iter, MODEL_TEXT_FULL_NAME, &name, -1);
			if(name)
			{
				gchar * shortname=g_path_get_basename( name );
				if(strcmp(shortname,"..")) file_list = g_list_prepend( file_list, name );
				g_free(shortname);
			}	
			
		}
		gtk_tree_path_free( tp );
	}
	g_list_free( rows );
	return file_list;
}

//-----------------------------------------------------------------------------

int ClassPanel::GetPair(void)
{
	return (1-MyIndex)&1;
}

//-----------------------------------------------------------------------------

void ClassPanel::BackPanel(void)
{
	gchar * tag = g_path_get_basename(MyPath);
	GList * list=g_list_first(history);
	if(!list) return;
	history=g_list_remove(history,list->data);

	list=g_list_first(history);	
	if(!list) return;
	gchar * pathname=g_strdup((char*)list->data);
	history=g_list_remove(history,list->data);
	LoadDir(pathname);
	g_free(pathname);

	SetCursor((const char *)tag);
	g_free (tag);
}

//-----------------------------------------------------------------------------

void ClassPanel::reload(void)
{
	ClassString path=g_strdup(MyPath);
	LoadDir(path.s);
}

//-----------------------------------------------------------------------------

const char * ClassPanel::get_path(void)
{
	return MyPath;
}

//-----------------------------------------------------------------------------

GList * ClassPanel::get_selected(GtkTreeModel** model)
{
	GtkTreeSelection* selection = gtk_tree_view_get_selection( treeview );
	return gtk_tree_selection_get_selected_rows( selection, model );
}

//-----------------------------------------------------------------------------

GtkTreeModel * ClassPanel::GetModel(void)
{
	if(treeview) return gtk_tree_view_get_model(treeview);
	 else return 0;
}

//-----------------------------------------------------------------------------

void ClassPanel::SetModel(GtkTreeModel * model)
{
	GtkTreeModel * old_model=GetModel();
	if(old_model) g_object_unref( G_OBJECT( old_model ) ); //со старой моделью надо что-то делать

	if(model)
	{ 
		gtk_tree_view_set_model(treeview, model);
	}
//	OnlyHiddenList=0;
//	OnlyBlackList=0;
}

//-----------------------------------------------------------------------------

GtkWidget* ClassPanel::GetWidget(void)
{
	return (GtkWidget*)treeview;
}

//-----------------------------------------------------------------------------

void ClassPanel::SetVisibleColumn(int i, int val)
{
	if(treeview)
	{
		GtkTreeViewColumn * col = gtk_tree_view_get_column(treeview, i );// +1 skip ICON
		if(col)	gtk_tree_view_column_set_visible (col,val);
	}
}

//-----------------------------------------------------------------------------

void ClassPanel::SetColumnTitle(int i, char * name)
{
	GtkTreeViewColumn * col = gtk_tree_view_get_column(treeview, i +1 );// +1 skip ICON
	if(col)
	{
	 	gtk_tree_view_column_set_title( col, name);
	}
}

//-----------------------------------------------------------------------------

GtkListStore * ClassPanel::CreateNewModel()
{
	return gtk_list_store_new( N_COLS, GDK_TYPE_PIXBUF, 
					G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING,G_TYPE_STRING, 
	                G_TYPE_INT,G_TYPE_INT,G_TYPE_INT,G_TYPE_INT,G_TYPE_INT);  //ADD_ITEM
//icon 
//	name right size datetime
}

//-----------------------------------------------------------------------------

void ClassPanel::SetMyPath(const char * path_folder)
{
	if(but_path) gtk_button_set_label(but_path, path_folder);
	if(MyPath) g_free(MyPath);
	MyPath=g_strdup(path_folder);
}

//-----------------------------------------------------------------------------

static int sel(const struct dirent * d)
{
	if(!strcmp(d->d_name,"..")) return 0;
	if(!strcmp(d->d_name,".")) return 0;
	return 1;
}

//-----------------------------------------------------------------------------

int ClassPanel::IsBlackFile(const char * name, const char * ext)
{ 
	if(!OnlyHiddenList)
	{	
		if(ShowHidden && name[0]=='.') return 1;
	} else
	{
		if(name[0]!='.') return 1;
	}

	int res=0;

	if(ext)
	{	
		GList* list=g_list_first(list_black_files);
		while(list)
		{
			const char * str=(gchar *)list->data;
			if(str && !strcmp(str,ext))
			{
				res=1;
				break;
			}
    	   list = (GList*)g_slist_next(list);
		}
	}	
 return res-OnlyBlackList;
}

//-----------------------------------------------------------------------------

void ClassPanel::LoadBlackFiles()
{
	ClearListStrings(&list_black_files);
	ClassString setfile = g_strdup_printf("%s/.billfm.dir",MyPath);

	ClassString net_path=GetNetworkPath();
	if(net_path.s && !strncmp(net_path.s,MyPath,strlen(net_path.s))) return;

	struct stat filestat;

	if(!lstat(setfile.s,&filestat)) 
	{
		FILE * f = fopen (setfile.s,"rt");
		if(!f) return;
		char str[256];
		while(fgets(str,255,f))
		{
			str[strlen(str)-1]=0;
			list_black_files=g_list_append(list_black_files,g_strdup(str));
		}
		fclose(f);
	}	

}

//-----------------------------------------------------------------------------

gboolean ClassPanel::FindIter(const char * tagname, GtkTreeIter * iter, int col)
{
	GtkTreeModel* model=GetModel();

	gboolean valid = gtk_tree_model_get_iter_first (model, iter);

	while (valid)
    {
		gchar *name=NULL;
		gtk_tree_model_get (model, iter,col, &name, -1);
		if(name)
		{
			if(!strcmp(name,tagname))
			{
				g_free (name);	
				return 1;
			}
			g_free (name);
		}
      valid = gtk_tree_model_iter_next (model, iter);
    }
	return 0;
}

//-----------------------------------------------------------------------------

void PanelInsertItem(int num,const char * fullname)
{
	Panels[num]->InsertItem(fullname);
}

//-----------------------------------------------------------------------------

void PanelDeleteItem(int num,const char * fullname)
{
	Panels[num]->DeleteItem(fullname);
}

//-----------------------------------------------------------------------------

void PanelReload(int num)
{
	Panels[num]->reload();
}

//------------------------------------------------------------------------------

GList * PanelGetSelected(void)
{
  return Panels[ActivePanel]->GetSelectedFiles();
}

//------------------------------------------------------------------------------

const char * PanelGetDestDir(void)
{
  return Panels[1-ActivePanel]->MyPath;
}

//-----------------------------------------------------------------------------

int PanelUpdateItemDropbox(int num,gchar * tagname)
{
	int res=1;
	ClassPanel * p = Panels[num];
	if(!p->InDropboxFlag) return res;

	GtkTreeIter iter;

	if(p->FindIter((const char*)tagname,&iter,MODEL_TEXT_FULL_NAME))
	{
		GtkTreeModel* model=p->GetModel();
		int mime_type;
		gtk_tree_model_get(model,&iter,MODEL_INT_COL_MIME,&mime_type,-1);		
		mime_type&=~(MIME_FLAG_DROPBOX_OK | MIME_FLAG_DROPBOX_SYNC);
		mime_type|=res=GetDropboxStatusFile(tagname);
		if(!(mime_type & MIME_FLAG_DROPBOX_OK)) res=0;
		gtk_list_store_set((GtkListStore*)model, &iter, MODEL_INT_COL_MIME,mime_type,-1);
	}   

	return res;
}

//-----------------------------------------------------------------------------

int ClassPanel::InTrash(const char * name)
{
	return 0;
}

//-----------------------------------------------------------------------------

void ClassPanel::OnButtonEditCell(void)
{
	ClassString source=GetSelectedItem();	
	if(!source.s) return;

	ClassString dest=EditFileName(source.s);
	if(!dest.s)  return;

	InfoOperation fo;
	fo.func=TASK_MOVE;
	fo.source_delete=SOURCE_DELETE;
	if(FileMoveFunc(source.s,dest.s,&fo)) return;
}

//-----------------------------------------------------------------------------

void ClassPanel::LoadDir(const char * fullname)
{
	OkSearchFlag=0;
	if(!strcmp(fullname,"/usr/bin")) ScanUsrbin();

	TypeFS = GetTypeFS(fullname);	
//	if(!strncmp(fullname,PATH_BILLFM_MENU,strlen(PATH_BILLFM_MENU)))
	if(!InMenuPath(fullname))
	{	
		LoadMenu(fullname);
	} else
	{
		__LoadDir(fullname);
	}	
	FsMonitorChangePath(MyIndex,fullname,InDropboxFlag); 
	SetSort(SortType);
	SetCursor();

	DrawPathButton(fullname,SavePath);
	SavePath=0;
	chdir(MyPath);
}

//-----------------------------------------------------------------------------

void ClassPanel::LoadMenu(const char * source)
{
	struct dirent ** entry;
	int n = scandir(source, &entry, sel, alphasort);

	if (n < 0) 
	{
		ClassString mes = g_strdup_printf("Error scan dir '%s'.\n%s",source,strerror(errno));
		ShowCancel(mes.s);
		return;
	}
		
	SetVisibleColumn(VIEW_COL_EXT,0);
	SetVisibleColumn(VIEW_COL_RIGHT,0);
	SetVisibleColumn(VIEW_COL_SIZE,0);
	SetVisibleColumn(VIEW_COL_DATETIME,0);
	SetVisibleColumn(VIEW_COL_NAME,1);

	history = g_list_prepend( history, (void*)g_strdup(source) );	
	SetMyPath(source);

	InDropboxFlag=0;

	LoadBlackFiles();
	 
	GtkListStore *store = CreateNewModel();
	for(int i = 0; i < n; i++)
	{
		ClassString fullname = g_build_filename("/",MyPath,entry[i]->d_name,NULL);
		GtkTreeIter   _iter;
		GtkTreeIter  * iter=&_iter;	

		ClassString name=g_path_get_basename(fullname.s);
		ClassString ext=GetExt(name.s);
	
		gtk_list_store_append(store,iter);

		ClassString strSize;
		ClassString temp;
		struct stat filestat;
		int is_dir;
		if(!lstat(fullname.s,&filestat)) 
		{
			if(S_ISDIR(filestat.st_mode))
			{
				is_dir=1;
			} else
			{
				is_dir=0;
				if(strcmp(ext.s,"desktop")) return;
			}
		} else return;

		gchar * item=GetDesktopKey(fullname.s,"Name");
		if(!item) item=name.s;
		gtk_list_store_set( store, iter,MODEL_TEXT_NAME, item, -1);
		gtk_list_store_set(store,iter,MODEL_TEXT_COL_EXT,ext.s,-1);
		gtk_list_store_set( store, iter, MODEL_TEXT_FULL_NAME, fullname.s,	-1 );
		int mime_type=0;
		mime_type=GetMime(MyPath,fullname.s);
		gtk_list_store_set(store,iter,MODEL_INT_COL_MIME, mime_type,-1);
		gtk_list_store_set(store,iter,MODEL_INT_COL_ISDIR, is_dir,	-1);
	}

	SetModel((GtkTreeModel*)store);
	SortType=TYPE_SORT_NAME;
}

//-----------------------------------------------------------------------------

void ClassPanel::__LoadDir(const char * source)
{
	struct dirent ** entry;
	int n = scandir(source, &entry, sel, alphasort);

	if (n < 0) 
	{
		ClassString mes = g_strdup_printf("Error scan dir '%s'.\n%s",source,strerror(errno));
		ShowCancel(mes.s);
		return;
	}
		
	SetVisibleColumn(VIEW_COL_NAME,1);
	SetVisibleColumn(VIEW_COL_EXT,1);
	SetVisibleColumn(VIEW_COL_SIZE,1);
	SetVisibleColumn(VIEW_COL_DATETIME,1);
	SetVisibleColumn(VIEW_COL_RIGHT,(TypeFS!=TYPE_FS_NTFS));

	history = g_list_prepend( history, (void*)g_strdup(source) );
	SetMyPath(source);

	InDropboxFlag=InDropbox(MyIndex,MyPath);

	LoadBlackFiles();
	 
	GtkListStore *store = CreateNewModel();
	for(int i = 0; i < n; i++)
	{
		ClassString fullname = g_build_filename("/",MyPath,entry[i]->d_name,NULL);
		SetPropertyItem(store,fullname.s);
	}

	SetModel((GtkTreeModel*)store);
}

//-----------------------------------------------------------------------------

int ClassPanel::SetPropertyItem(GtkListStore *store, const char * fullname)
{
	GtkTreeIter   _iter;
	GtkTreeIter  * iter=&_iter;	

	ClassString name=g_path_get_basename(fullname);
	ClassString ext=GetExt(name.s);

	
	if(IsBlackFile(name.s,ext.s)) return 1;

	gtk_list_store_append(store,iter);

	ClassString strSize;
	ClassString temp;
	struct stat filestat;
	int is_dir;
	int full_size;
	if(!lstat(fullname,&filestat)) 
	{
		if(S_ISDIR(filestat.st_mode))
		{
			is_dir=1;
			if(SizeDirShow)
			{
				full_size=GetSizeDir(fullname);
				strSize=g_strdup_printf("%ld",(long int)full_size);				
			} else
			{	
				strSize="<dir>";
				full_size=filestat.st_size;
			}
		} else
		{
			is_dir=0;
			full_size=filestat.st_size;
			strSize=g_strdup_printf("%ld",(long int)filestat.st_size);
		}
	}//а если нет статуса ?

	gtk_list_store_set( store, iter,MODEL_TEXT_NAME, name.s, -1);

	gtk_list_store_set(store,iter,MODEL_TEXT_COL_EXT,ext.s,-1);

	temp =get_file_rigth_string(filestat.st_mode);
	gtk_list_store_set(store,iter,MODEL_TEXT_COL_RIGHT,temp.s,-1);
			
	gtk_list_store_set( store, iter, MODEL_TEXT_COL_SIZE,strSize.s,-1);

	temp=get_file_mtime_string(filestat.st_mtime);
	gtk_list_store_set(store,iter,MODEL_TEXT_COL_DATETIME,temp.s,-1);		

	gtk_list_store_set( store, iter, MODEL_TEXT_FULL_NAME, fullname,	-1 );

	int mime_type=GetMime(MyPath,fullname);

	gtk_list_store_set(store,iter,MODEL_INT_COL_MIME, mime_type,-1);

	gtk_list_store_set(store,iter,MODEL_INT_COL_ISDIR, is_dir,	-1);
	gtk_list_store_set(store,iter,MODEL_INT_COL_SIZE,full_size,-1);
	gtk_list_store_set(store,iter,MODEL_INT_COL_DATETIME,(int)filestat.st_mtime ,-1);

	InsertInListDropbox(MyIndex,fullname);
	return 0;
}

//-----------------------------------------------------------------------------

void ClassPanel::InsertItem(const char * fullname)
{
	GtkTreeIter iter;

	if(InDropboxFlag) InsertInListDropbox(MyIndex,fullname);
	
	if(FindIter((gchar*)fullname,&iter,MODEL_TEXT_FULL_NAME))
	{
		return;
	}   
	GtkListStore *store=(GtkListStore *)GetModel();
	SetPropertyItem(store,fullname);
}

//-----------------------------------------------------------------------------

void ClassPanel::DeleteItem(const char * tagname)
{
	GtkTreeIter iter;
	GtkTreeModel* model=GetModel();
	if(FindIter(tagname,&iter,MODEL_TEXT_FULL_NAME))
	{
		gtk_list_store_remove((GtkListStore*)model, &iter);
	}   
}

//-----------------------------------------------------------------------------

void ClassPanel::InfoSelectedInStatusBar(void)
{
	int n=GetSelectedCount();
	if(n==1) InfoSingleInStatusBar();	else
	if(n>1)	InfoMultiInStatusBar();
}

//-----------------------------------------------------------------------------

void ClassPanel::InfoMultiInStatusBar(void)
{
	int n=GetSelectedCount();
	ClassString mes= g_strdup_printf("%d selected",n);
/*	GList * l = GetSelectedFiles();
	if(l) 
	{	
     ClassString mes=InfoDir(l);
     gtk_statusbar_push (StatusBar,0, mes.s);
	}			
*/
}

//-----------------------------------------------------------------------------
void  ClassPanel::ItemDoubleClick( const char * fullname )
{
}

void  ClassPanel::OnDoubleClick( GtkTreePath * _path )
{
	ClassString name=GetSelectedItem();

	const char * fullname=name.s;
	if(!fullname) return;

	struct stat file_stat;
	if(lstat(fullname, &file_stat)) return;

	if(S_ISDIR(file_stat.st_mode))
	{
		LoadDir(fullname);
		return;
	} 	

	if(S_ISLNK(file_stat.st_mode))
	{
		ClassString link=Link2File(fullname);

		if(lstat(link.s, &file_stat)) return;

		if(S_ISDIR(file_stat.st_mode))
		{
			LoadDir(fullname);
		}
		{
			ClickExecFile(link.s,&file_stat);
		}	
		return;
	}

	ClickExecFile(fullname,&file_stat);
}


//-----------------------------------------------------------------------------

void ClassPanel::InfoSingleInStatusBar(void)
{
	ClassString name = GetSelectedItem();
    if(!name.s)
	{
		gtk_statusbar_push (StatusBar,0,"");
		return;
	}	
	ClassString mes;
	struct stat file_stat;
	int res=lstat(name.s, &file_stat);
	if(!res) 
	{
		ClassString archve = GetArchivePath();
		if(!strncmp(name.s,archve.s,strlen(archve.s)))
		{
			ClassString zip=g_strdup(&name.s[strlen(archve.s)]);
			ClassString inzip=g_strdup(zip.s);
			
			struct stat file_stat;
			int inside=0;
			if(lstat(zip.s, &file_stat))
			{	
				inside=1;
				while(lstat(zip.s, &file_stat))
				{
					zip=g_path_get_dirname(zip.s);
				}
				inzip=g_strdup(&inzip.s[strlen(zip.s)+1]);
			} else
			{	
				inside=0;
				inzip=g_strdup("");				
			}	
			
			ClassString mes;
			if(inside)
			{	
				mes	= g_strdup_printf("Archive: %s -> %s",zip.s,inzip.s);
				gtk_statusbar_push (StatusBar,0,mes.s);
				return;
			}	
//			 else mes	= g_strdup_printf("Archive: %s -> %s",zip.s,inzip.s);
		}	
		
		if(S_ISDIR(file_stat.st_mode))
		{
			gtk_statusbar_push (StatusBar,0,name.s);
		} else
		if(S_ISLNK(file_stat.st_mode))
		{	
			ClassString link=g_file_read_link(name.s,NULL);
//			if(link.s[0]!='/')	link = g_build_filename(MyPath,link.s,NULL);
			mes= g_strdup_printf("Link on %s", link.s);

			gtk_statusbar_push (StatusBar,0,mes.s);
		} else
		{
			ClassString show_size=showfilesize(file_stat.st_size);
			mes= g_strdup_printf("%s - file size %ld %s",
					                              name.s,(long int)file_stat.st_size,show_size.s);
			gtk_statusbar_push (StatusBar,0, mes.s);
		}	
	} else 
	{
		mes= g_strdup_printf("%s - lstat error (%d) %s", name.s,errno,strerror(errno));
		gtk_statusbar_push (StatusBar,0, mes.s);
	}


}

//-----------------------------------------------------------------------------

void ClassPanel::SelectBreakLink(void)
{

	GtkTreeIter iter;
	GtkTreeModel * model = GetModel();	

	GtkTreeSelection * tree_sel = gtk_tree_view_get_selection( treeview );
	gboolean valid = gtk_tree_model_get_iter_first(model,&iter);
	while(valid)
	{
		GtkTreePath* path =gtk_tree_model_get_path(model,&iter);
		int mime;
		gtk_tree_model_get(model, &iter, MODEL_INT_COL_MIME, &mime, -1);
		if((mime&MASK_MIME)==ICON_GNOME_BREAK_LINK) 
			gtk_tree_selection_select_path ( tree_sel, path );
		valid=gtk_tree_model_iter_next(model,&iter);		
	}
	InfoSelectedInStatusBar();
}

//-----------------------------------------------------------------------------
