#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include "tree.h"
#include "trash.h"
#include "utils.h"

#define COUNT_FUNCTION_BUTTON 20

int ShowSide;
int ShowOne;

static char * ButtonName[COUNT_FUNCTION_BUTTON];
static char * ButtonLabel[COUNT_FUNCTION_BUTTON];


const char * EditViewDefault[] =
{
		"leafpad",
		"gedit",
		"gksudo leafpad",
		"gksudo gedit",
		0
};

typedef void (OnButtonFunction) ( GtkButton* button, int index_operation );

//-----------------------------------------------------------------------------

void OnButtonBack( GtkButton* button, int data ) 
{
 Panels[ActivePanel]->BackPanel();
}

//-----------------------------------------------------------------------------

void OnMenuSidePanel (GtkObject *object, gpointer user_data)
{
 if(SidePanel)
 {
	ShowSide = 1-ShowSide;
	if(ShowSide)
	{	 
		gtk_widget_show( SidePanel ); 
//		side_panel.load_device();
	}	 else gtk_widget_hide( SidePanel );
 }
}

//-----------------------------------------------------------------------------

void OnMenuOnePanel (GtkObject *object, gpointer user_data)
{
 if(OnePanel)
 {
	ShowOne = 1 - ShowOne;
	if( ShowOne ) gtk_widget_show( OnePanel ); 
	else
	{
		gtk_widget_hide( OnePanel );
		ActivePanel=1;
		
	}
 }

}

//-----------------------------------------------------------------------------

void OnButtonHome( GtkButton* button, int index_operation ) 
{
	const char * path = g_get_home_dir();
	Panels[ActivePanel]->LoadDir(path);
}

//-----------------------------------------------------------------------------

void OnButtonExit( GtkButton* button, int index_operation ) 
{
	SaveSetting();
	gtk_main_quit();
}

//-----------------------------------------------------------------------------

void OnButtonSide( GtkButton* button, int index_operation ) 
{
}

//-----------------------------------------------------------------------------

void CreatePanel(int i, int mode)
{
	ClassPanel* panel;
	if(mode==1) panel = new ClassIconPanel();
	else if(mode==0) panel = new ClassTreePanel(); 
	else return;
	panel->MyIndex=i;
	Panels[i]=panel;
	InitPanel(panel);	
}

void 	OnButtonModeView( GtkButton* button, int index_operation )
{
	ClassPanel* panel = (ClassPanel*) Panels[ActivePanel];
	gchar * path =  g_strdup(panel->MyPath);
	int mode=1-panel->MyMode;
	panel->DeInit();
	g_free(panel);
	
    CreatePanel(ActivePanel,mode);

	Panels[ActivePanel]->LoadDir(path);
	g_free(path);
}


//-----------------------------------------------------------------------------

void	OnButtonHidden( GtkButton* button, int index_operation )
{
	Panels[ActivePanel]->ShowHidden = 1 - 	Panels[ActivePanel]->ShowHidden;
	Panels[ActivePanel]->reload();
}

//-----------------------------------------------------------------------------

void OnButtonSame( GtkButton* button, int index_operation ) 
{
	const char * path = Panels[ActivePanel]->get_path();
	int pair=Panels[ActivePanel]->GetPair();
	Panels[pair]->LoadDir(path);
}

//-----------------------------------------------------------------------------

void OnButtonRun( GtkButton* button, int index_operation ) 
{
	ClassString command=g_strdup("xset dpms 10 10 10");
	system(command.s);
	command=g_strdup("xtrlock");
	system(command.s);
	command=g_strdup("xset dpms 600 600 600");
	system(command.s);	
}

//-----------------------------------------------------------------------------

void OnButtonSearch( GtkButton* button, int index_operation ) 
{
	NewSearch( Panels[ActivePanel]->get_path() );
}

//-----------------------------------------------------------------------------

void OnButtonReload( GtkButton* button, int index_operation ) 
{
   if(SelectedSidePanel) side_panel.LoadDevice();
		else Panels[ActivePanel]->reload();
}

//-----------------------------------------------------------------------------

void	SetButton( OnButtonFunction func, int i)
{
	GtkButton*	button=(GtkButton*)gtk_builder_get_object( builder, ButtonName[i] );
	if( button )	
	{
		gtk_button_set_label(button, ButtonLabel[i] );
		g_signal_connect( G_OBJECT( button ), "clicked",     G_CALLBACK( func ), (void*)i );
	}
}

