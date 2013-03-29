#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <utime.h>

using namespace std;

#include "disks.h"
#include "trash.h"
#include "utils.h"


//-----------------------------------------------------------------------------
int SizeDirShow=0;

static int all_dirs;
static int all_files;
static long int all_size;
static int ovewrite_all;
static int skip_all;
static int cancel_all;
static int all_hidden;

long int PgCommonSize=0;
int tolower ( int c );
int toupper ( int c );

//#define MAX_FILE_SIZE   10l*0x100000
//#define MAX_FILE_SIZE   0x7FFFFFFFFFFFFFFF


//	if( g_file_test( path, G_FILE_TEST_IS_DIR G_FILE_TEST_EXISTS) )

//G_FILE_TEST_IS_REGULAR	TRUE if the file is a regular file (not a symlink or directory)
//G_FILE_TEST_IS_SYMLINK	TRUE if the file is a symlink.
//G_FILE_TEST_IS_DIR	TRUE if the file is a directory.
//G_FILE_TEST_IS_EXECUTABLE	TRUE if the file is executable.
//G_FILE_TEST_EXISTS	TRUE if the file exists. It may or may not be a regular file.

static void ParserDir(const char * source,  const char * dest, InfoOperation * fo, int level);
void OperationFile(const char * source,  const char * dest, InfoOperation * fo);
void FileCopyFunc(const char * source, const char * dest, InfoOperation * fo);
int  LowFileCopy(const char * source, const char * dest, long int size, InfoOperation * fo);
static int UnlinkFile(const char * source);
void CopyProperty(const char * source, const char * dest);
static void StartFileOperation();
static void FileOperation1(GList * l, const char * dest_dir, InfoOperation * fo);
static int CheckWriteFile(const char * source,gchar ** dest, InfoOperation * fo);
int CreateWriteDir(const char * source, gchar ** dest);
int CreateNewSymlink(const char * link, const char * dest);

//-----------------------------------------------------------------------------

void ShowMessage3(GtkWindow* parent, const char* title, const char* message )
{
  GtkWidget* dlg = gtk_message_dialog_new_with_markup(parent, GTK_DIALOG_MODAL, 
                                                      GTK_MESSAGE_ERROR, GTK_BUTTONS_OK,"%s", message);
  if( title )
    gtk_window_set_title( (GtkWindow*)dlg, title );
  gtk_dialog_run( GTK_DIALOG(dlg) );
  gtk_widget_destroy( dlg );
}


//-----------------------------------------------------------------------------

void ShowFileOperation(gchar * str)
{
	ShowMessage3(NULL,"Error file operation !",str);
}

//-----------------------------------------------------------------------------

void ShowWarning(gchar * str)
{
	ShowMessage3(NULL, "Warning  !", str );
}

//-----------------------------------------------------------------------------

gchar * showfilesize(long int size)
{
 if(size<1024) return g_strdup(" "); else
 if(size<1024*1024) return g_strdup_printf("(%2.2f K)", size/1024.0); else
 if(size<1024*1024*1024) return g_strdup_printf("(%2.2f M)",size/(1024.0*1024)); else
		return g_strdup_printf("(%2.2f G)",((float)size)/(1024*1024*1024));
}

//-----------------------------------------------------------------------------

void ShowCancel(gchar * str)
{
    gint ret;
	GtkWidget * dlg = gtk_message_dialog_new(NULL,
                                      GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_YES_NO,
                                      "%s\nПрервать все операции ?",str);
        gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_YES); //MOD
        ret = gtk_dialog_run( GTK_DIALOG( dlg ) );
        gtk_widget_destroy( dlg );
        if ( ret == GTK_RESPONSE_YES ) cancel_all=1;
}

//-----------------------------------------------------------------------------

void StartFileOperation()
{
	ovewrite_all=0;
	skip_all=0;
	cancel_all=0;
	all_dirs=0;
	all_files=0;
	all_size=0;
	all_hidden=0;
}

//-----------------------------------------------------------------------------

void ClearDir(const char * source)
{
	StartFileOperation();
	if(g_file_test(source,(GFileTest)(G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS)))
	{	
		InfoOperation fo;
		fo.func=TASK_CLEAR_DIR;
		ParserDir(source,"/dev/null",&fo,0);
	}	
}

