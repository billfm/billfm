#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>

#include "disks.h"
#include "trash.h"
#include "utils.h"


//------------------------------------------------------------------------------

const char * IsAlreadyTrashed(const char * name)
{
	InfoDisk * info;
	GList * list=g_list_first (List_disk);
 
	while (list!= NULL)
	{
		info=(InfoDisk *)list->data;
		if(info->path_trash)
		{	
			if(!strncmp(info->path_trash,name,strlen(info->path_trash)))
			{
				return (const char *)info->path_trash;			
			}
		}	
       list = (GList*)g_slist_next(list);
	}
 return 0;
}

//-----------------------------------------------------------------------------

char * find_trash_dev(int dev)
{ 
	InfoDisk * info;
	GList* list=g_list_first (List_disk);

	while (list!= NULL)
	{
		info=(InfoDisk *)list->data;
		if(info->num_dev==dev)
		{
		   return info->path_trash;
		}
	       list = (GList*)g_slist_next(list);
	}
 return NULL;
}


//-------------------------------------------------------------------------------------------------

gchar * CheckTrash(gchar * name)
{
	struct stat file_stat;
	if(!lstat(name, &file_stat))
	{
		ClassString dir = g_build_filename(name,"files", NULL );
		if(!lstat(dir.s, &file_stat))
		{
			ClassString dir = g_build_filename(name,"files", NULL );
			if(!lstat(dir.s, &file_stat))
			{
				return name;
			}
		}
	}	
	return 0;
}

//-------------------------------------------------------------------------------------------------

gchar * get_trashinfo(char * filename,char * trash_name)
{
//по имени корзины и полному имени файла в корзине возвращает имя инфо файла 
    char * s1;
    char * s2;
    char * info=NULL;
    struct stat file_stat;    
    char * name=g_strdup(filename);

    if(trash_name)
    {
     s1=name+strlen(trash_name)+strlen("/files/");//delete name trash
     s2=strstr(s1,"/");
     if(s2) s1[ strlen(s1)-strlen(s2) ]=0;
     info = g_strdup_printf("%s/info/%s.trashinfo",trash_name,s1);
    };  
    g_free(name);

    if(lstat( info, &file_stat )) 
    {
     printf("For %s \nin trash %s \nnot found %s\n.",filename,trash_name,info);
     return NULL;  
    }

    return info;
}

//------------------------------------------------------------------------------

gchar * get_full_restore_name( const char * source, const char * trash_name, const char * dest )
{
    char * name_in_trash;

    name_in_trash = (char*)source + strlen(trash_name) + strlen("/files/");
    char * s=strstr(name_in_trash,"/");
	if(s)
	{
     s++;
	 return g_strdup_printf("%s/%s",dest,s);
	}
    return g_strdup(dest); 
}

//-----------------------------------------------------------------------------

gchar * PrepareRestore(const char * source)
{
	gchar * dest_name=NULL;
	gchar * dirname;

    const char * trash_name=IsAlreadyTrashed( source );
    if(!trash_name)
	{
		ClassString mes = g_strdup_printf("Cannot find(?!) trash for `%s`.", source );
		ShowFileOperation(mes.s);
		return NULL;
	}	

	gchar * trashinfo = get_trashinfo( (gchar*)source, (gchar*)trash_name );
	if(!trashinfo)
	{
		ClassString mes = g_strdup_printf("Cannot find(?!) trashinfo for `%s`.", source );
		ShowFileOperation(mes.s);
		return NULL;
	}	

	GKeyFile*     kf = g_key_file_new();
	if(g_key_file_load_from_file( kf, trashinfo, G_KEY_FILE_NONE, NULL ))
	{
		gchar * trashinfo_val = g_key_file_get_string( kf, "Trash Info", "Path", NULL );
		dest_name = get_full_restore_name(source, trash_name, trashinfo_val);
		g_free( trashinfo_val );
	} 
	g_key_file_free( kf );
	g_free(trashinfo);

	dirname = g_path_get_dirname( dest_name ); 
	CreateDirInDir(dirname);
	g_free( dirname );

	printf("RESTORE '%s' '%s' \n", source, dest_name);
	return dest_name;
}