//-----------------------------------------------------------------------------

void	SetButtonTitle( int i , char * title)
{
	GtkButton*	button=(GtkButton*)gtk_builder_get_object( builder, ButtonName[i] );
	if( button )	
	{
		gtk_button_set_label(button, title );
	}
}

//-----------------------------------------------------------------------------

void InitButton(void)
{
	ButtonName[0]=g_strdup_printf("bView");//
	ButtonName[1]=g_strdup_printf("bEdit");//
	ButtonName[2]=g_strdup_printf("bCopy");
	ButtonName[3]=g_strdup_printf("bMove");
	ButtonName[4]=g_strdup_printf("bNewDir");
	ButtonName[5]=g_strdup_printf("bDelete");
	ButtonName[6]=g_strdup_printf("bSearch");
	ButtonName[7]=g_strdup_printf("bReload");
	ButtonName[8]=g_strdup_printf("bRun");
	ButtonName[9]=g_strdup_printf("bTest");
	ButtonName[10]=g_strdup_printf("bSame");
	ButtonName[11]=g_strdup_printf("bHome");
	ButtonName[12]=g_strdup_printf("bSidePanel");
	ButtonName[13]=g_strdup_printf("bBookmark");
	ButtonName[14]=g_strdup_printf("bHidden");
	ButtonName[15]=g_strdup_printf("button1");
	ButtonName[16]=g_strdup_printf("bZip");
	ButtonName[17]=g_strdup_printf("bMode");
	ButtonName[18]=g_strdup_printf("bOnePanel");
	ButtonName[19]=g_strdup_printf("bRestore");

	ButtonLabel[0]=g_strdup_printf("View F3");
	ButtonLabel[1]=g_strdup_printf("Edit F4");
	ButtonLabel[2]=g_strdup_printf("Copy F5");
	ButtonLabel[3]=g_strdup_printf("Move F6");
	ButtonLabel[4]=g_strdup_printf("Dir F7");
	ButtonLabel[5]=g_strdup_printf("Trash F8");
	ButtonLabel[6]=g_strdup_printf("Search");
	ButtonLabel[7]=g_strdup_printf("Reload");
	ButtonLabel[8]=g_strdup_printf("Lock");
	ButtonLabel[9]=g_strdup_printf("Test");
	ButtonLabel[10]=g_strdup_printf("Same");
	ButtonLabel[11]=g_strdup_printf("Home");
	ButtonLabel[12]=g_strdup_printf("Side");
	ButtonLabel[13]=g_strdup_printf("Bookmark");
	ButtonLabel[14]=g_strdup_printf("Hidden");
	ButtonLabel[15]=g_strdup_printf("test");
	ButtonLabel[16]=g_strdup_printf("Zip");
	ButtonLabel[17]=g_strdup_printf("ModeView");
	ButtonLabel[18]=g_strdup_printf("OnePanel");
	ButtonLabel[19]=g_strdup_printf("Restore");

	SetButton( OnButtonEdit, 0 );
	SetButton( OnButtonEdit, 1 );
	SetButton( OnButtonCopy, 2 );
	SetButton( OnButtonMove, 3 );
	SetButton( OnButtonNewDir, 4 );
	SetButton( OnButtonDelete, 5 );
	SetButton( OnButtonSearch, 6 );
	SetButton( OnButtonReload, 7 );
	SetButton( OnButtonRun, 8 );
	SetButton( OnButtonTest, 9 );
	SetButton( OnButtonSame, 10 );
	SetButton( OnButtonHome, 11 );
	SetButton( (OnButtonFunction*)OnMenuSidePanel, 12 );
	SetButton( OnButtonBookmark, 13 );
	SetButton( OnButtonHidden, 14 );
	SetButton( OnButtonTest, 15 );
	SetButton( OnButtonZip, 16 );
	SetButton( OnButtonModeView, 17 );
	SetButton( (OnButtonFunction*)OnMenuOnePanel, 18 );
	SetButton( OnButtonRestore, 19 );

//	EditViewDefault[0] = g_strdup_printf("%s", "leafpad");
//	EditViewDefault[1] = g_strdup_printf("%s", "gedit");
	
}

//-----------------------------------------------------------------------------
 