//-----------------------------------------------------------------------------

static int UnlinkFile(const char * source)
{
	int res=unlink(source);
	if(res)
	{
		ClassString mes = g_strdup_printf("Error delete source '%s'.\n%s",source,strerror(errno));
		ShowCancel(mes.s);
	}  
	return res;
}

//-----------------------------------------------------------------------------

int IsEmptyDir(const char * source)
{
	if(!source) return 0;
	StartFileOperation();
	InfoOperation fo;
	fo.func=TASK_INFO;

	ParserDir(source,"/dev/null",&fo,0);
	ClassString size=showfilesize(all_size);
	return  all_dirs+all_files-1;
}

//-----------------------------------------------------------------------------

void UtilsMoveFiles(GList * l, const char * dest_dir)
{
	InfoOperation fo;
	fo.func=TASK_MOVE;

	FileOperation1(l,dest_dir,&fo);
}

//-----------------------------------------------------------------------------

void UtilsClearTrash(void)
{
	ClassString source; 	
	InfoDisk * _info;
	GList* list=g_list_first (List_disk);

	StartFileOperation(); 
	InfoOperation fo;
	fo.func=TASK_CLEAR_DIR;

	while (list!= NULL)
	{
		_info=(InfoDisk *)list->data;
		source=g_strdup_printf("%s/info",_info->path_trash);

   if(g_file_test(source.s,(GFileTest)(G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS)))
	{	
		ParserDir(source.s,"/dev/null",&fo,0);
	}	

	source=g_strdup_printf("%s/files",_info->path_trash);

   if(g_file_test(source.s,(GFileTest)(G_FILE_TEST_IS_DIR | G_FILE_TEST_EXISTS)))
	{	
		ParserDir(source.s,"/dev/null",&fo,0);
	}	

    list = (GList*)g_slist_next(list);
   }
}

//-----------------------------------------------------------------------------

void CopyProperty(const char * source, const char * dest)
{
	struct stat file_stat;

	if(!lstat(source, &file_stat)) 
	{
		struct utimbuf times;
	    times.actime = file_stat.st_atime;
        times.modtime = file_stat.st_mtime;
        utime(dest, &times );

		int TypeFS = GetTypeFS(source);

		if( TypeFS!=TYPE_FS_NTFS )
	    {	
          chown(dest,file_stat.st_uid,file_stat.st_gid); 
          chmod(dest,file_stat.st_mode);			
		} else
		{
			chown(dest,getuid(),getgid()); 
			if(S_ISDIR(file_stat.st_mode)) chmod(dest,0777); else chmod(dest,0666);
		}	
	}
}

//-----------------------------------------------------------------------------

void UtilsSaveFiles(const char * source)
{
	GList * l = PanelGetSelected();
	FILE *  f=fopen(source,"wt");
	const char cr[]="\n";
	for ( ; l; l = l->next )
	{
		gchar* name = (gchar*) l->data;
		fwrite(name,1,strlen(name),f);
		fwrite(cr,1,1,f);		
	}
	fclose(f);
}

//-----------------------------------------------------------------------------

static int CheckWriteFile(const char * source,gchar ** dest, InfoOperation * fo)
{
    fo->dest_open=fo->Lstat_dest_new(*dest, "CheckWriteFile");
	if(fo->dest_open) 
	{
		if(fo->dest_errno==ENOENT) 	return 1; else return 0;
	} 

	if(S_ISDIR(fo->dest_mode))
    {
		ClassString mes = g_strdup_printf("Cannot overwrite directory '%s'\n to file.",*dest);
        ShowFileOperation(mes.s);
		return 0;
	}

	if(skip_all) return 0;
	if(ovewrite_all)
	{
		return 1;
	}

	int res=DialogOverride((gchar*)source,(const char **) dest);

	switch(res)
	{
		case RESPONSE_RENAME:
		    fo->Lstat_dest_new(*dest,"DialogOverride"); 
		return 1;			

		case RESPONSE_OVERWRITE:
			fo->dest_override=1;
			return 1;
   
		case RESPONSE_OVERWRITEALL:
			ovewrite_all=1;
			fo->dest_override=1;
			return 1;
              
		case RESPONSE_SKIPALL:
            skip_all=1;
		case RESPONSE_SKIP: 
		return 0;

        case GTK_RESPONSE_CANCEL:
		return 0;

		default: 
			cancel_all=1; 
		return 0;
	}

	return 0;
}