//------------------------------------------------------------------------------

static int WriteTrashFile(int fd, const char * buf, const char * filename)
{
	int len=strlen(buf);
	int res=write(fd,buf,len);
	if(res!=len)
	{
		ClassString mes = g_strdup_printf("Not write in '%s'.\n(%d)%s",
		                                  filename,errno,strerror(errno));
		ShowCancel(mes.s);
		return 1;
	}	
	return 0;
}

//------------------------------------------------------------------------------

int CreateTrashFile(const char * trash_filename, const char * source)
{
	struct stat file_stat;
	int fd = open(trash_filename, O_CREAT | O_WRONLY, 0600);
    if( (fd>0) && (!fstat(fd,&file_stat)))
    {
		ClassString temp=g_strdup_printf("[Trash Info]\nPath=%s\n",source);
		if(WriteTrashFile(fd,temp.s,trash_filename)) return 1;
		char str[128];
		strftime (str, sizeof (str), "DeletionDate=%FT%T\n", localtime (&file_stat.st_mtime));
		if(WriteTrashFile(fd,str,trash_filename)) return 1;
		close(fd);
    } else
    {
		ClassString mes = g_strdup_printf("Not open '%s'.\n(%d)%s",
		                                  trash_filename,errno,strerror(errno));

		ShowCancel(mes.s);
		return 1;
    }
	return 0;
}

//------------------------------------------------------------------------------

const char * Filename2TrashDir(const char * fullname)
{
	struct stat file_stat;
	if(lstat(fullname,&file_stat))
	{
		ClassString mes = g_strdup_printf("Error stat file '%s'\n%s.", 
		                              fullname,strerror( errno ) );
		ShowFileOperation(mes.s);
		return 0;
	}	 

	return find_trash_dev((int)(file_stat.st_dev));
}

//-----------------------------------------------------------------------------

gchar * GenerateName(const char * name, const char * dest_dir) 
{
	struct stat file_stat;
	ClassString fullname=g_build_filename(dest_dir,name,NULL);
	int	i=0;
//	printf("lstat %s \n",fullname.s);
	while(!lstat(fullname.s,&file_stat))
	{  
      i++;
      fullname=g_strdup_printf("%s/%s.%d",dest_dir,name,i);
//    	printf("lstat %s \n",fullname.s);		
     }
	if(i==0) return g_strdup(name);
	 else return g_strdup_printf("%s.%d",name,i);
}

//-----------------------------------------------------------------------------

void DeleteFile(char * full_path)
{
	const char * path_trash;

	path_trash=Filename2TrashDir(full_path);

	if(!path_trash) 
	{
		ClassString mes=g_strdup_printf("Корзина не найдена,\n %s\nудалить безвозвратно?",full_path);
		if(DialogYesNo(mes.s))
		{
			GList * l=0;
			l=g_list_append(l,full_path);
			UtilsUnlink(l);
		}	
		return;
	}

	ClassString shortname=g_path_get_basename(full_path);
	ClassString trash_files=g_build_filename(path_trash,"files",NULL);
	ClassString genname=GenerateName(shortname.s,trash_files.s);
	ClassString filename_trashed=g_build_filename(path_trash,"files",genname.s,NULL);

	ClassString infoname=g_strdup_printf("%s/info/%s.trashinfo",path_trash,genname.s); 
    if(CreateTrashFile(infoname.s,full_path)) return;

	if(rename(full_path,filename_trashed.s))
	{
		ClassString mes = g_strdup_printf("Error rename file '%s'\nto trash '%s' \n%s.", 
		                              full_path,filename_trashed.s,strerror( errno ) );
		ShowCancel(mes.s);
	}   
	printf("rename %s %s\n",full_path,filename_trashed.s);
}

//------------------------------------------------------------------------------------------------- 