gchar * GetForZipString(void)
{
	GtkTreeModel     *model;
	GtkTreeIter       iter;
	GList *row, *rows;
	gchar* name=NULL;

	rows = Panels[ActivePanel]->get_selected(&model);
	string s="";
	for( row = rows; row; row = row->next )
	{
		GtkTreePath* tp = (GtkTreePath*)row->data;

		if( gtk_tree_model_get_iter( model, &iter, tp ) )
		{
			gtk_tree_model_get(model, &iter, MODEL_TEXT_FULL_NAME, &name, -1);
			s = s+" "+string(name);
			g_free(name);
		}
		gtk_tree_path_free( tp );
	}
	g_list_free( rows );
	
	if(s.length()) return g_strdup(s.c_str());
        
	return 0;
}

//-----------------------------------------------------------------------------

void OnButtonZip( GtkButton* button, int index_operation ) 
{
	const char * path = Panels[ActivePanel]->get_path();
	gchar * strlist =  GetForZipString();
	if(strlist)
	{
//tar -cvzpf archive.tar.gz dir1
		gchar * command = g_strdup_printf("cd %s && tar -cvzpf pack.tar.gz %s &", path, strlist);
		system(command);
		g_free(strlist);
		g_free(command);
	}
}

//-----------------------------------------------------------------------------

void	OnButtonRestore( GtkButton* button, int index_operation )
{
	RestoreSelectedFiles();
}

//-----------------------------------------------------------------------------

void OnButtonSelAll( GtkButton* button, int index_operation )
{
	Panels[ActivePanel]->SelectAll();
}

//-----------------------------------------------------------------------------

void OnButtonUnselAll( GtkButton* button, int index_operation )
{
	Panels[ActivePanel]->UnSelectAll();
}

//-----------------------------------------------------------------------------

void OnButtonSelPattern( GtkButton* button, int index_operation )
{
    gchar * pattern = NewPattern();
	if(pattern)
	{
		Panels[ActivePanel]->UnSelectAll();
		Panels[ActivePanel]->SelectPattern(pattern);
		g_free(pattern);
	}	
}

//-----------------------------------------------------------------------------

void OnShiftEdit( GtkButton* button, int index_operation ) 
{
	CreateNewFileDialog(Panels[ActivePanel]->MyPath,EditViewDefault[ 1 & index_operation ]);
}

//-----------------------------------------------------------------------------

void OnShiftSearch( GtkButton* button, int index_operation ) 
{
	ClassString com=g_strdup_printf("gnome-search-tool --path=%s&",Panels[ActivePanel]->MyPath); 
	system(com.s);
}

//-----------------------------------------------------------------------------

void OnButtonDiff( GtkButton* button, int index_operation )
{
 ((ClassTreePanel*)Panels[ActivePanel])->Diff(
     Panels[ActivePanel]->MyPath,
     Panels[1-ActivePanel]->MyPath,index_operation);
}

//-----------------------------------------------------------------------------

void OnButtonNewDir( GtkButton* button, int index_operation ) 
{
	const char * workdir = Panels[ActivePanel]->get_path();
	CreateNewDirDialog(workdir);
}

//-----------------------------------------------------------------------------

void	OnButtonBookmark( GtkButton* button, int index_operation )
{
//	Panels[ActivePanel]->CreateBookmark();
}

//-----------------------------------------------------------------------------

void OnButtonMove( GtkButton* button, int index_operation ) 
{
	ExternalFileCopy(getuid(),TASK_MOVE);
}

//-----------------------------------------------------------------------------

void OnButtonCopy( GtkButton* button, int _index_operation ) 
{
	GList * l = PanelGetSelected();
	gchar * mes=0;
	for ( int i=0; l; l = l->next,i++ )
	{
		mes=Untar((const char*)l->data,Panels[1-ActivePanel]->MyPath);
	}
	if(!mes)
	{	
		ExternalFileCopy(getuid(),TASK_COPY);
	}
}

//-----------------------------------------------------------------------------