//-----------------------------------------------------------------------------

static void FileOperation1(GList * l, const char * dest_dir, InfoOperation * fo)
{
	StartFileOperation();

	InfoOperation ffo;
	memcpy(&ffo,fo,sizeof(InfoOperation));

	for ( ; l; l = l->next )
	{
        if(cancel_all) break;
		const char* source = (const char*) l->data;
		ClassString dest= g_path_get_basename(source);	
		dest = g_build_filename(dest_dir,dest.s, NULL );
		ParserDir(source,(const char *)dest.s,&ffo,0);
//	    CopyProperty(source,dest.s);
	}
	printf("All dirs %d, all files %d, common files size %ld\n",all_dirs,all_files,all_size);
}

//-----------------------------------------------------------------------------

void OperationFile(const char * source,  const char * dest1, InfoOperation * fo)
{
	all_files++;
	if(fo->func==TASK_INFO)
	{	
		ClassString str=g_strdup_printf("%s\n",source);
		if(fo->log>0)
		{	
			write(fo->log,str.s,strlen(str.s));                          
		}	
		return;
	}	
	
	if(fo->func==TASK_DELETE || fo->func==TASK_CLEAR_DIR)
	{
		UnlinkFile(source);
		return;
	}	

	ClassString dest=g_strdup(dest1);
	{
		gchar * new_dest=(gchar *)dest1;
		if(!CheckWriteFile(source, &new_dest,fo)) return;
		if(new_dest!=dest1)
		{
			printf("Rename file %s to %s\n",source,new_dest);
			dest=g_strdup(new_dest);
			g_free(new_dest);
		}
	}

	if(fo->func==TASK_COPY)
	{	
		fo->source_delete=SOURCE_NO_DELETE;
		FileCopyFunc(source,dest.s,fo);
	} else	
	if(fo->func==TASK_MOVE)
	{	
		fo->source_delete=SOURCE_DELETE;
		FileMoveFunc(source,dest.s,fo); 
	}	
}

//-----------------------------------------------------------------------------

int FileMoveFunc(const char * source, const char * dest, InfoOperation * fo)
{
	if(S_ISLNK(fo->source_mode) )
	{
		fo->source_delete=SOURCE_DELETE;
		FileCopyFunc(source,dest,fo); 
		return 0;
	}	

	int res=rename(source , dest);
	if(res)
	{
		if(errno==EXDEV)
		{
			fo->source_delete=SOURCE_DELETE;
			FileCopyFunc(source,dest,fo); 
			return 0;
		}
		ClassString mes = g_strdup_printf("Error rename file '%s'\nto '%s' \n%s.", 
		                              source,dest,strerror( errno ) );
		ShowCancel(mes.s);
	}
	printf("Rename file %s to %s \n",source,dest);
	return res;
}

//-----------------------------------------------------------------------------

void CreateDir(gchar * dest)
{
	if(mkdir(dest, 0755))
	{
		ClassString mes = g_strdup_printf("Error create dir '%s'.\n%s",dest,strerror(errno));
   	    ShowCancel(mes.s);
	}
}

//-----------------------------------------------------------------------------

int CreateWriteDir(const char * source, gchar ** dest)
{
	if(skip_all) return 0;

	struct stat dest_stat;

    if(lstat(*dest, &dest_stat)) 
	{
		if(errno==ENOENT)
		{ 
			CreateDir(*dest);
			return 1;
		} else
		{
			ClassString mes = g_strdup_printf("Not read state CreateDir(%s)",*dest);
            ShowFileOperation(mes.s);
			return 0;
		}
	} 

	if(!S_ISDIR(dest_stat.st_mode))
    {
		ClassString mes = g_strdup_printf("Cannot overwrite file  '%s'\n to directory.",*dest);
        ShowFileOperation(mes.s);
		return 0;
	}
//сюда дошли если директория существует
	if(ovewrite_all)
	{
		return 1;
	}

	int res=DialogOverride((gchar*)source,(const char **) dest);

	switch(res)
	{
		case RESPONSE_RENAME:
			CreateDir(*dest);
		return 1;			

		case RESPONSE_OVERWRITE:	return 1;
   
		case RESPONSE_OVERWRITEALL:
			ovewrite_all=1;
		return 1;
              
		case RESPONSE_SKIPALL:
            skip_all=1;
		case RESPONSE_SKIP: 
		return 0;

        case GTK_RESPONSE_CANCEL:
		return 0;

		default: 
			cancel_all=1; 
		return 0;
	}

	return 0;
		
}

