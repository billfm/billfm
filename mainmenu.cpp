#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <errno.h>
#include <sys/inotify.h>

#include "tree.h"
#include "disks.h"
#include "utils.h"
#include "fsmonitor.h"

static void MountShared(int ReadOnly);
int  SetApplication(const char * app, const char * filename);

void CreateLink(const char * source,const char * dest_dir, int show_query,int type_link);
void miCreateBookmark(GtkButton* button, int index_operation);
void miCreateSymlink(GtkButton* button, int index_operation);
void miCreateHardlink(GtkButton* button, int index_operation);
void OnMenuFarCopy(GtkObject *object, gpointer user_data);
void miBreakLink(GtkButton* button, int index_operation);

//-----------------------------------------------------------------------------

void ChangeLowerUpper(int mode)
{
	ClassString mes;
	if(!mode) mes=g_strdup("Преобразовать имя файла к нижнему регистру");
	 else mes=g_strdup("Преобразовать имя файла к верхнему регистру");
		
	if(!DialogYesNo(mes.s)) return;

    GList * l = Panels[ActivePanel]->GetSelectedFiles();
	for ( int i=0; l; l = l->next,i++ )
	{
		ClassString dest=g_path_get_basename((const char*)l->data);	
		if(!mode) dest=utf8tolower(dest.s);
		 else dest=utf8toupper(dest.s);
		dest=g_build_filename(Panels[ActivePanel]->MyPath,dest.s, NULL );			
		rename((const char*)l->data,dest.s);
	}
}

//-----------------------------------------------------------------------------

void miDeletelinkAndFile(GtkObject *object, gpointer user_data)
{
	ClassString link = Panels[ActivePanel]->GetSelectedItem();
	ClassString name=Link2File(link.s);
	if(name.s)
	{	
		ClassString mes=g_strdup_printf("Удалить файл %s \nпо ссылке  %s\n и саму ссылку?",name.s,link.s);;
		if(!DialogYesNo(mes.s)) return;
		GList * l=0;
		l=g_list_append(l,name.s);
		l=g_list_append(l,link.s);		
		UtilsMoveInTrash(l);
	}	
}

//-----------------------------------------------------------------------------

void miLastOperation(GtkObject *object, gpointer user_data)
{
	system("leafpad /tmp/billfm.txt");
}

//-----------------------------------------------------------------------------

void miToLower(GtkObject *object, gpointer user_data)
{
	ChangeLowerUpper(0);
}

//-----------------------------------------------------------------------------

void miToUpper(GtkObject *object, gpointer user_data)
{
	ChangeLowerUpper(1);
}

//-----------------------------------------------------------------------------

void miClearNet(GtkObject *object, gpointer user_data)
{

}

//-----------------------------------------------------------------------------

void miScanNet(GtkObject *object, gpointer user_data)
{

}

void miMountSmbRead(GtkObject *object, gpointer user_data)
{
	MountShared(1);
}

void miMountSmb(GtkObject *object, gpointer user_data)
{
	MountShared(0);
}

//-----------------------------------------------------------------------------

void miUmountSmb(GtkObject *object, gpointer user_data)
{
	ClassString dest=Panels[ActivePanel]->GetSelectedItem();
    ClassString com=g_strdup_printf("gksudo umount '%s'",dest.s);
	system(com.s);
	system("sudo -K"); 	
}

//-----------------------------------------------------------------------------

void OnMenuSudoView (GtkObject *object, gpointer user_data)
{
  OnButtonEdit( NULL, 2);
}

//-----------------------------------------------------------------------------

void OnMenuSudoEdit (GtkObject *object, gpointer user_data)
{
  OnButtonEdit( NULL, 3);
}

//-----------------------------------------------------------------------------

void OnMenuSudoClearTrash (GtkObject *object, gpointer user_data)
{
	ExternalClearTrash(g_get_home_dir());
}

//-----------------------------------------------------------------------------

void OnMenuSudoCopy(GtkObject *object, gpointer user_data)
{
	ExternalFileCopy(0,TASK_COPY);
}

//-----------------------------------------------------------------------------

void OnCreateDir(GtkObject *object, gpointer user_data)
{
	const char * workdir = Panels[ActivePanel]->get_path();
	ExternalCreateDir(workdir);
}

//-----------------------------------------------------------------------------

void OnMenuDiff1 (GtkObject *object, gpointer user_data)
{
  OnButtonDiff( NULL, 1);
}

//-----------------------------------------------------------------------------

void OnMenuDiff2 (GtkObject *object, gpointer user_data)
{
  OnButtonDiff( NULL, 2);
}

//-----------------------------------------------------------------------------

void OnMenuDiff3 (GtkObject *object, gpointer user_data)
{
  OnButtonDiff( NULL, 3);
}

//-----------------------------------------------------------------------------

void OnMenuExit (GtkObject *object, gpointer user_data)
{
	SaveSetting();
	gtk_main_quit();
}

//-----------------------------------------------------------------------------