void OnButtonDelete( GtkButton* button, int on_dialog ) 
{
	int intrash;
	ClassString mes;

	if(IsAlreadyTrashed(Panels[ActivePanel]->MyPath))
	{   
		mes="Это файлы уже в корзине.\nУдалить безвозвратно?";
		intrash=0;
		on_dialog=1;
	} else
	{
		const char * path_trash;
		path_trash=Filename2TrashDir(Panels[ActivePanel]->MyPath);
		if(!path_trash) 
		{
			mes="Корзина не найдена, удалить безвозвратно?";
			intrash=0;
			on_dialog=1;
		} else
		{
			mes="Переместить в корзину?";
			intrash=1;
		}	
	}

	if(on_dialog)
	{	
		if(!DialogYesNo(mes.s)) return;
	}	

	GList * l = Panels[ActivePanel]->GetSelectedFiles();
	if(intrash)	UtilsMoveInTrash(l); else UtilsUnlink(l);
}

//-----------------------------------------------------------------------------

void OnButtonEdit( GtkButton * b, int flags ) 
{
	const char * edit=EditViewDefault[flags &3];
	
	ClassString name = Panels[ActivePanel]->GetSelectedFile();

	if(!name.s)
	{
		string s="Not selected text file";
        gtk_statusbar_push (StatusBar,0, s.c_str());
		return;
	}

	Untar(name.s,0);
	
	ClassString com = g_strdup_printf("%s '%s' &", edit, name.s);
	system(com.s);
}

//-----------------------------------------------------------------------------

void	Execute(const char * com)
{
		printf("Start unpack:%s \n",com);
		system(com);
		printf("Unpack end \n");	
}

//-----------------------------------------------------------------------------

gchar * Untar(const char * name,const char * destdir)
{
	ClassString archve = GetArchivePath();

	const char mes[]="Не в архиве ";

	if(strncmp(name,archve.s,strlen(archve.s)))
	{
		printf("%s '%s' \n",mes,name);
		return 0;
	}

	struct stat file_stat;
	if(!lstat(name, &file_stat))
	{	
		if(S_ISLNK(file_stat.st_mode))
		{
			return  g_strdup(name);
		}	
	}

	ClassString zip=g_strdup(&name[strlen(archve.s)]);
	ClassString inzip=g_strdup(zip.s);
			
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
		printf("%s\n%s",mes,name);
		return 0;
	}	

	ClassString	cache=g_build_filename(PATH_CACHE_ARCHIVE,zip.s,NULL);
	CreateDirInDir(cache.s);


	ClassString com;
	const char * unpack_dir;
	if(!destdir) unpack_dir=cache.s;  else unpack_dir=destdir;

	if(IsTar(zip.s))
	{	
        ClassString base=g_path_get_dirname(inzip.s);
		if(!strcmp(base.s,"."))
		{
			com=g_strdup_printf("cd '%s'; tar -x --file='%s' './%s'", unpack_dir,zip.s,inzip.s);
		}	else com=g_strdup_printf("cd '%s'; tar -x --file='%s' '%s'", unpack_dir,zip.s,inzip.s);
		Execute(com.s);
	}	else

	if(IsDeb(zip.s))
	{	
		com=g_strdup_printf("cd '%s'; ar x '%s' '%s'", unpack_dir,zip.s,inzip.s);
		Execute(com.s);
	}	else
		
	if(IsZip(zip.s))
	{	
		
		if(!lstat(name, &file_stat))
		{	
			if(S_ISDIR(file_stat.st_mode) ) 
				com=g_strdup_printf("cd '%s'; unzip -o '%s' '%s/*'", unpack_dir, zip.s, inzip.s);
			else com=g_strdup_printf("cd '%s'; unzip -o '%s' '%s'", unpack_dir, zip.s, inzip.s);
		}
		Execute(com.s);
		
	}	else
	if(IsRar(zip.s))
	{	
		if(inzip.s[0]=='*')
		{
			ClassString password=InputPassword();
			printf("Треба пассворд <%s>\n",password.s);
			com=g_strdup_printf("cd '%s'; unrar -p'%s' x '%s' '%s'", 
			                    unpack_dir,password.s,zip.s,&inzip.s[1]);
			printf("%s\n",com.s);
		} 	else com=g_strdup_printf("cd '%s'; unrar -p- x '%s' '%s'", unpack_dir,zip.s,inzip.s);			
		Execute(com.s);
	}	else return 0;
	
	cache=g_build_filename(unpack_dir,inzip.s,NULL);
	if(!destdir)
	{	
		if(!lstat(cache.s, &file_stat))
		{	
			unlink(name);
			symlink(cache.s,name);
			return g_strdup(name);
		} else printf("file not create - '%s'\n",cache.s);
	}	
	return g_strdup(cache.s);
}

//-----------------------------------------------------------------------------