//-----------------------------------------------------------------------------
 
void ExternalFileCopy(uid_t user,int operation)
{
	GList * l=PanelGetSelected();
	const char * dest_dir=PanelGetDestDir();

	ClassString com;	
	if(user!=getuid())
	{	
		com=g_strdup_printf("gksudo %s &",PATH_FM_UTILS);
	} else	
	{	
		com=g_strdup_printf("%s &",PATH_FM_UTILS);		
	}	

	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	if(operation==TASK_COPY)	fprintf(f,"COPY\n"); else
	if(operation==TASK_MOVE)	fprintf(f,"MOVE\n");

	fprintf(f,"%s\n",dest_dir);

	for ( ; l; l = l->next )
	{
		const char* source = (const char*) l->data;
		fprintf(f,"%s\n",source);
	}
	fclose(f);
	system(com.s);
	system("sudo -K"); 	
}

//-----------------------------------------------------------------------------
 
void ExternalCreateDir(const char * dest_dir)
{
	ClassString com=g_strdup_printf("gksudo %s &",PATH_FM_UTILS);		

	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"CREATE_DIR\n");
	fprintf(f,"%s\n",dest_dir);

	fclose(f);
	system(com.s);
	system("sudo -K"); 	
}

//-----------------------------------------------------------------------------
 
void ExternalClearTrash(const char * dest_dir)
{
	ClassString com=g_strdup_printf("gksudo %s &",PATH_FM_UTILS);
	
	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"CLEAR_TRASH\n");
	fprintf(f,"%s\n",dest_dir);

	fclose(f);
	system(com.s);
	system("sudo -K"); 	
}

//-----------------------------------------------------------------------------

int InfoOperation::Lstat_source(const char *file_name, const char * mes)
{
	struct stat source_stat;			
	ClassString str;
	if(!mes) str=g_strdup_printf("Not read state");
	source_open=lstat(file_name,&source_stat);
	if(source_open)
	{
		str=g_strdup_printf("%s\n -Источник: `%s`\n(%d) %s ",mes,file_name,errno,strerror(errno));
	    ShowFileOperation(str.s);
	} else
	{
		source_mode=source_stat.st_mode;
		source_size=source_stat.st_size;
	}
	return source_open;
};	

//-----------------------------------------------------------------------------

int InfoOperation::Lstat_dest(const char *file_name, const char * mes)
{
	ClassString str;
	struct stat dest_stat;	
	if(!mes) str=g_strdup_printf("Not read state");
	dest_open=lstat(file_name,&dest_stat);
	if(dest_open)
	{
		str=g_strdup_printf("%s\n -Получатель: `%s`\n(%d) %s ",mes,file_name,errno,strerror(errno));
	    ShowFileOperation(str.s);
	} else
	{
		dest_mode=dest_stat.st_mode;
		dest_size=dest_stat.st_size;
	} 
	return dest_open;
};	

//-----------------------------------------------------------------------------

int InfoOperation::Lstat_dest_new(const char *file_name, const char * mes)
{
	ClassString str;
	struct stat dest_stat;	
	if(!mes) str=g_strdup_printf("Not read state");
	dest_open=lstat(file_name,&dest_stat);
	if(dest_open)
	{
		if(errno==ENOENT) 
		{ 
			dest_mode=0;
			dest_size=0;
			dest_errno=ENOENT;
		} else
		{	
			str=g_strdup_printf("%s\n -Получатель: `%s`\n(%d) %s ",mes,file_name,errno,strerror(errno));
		    ShowFileOperation(str.s);
		}	
	} else
	{
		dest_mode=dest_stat.st_mode;
		dest_size=dest_stat.st_size;
	} 
	return dest_open;
};	

//-----------------------------------------------------------------------------