gchar *  OpenFileDialog(void)
{
	gchar *result=0;
	GtkWidget *filedlg;

   filedlg = gtk_file_chooser_dialog_new("Выбор файла", (GtkWindow*)NULL, 
	GTK_FILE_CHOOSER_ACTION_OPEN,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_OPEN, 
	GTK_RESPONSE_ACCEPT, NULL);

   if (gtk_dialog_run (GTK_DIALOG(filedlg)) == GTK_RESPONSE_ACCEPT)
   {
      result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER (filedlg));
   }
	gtk_widget_destroy(filedlg);
	return g_strdup(result);
}

//-----------------------------------------------------------------------------

gchar *  SaveFileDialog(void)
{
//Сохранить как...
   gchar *result=0;       //Путь к открываемому файлу
   GtkWidget *filedlg;    //Новый виджет, «диалог выбора файлов»
/*Создаем «диалог сохранения файлов»*/
   filedlg = gtk_file_chooser_dialog_new("Сохранить как...", (GtkWindow*)0, 
GTK_FILE_CHOOSER_ACTION_SAVE,GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, GTK_STOCK_SAVE, GTK_RESPONSE_ACCEPT, NULL);
   /*Если нажата клавиша «Сохранить как», то… */
	if (gtk_dialog_run (GTK_DIALOG(filedlg)) == GTK_RESPONSE_ACCEPT)
	{
		result = gtk_file_chooser_get_filename(GTK_FILE_CHOOSER(filedlg));
		gtk_widget_destroy(filedlg);     //Уничтожаем окно
	}
	gtk_widget_destroy(filedlg);
	return g_strdup(result);		  
}		  

//-----------------------------------------------------------------------------

void OnMenuSaveList (GtkObject *object, gpointer user_data)
{
	ClassString source=SaveFileDialog();
	if(source.s) UtilsSaveFiles(source.s);
}


//-----------------------------------------------------------------------------

void OnMenuLoadList(GtkObject *object, gpointer user_data)
{
	ClassString source=OpenFileDialog();
	if(source.s)
	{
	((ClassTreePanel*)Panels[ActivePanel])->LoadFromFile(source.s);
	}	
}

//-----------------------------------------------------------------------------

void miApplication(GtkObject *object, gpointer user_data)
{
	ClassString app=Panels[ActivePanel]->GetSelectedFile();
	ClassString filename=Panels[1-ActivePanel]->GetSelectedFile();
	if(SetApplication(app.s,filename.s)) SetApplication(filename.s,app.s);
}

//------------------------------------------------------------------------------

void ConnectMemuSignal()
{
	const char* items[] =
    {
		"miExit",
		"miSidePanel",
		"miOnePanel",
		"miModePanel",
		"miBookmark",
		"miSelectAll",
		"miUnselectedAll",
		"miSelectPattern",
		"miSymlink",
		"miSudoView",
		"miSudoEdit",		
		"miDiff1",
		"miDiff2",
		"miDiff3",
		"miSudoClearTrash",
		"miLoadList",
		"miSaveList",
		"miCopyFar",
		"miSudoCreateDir",
		"miSudoCopy",
		"miClearNet",
		"miScanNet",
		"miMountSmb",
		"miUmountSmb",
		"miMountSmbRead",
		"miApplication",
		"miToLower",
		"miToUpper",
		"miHardlink",
		"miDeletelinkAndFile",
		"miSelectBreakLink",
		"miLastOperation",
		0
	};

	GCallback sig[] =
	{
		(GCallback) OnMenuExit,

		(GCallback) OnMenuSidePanel,
		(GCallback) OnMenuOnePanel,
		(GCallback) OnButtonModeView,
		(GCallback) miCreateBookmark,
		(GCallback) OnButtonSelAll,
		(GCallback) OnButtonUnselAll,
		(GCallback) OnButtonSelPattern,
		(GCallback) miCreateSymlink,

		(GCallback) OnMenuSudoView,
		(GCallback) OnMenuSudoEdit,
		(GCallback) OnMenuDiff1,
		(GCallback) OnMenuDiff2,
		(GCallback) OnMenuDiff3,
		(GCallback) OnMenuSudoClearTrash,				
		(GCallback) OnMenuLoadList,
		(GCallback) OnMenuSaveList,
		(GCallback) OnMenuFarCopy,
		(GCallback) OnCreateDir,
		(GCallback) OnMenuSudoCopy,
		(GCallback) miClearNet,
		(GCallback) miScanNet,
		(GCallback) miMountSmb,
		(GCallback) miUmountSmb,		
		(GCallback) miMountSmbRead,
		(GCallback) miApplication,
		(GCallback) miToLower,
		(GCallback) miToUpper,
		(GCallback) miCreateHardlink,
		(GCallback) miDeletelinkAndFile,
		(GCallback) miBreakLink,
		(GCallback) miLastOperation,
		0
	}; 

	GtkImageMenuItem * menu_item;
	for(int i=0; items[i]; i++)
	{	
		menu_item = (GtkImageMenuItem *)gtk_builder_get_object( builder, items[i] );
		if(menu_item) g_signal_connect((GtkWidget*) menu_item, "activate", sig[i],0 );
	}	
}