void RestoreSelectedFiles()
{
	StartFileOperation();
	GList * l = PanelGetSelected();

	for ( ; l; l = l->next )
	{
		gchar * source = (gchar*) l->data;
        if(cancel_all) break;
		gchar * dest=PrepareRestore(source);
		if(dest)
		{
			InfoOperation fo;
			fo.func=TASK_COPY;
			ParserDir(source,(const char *)dest,&fo,0);
			g_free(dest);
		}
	}
}

//-----------------------------------------------------------------------------

static void ParserDir(const char * source,  const char * dest, InfoOperation * fo, int level)
{
	if(fo->Lstat_source(source, "ParserDir - Не открывается источник"))
	{	
		printf("ERROR: %s\n",source);
		return;
	}	

	if( S_ISDIR(fo->source_mode))
    {
		if(strstr(dest,source))
		{
			ClassString mes = g_strdup_printf("Копирование в себя\nИсточник: '%s'\nПолучатель: %s",source,dest);
            ShowMessage3(0,"Операция отменена",mes.s);
			return;
		}	

		if(fo->func==TASK_COPY || fo->func==TASK_MOVE)
		{	
			if(!CreateWriteDir(source,(gchar**)&dest)) return;
		}	
		
		DIR * d;     
		struct dirent * entry;
		d = opendir(source);
		if(d == NULL) 
		{
			if(!SizeDirShow)
			{
				ClassString mes = g_strdup_printf("Error open source dir !\n Source file '%s'\n Error (%d) '%s'",
    	           source,errno,strerror(errno));

				ShowCancel(mes.s);
			}	
			return;
	     }

		while ( (entry = readdir(d))>0)
	    {
			if(cancel_all) return;
			if(!strcmp(entry->d_name,".")) continue;
			if(!strcmp(entry->d_name,"..")) continue;
			if(!strncmp(entry->d_name,".",1)) all_hidden++;
			
			ClassString src_new = g_build_filename(source,entry->d_name, NULL );
			ClassString dest_new = g_build_filename(dest,entry->d_name, NULL );			

//			InfoOperation fo_new;
//			fo_new.func=fo->func;
//			fo_new.mode_link=fo->mode_link;
//			ParserDir((const char*)src_new.s,(const char*)dest_new.s,&fo_new,level+1);
			ParserDir((const char*)src_new.s,(const char*)dest_new.s,fo,level+1);
			if(cancel_all) return;			
	     }  
		closedir(d);
		all_dirs++; 
 		if( fo->func==TASK_DELETE || fo->func==TASK_MOVE || (fo->func==TASK_CLEAR_DIR && level))
		{	
			if(rmdir(source))
			{
//				if(errno==ENOTEMPTY)
//				{
//					BigRemdir(source);
//					return;	
//				}	
				ClassString mes = g_strdup_printf("Error delete dir '%s'.\n%s",source,strerror(errno));
				ShowCancel(mes.s);
			}	
		}	
    } else
	{
		if( !S_ISREG(fo->source_mode) 
		 && !S_ISLNK(fo->source_mode)
		 && !S_ISSOCK(fo->source_mode)		   
		   )
		{
	 		if(fo->func!=TASK_INFO)
			{	
				gchar * mes = g_strdup_printf("Unknow file type\n '%s')", source);
				ShowWarning(mes);
			}	
			return;
		}
		all_size+=fo->source_size;
		OperationFile(source,dest,fo);
	}

}

//-----------------------------------------------------------------------------

long int GetSizeDir(const char * source)
{
	StartFileOperation();
	InfoOperation fo;
	fo.func=TASK_INFO;
	ParserDir(source,"/dev/null",&fo,0);
    return all_size;
}

//------------------------------------------------------------------------------

void UtilsCreateLink(const char * source, const char * dest_dir)
{
    ClassString link_name = g_path_get_basename(source);
    link_name =g_build_filename(dest_dir,link_name.s, NULL );

	if(symlink(source,link_name.s))
	{   
		ClassString mes = g_strdup_printf("Error create link !\n Source file '%s'\n Link name '%s' \n Error (%d) '%s'",
    	           source,link_name.s,errno,strerror(errno));
		printf("%s\n",mes.s);
	}
}

//-----------------------------------------------------------------------------

void ExternalFind(const char * mask,const char * text, const char * dest_dir)
{
	ClearDir(PATH_FIND);
	ClassString com=g_strdup_printf("%s &",PATH_FM_UTILS);

	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"FIND\n");
	fprintf(f,"%s\n",dest_dir);
	fprintf(f,"%s\n",mask);
	fprintf(f,"%s\n",text);	
	fclose(f);
	system(com.s);
}

//-----------------------------------------------------------------------------

void UtilsMoveInTrash(GList * l)
{
	StartFileOperation();
	for ( ; l; l = l->next )
	{
		gchar* name = (gchar*) l->data;
		if(cancel_all) break;
		DeleteFile(name);
	}
}

//-----------------------------------------------------------------------------

void UtilsUnlink(GList * l)
{
	InfoOperation fo;
	fo.func=TASK_DELETE;
	FileOperation1(l,"/dev/null",&fo);
}

//-----------------------------------------------------------------------------

gchar * Link2File(const char * fullname)
{
	ClassString link=g_file_read_link(fullname,NULL);
	if(!link.s)
	{
		printf("Link2File: %s %s\n",fullname,strerror(errno));
		return NULL;
	}	

	if(!g_path_is_absolute(link.s))
	{
		ClassString dirname = g_path_get_dirname(fullname);
		link = g_build_filename(dirname.s,link.s, NULL );	
	}
	return g_strdup(link.s);
}

//-----------------------------------------------------------------------------

int CreateDirInDir(const char * dest)
{
	if(!strcmp(dest,"/")) return 0;

	struct stat file_stat;
	if(!lstat( dest, &file_stat ) ) 
	{
		if(S_ISDIR(file_stat.st_mode))	return 0;
	}		

//	printf("create %s\n",dest);
	ClassString dest_next=g_path_get_dirname(dest);
	int res=CreateDirInDir(dest_next.s);
	if(res) return res;
    return mkdir(dest, 0755);
}

//-----------------------------------------------------------------------------

int CreateNewSymlink(const char * link, const char * dest)
{
//	unlink(link);
	if(symlink(link,dest))
	{
		ClassString mes = g_strdup_printf("CreateNewSymlink '%s' %s.\n%s",
		                                  link,dest,strerror(errno));
		ShowCancel(mes.s);
		return errno;
	}	
	return 0;
}

//-----------------------------------------------------------------------------

void ExternalListTar(const char * fullname, const char * dest_dir)
{
	CreateDirInDir(dest_dir);
	ClassString com=g_strdup_printf("%s &",PATH_FM_UTILS);
	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"READ_TAR\n");
	fprintf(f,"%s\n",dest_dir);
	fprintf(f,"%s\n",fullname);
	fclose(f);
	system(com.s);
}

//-----------------------------------------------------------------------------

int IsTar(const char * fullname)
{
	size_t len=strlen(fullname);

	const char key[]=".tar.gz";
	size_t lenkey=strlen(key);
	if(len>lenkey && !strcmp(key,&fullname[len-lenkey])) return 1;
	
	const char key1[]=".tar.bz2";
	size_t lenkey1=strlen(key1);
	if(len>lenkey1 && !strcmp(key1,&fullname[len-lenkey1])) return 1;

	const char key2[]=".tar";
	size_t lenkey2=strlen(key2);
	if(len>lenkey2 && !strcmp(key2,&fullname[len-lenkey2])) return 1;

	return 0;
}

//-----------------------------------------------------------------------------

int IsZip(const char * fullname)
{
	size_t len=strlen(fullname);

	const char key[]=".zip";
	size_t lenkey=strlen(key);
	if(len>lenkey && !strcmp(key,&fullname[len-lenkey])) return 1;
	
	return 0;
}

//-----------------------------------------------------------------------------

int IsRar(const char * fullname)
{
	size_t len=strlen(fullname);

	const char key[]=".rar";
	size_t lenkey=strlen(key);
	if(len>lenkey && !strcmp(key,&fullname[len-lenkey])) return 1;
	
	return 0;
}

//-----------------------------------------------------------------------------