//-----------------------------------------------------------------------------

void miCreateSymlink( GtkButton* button, int index_operation )
{
	GList * l = Panels[ActivePanel]->GetSelectedFiles();
	while (l!= NULL)
	{
		CreateLink((const char*)l->data,PanelGetDestDir(),0,0);
		l = (GList*)g_slist_next(l);
	}

}

//-----------------------------------------------------------------------------

void miCreateHardlink( GtkButton* button, int index_operation )
{
	GList * l = Panels[ActivePanel]->GetSelectedFiles();
	while (l!= NULL)
	{
		struct stat file_stat;
		if(!lstat((const char*)l->data,&file_stat)) 
		{
			if(!S_ISDIR(file_stat.st_mode))
			{
				CreateLink((const char*)l->data,PanelGetDestDir(),0,1);
			}
		}	
		l = (GList*)g_slist_next(l);
	}

}

//-----------------------------------------------------------------------------

void  CreateLink(const char * source, const char * dest_dir, int show_query,int type_link)
{
	if(!source) return;
	
	ClassString dest = g_path_get_basename(source);
    dest =g_build_filename(dest_dir,dest.s, NULL );
	if(show_query)
	{	
		dest=EditFileName(dest.s);
	} else
	{
		struct stat file_stat;
		if(!lstat(dest.s, &file_stat ) ) 
		{
			dest=EditFileName(dest.s);
		}	
	}	

	if(!dest.s)  return;
	int res;
	if(!type_link) res=symlink(source,dest.s); else res=link(source,dest.s);
	
	if(res)
	{   
		ClassString mes = g_strdup_printf("Error create link !\n Source file '%s'\n Link name '%s' \n Error (%d) '%s'",
    	           source,dest.s,errno,strerror(errno));
		ShowWarning(mes.s);
	}

}
 
//-----------------------------------------------------------------------------

void  miCreateBookmark(GtkButton* button, int index_operation)
{
	ClassString source=Panels[ActivePanel]->GetSelectedDir();
	if(!source.s) return;	
//	ClassString dest_dir = g_build_filename(g_get_home_dir(),".config/billfm/bookmark", NULL );	
   	ClassString book=g_build_filename(config_path,"bookmark",NULL);
	CreateLink(source.s,book.s,1,0);
	side_panel.LoadDevice();
}

//-----------------------------------------------------------------------------

int SetApplication(const char * app, const char * filename)
{
	gchar * p=GetExt(app);
	if(!p) return 1;
	if(strcmp(p,"desktop")) return 1;

	char * ext=GetExt(filename);
	if(!ext) return 1;

//	ClassString setfile = g_strdup_printf("%s/.config/billfm/mime2command.txt",g_get_home_dir());
	ClassString setfile=g_build_filename(config_path,"mime2command.txt",NULL);
	FILE * f = fopen (setfile.s,"a+");
	if(!f)
	{	
		printf("Not open %s\n",setfile.s);
		return 1;
	}	

	ClassString str=g_strdup_printf("%s %s\n",ext,app);
	fwrite(str.s,strlen(str.s),1,f);
	fclose(f);

	InsertCommand(ext,g_strdup(app));
	return 0;
}

//-----------------------------------------------------------------------------

static void MountShared(int ReadOnly)
{
	gchar * fullname;
	int mime;
	gchar * name;
	Panels[ActivePanel]->GetSelectedItem(&fullname,&mime,&name);
	ClassString dump=fullname;
	if(mime!=ICON_GNOME_SMB_SHARE)
	{
		printf("Not shared SMB folder \n");
		return;
	}	
	//ClassString dest=Panels[ActivePanel]->GetSelectedItem();
	struct stat file_stat;
	ClassString ip = g_path_get_dirname(fullname);
	if(lstat(ip.s, &file_stat))
	{
		printf("Not read stat shared SMB folder \n");
		return;
	}	

	if(S_ISLNK(file_stat.st_mode))
	{	
		ip=Link2File(ip.s);
	}	

	ip = g_path_get_basename(ip.s);

	ClassString source=name;//g_path_get_basename(dest.s);
	source=g_strdup_printf("//%s/%s",ip.s,source.s);
//{	                       
//	ClassString com=g_strdup_printf("gvfs-mount smb:%s &",source.s);
//}							   

	ClassString com=g_strdup_printf("gksudo %s &",util_path);
	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"SMB_MOUNT\n");
	fprintf(f,"%s\n",fullname);
	fprintf(f,"%s\n",source.s);
	if(ReadOnly) fprintf(f,"ro\n");	else fprintf(f,"rw\n");
	fclose(f);

	system(com.s);
//  gvfs-mount smb://10.20.12.34/d/	                       
}

//-----------------------------------------------------------------------------

void OnMenuFarCopy(GtkObject *object, gpointer user_data)
{
	ExternalFileCopy(getuid(),TASK_COPY);
}

//-----------------------------------------------------------------------------

void  miBreakLink(GtkButton* button, int index_operation)
{
	Panels[ActivePanel]->SelectBreakLink();
}
//-----------------------------------------------------------------------------