void FileCopyFunc(const char * source, const char * dest, InfoOperation * fo)
{
	if(S_ISLNK(fo->source_mode) )
	{
		ClassString link;
		LinkDialogCopy(fo,source,dest);
//printf("%d\n",fo->mode_link);
		if(fo->mode_link==3)
		{	
			link=Link2File(source);
			ClassString mes = g_strdup_printf("Ошибка открытия по ссылке `%s` на ",source);
			if(fo->Lstat_source(link.s, mes.s)) return;
			LowFileCopy(link.s,dest,fo->source_size,fo);
		}   else
		{	
			if(fo->mode_link==1)
			{	
				link=Link2File(source);
			}   else
			if(fo->mode_link==2)
			{
				link=g_file_read_link(source,NULL);
			} 
			if(fo->dest_override && !fo->dest_open)
			{
				if(UnlinkFile(dest)) return;
			}	
			int res=CreateNewSymlink(link.s,dest);
			if(res)
			{   
				ClassString mes = g_strdup_printf("Error create link !\n Source file '%s'\n Dest file '%s' \n Error (%d) '%s'",
    	           source,dest,res,strerror(res));
					ShowCancel(mes.s);
				return;
			}	
		}
	} else
	{
		if(LowFileCopy(source,dest,fo->source_size,fo)) return;
	}	

	if(fo->source_delete) UnlinkFile(source);
}


//-----------------------------------------------------------------------------

gchar * InfoDir(GList * l)
{

	StartFileOperation();
	InfoOperation fo;
	fo.func=TASK_INFO;

//    fo.log = open(PATH_INFO_LOG,O_WRONLY);
	for ( ; l; l = l->next )
	{
		gchar* source = (gchar*) l->data;
		if(cancel_all) break;
		ParserDir(source,"/dev/null",&fo,0);
	}

	ClassString size=showfilesize(all_size);
	if(fo.log>0) close(fo.log);
	return g_strdup_printf("Dirs %d, files %d,hidden=%d, common size %ld %s",
	                       all_dirs,all_files,all_hidden,all_size,size.s);

}

//-----------------------------------------------------------------------------

void ProcessCopyFiles(const char * dest_dir, InfoOperation * fo)
{
	if(fo->Lstat_dest(dest_dir, "Не открывается каталог получатель")) return;
	GList * l=PanelGetSelected();
    InfoDir(l);
	fo->all_size=all_size;
    fo->progress = open(PATH_INFO_PROGRESS,O_WRONLY);
	FileOperation1(l,dest_dir,fo);		
}

//-----------------------------------------------------------------------------

int LowFileCopy(const char * source, const char * dest, long int size, InfoOperation * fo)
{

#define READ_BUFFER 4096
	char buf[READ_BUFFER];
	FILE *  fs=0;
	FILE *  fd=0;
	long int i=0;
	int n;
	
start:
	fs=fopen(source,"r");
	if(!fs)
	{
		ClassString mes = g_strdup_printf("Error open source '%s'.\n%s",source,strerror(errno));
		ShowCancel(mes.s);
		goto error;
//		return 1;
	}	

	fd=fopen(dest,"w+");
	if(!fd)
	{
		ClassString mes = g_strdup_printf("Error open tag '%s'.\n%s",dest,strerror(errno));
		ShowCancel(mes.s);
//		return 1;
		goto error;
	}	


	while(i<size)
	{
		if(size-i>READ_BUFFER) n=READ_BUFFER; else n=size-i;

		int r=fread(buf,1,n,fs);
		if(r!=n)
		{
			ClassString mes = g_strdup_printf("Error read '%s'(%d %d).\n%s", source,r,n,strerror(errno));
			ShowCancel(mes.s);
			goto error;
//			return 1;		
		}	

		int w=fwrite(buf,1,n,fd);
		if(w!=n)
		{
			ClassString mes = g_strdup_printf("Error write '%s' (%d %d).\n%d-%s",
			                              source,w,n,errno,strerror(errno));
			ShowCancel(mes.s);
			goto error;				
//			return 1;		
		}	
		i+=n;
        if(fo->progress>0)
		{
			char buf[128];
			sprintf(buf,"%ld %ld\n",i,size);
//			printf("%s",buf);
			write(fo->progress,buf,strlen(buf));                          
		}	
	}

	fclose(fd);
	fclose(fs);
    CopyProperty(source,dest);
	printf("Copy file %s to %s ( %ld )\n",source,dest,size);
	return 0;
error:
	if(fd) fclose(fd);
	if(fs) fclose(fs);
	ClassString mes = g_strdup_printf("Пробовать еще раз ?");
	if(!DialogYesNo(mes.s)) return 1;
	goto start;
#undef READ_BUFFER
}

//-----------------------------------------------------------------------------