#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <sys/stat.h>
#include <sys/vfs.h>
#include <unistd.h>
#include <sys/types.h>

#include "utils.h"
#include "disks.h"

GList * List_disk;

//-----------------------------------------------------------------------------

static int sel(const struct dirent * d)
{
  return 1; // всегда подтверждаем
}

//-----------------------------------------------------------------------------

gchar * pad040(const char * s)
{
	gchar * str=g_strdup(s);
	char * p;
	while(1)
	{	
		p=strstr(str,"\\040");
		if(!p) break;
		*p=0x20;
		*(p+1)=0;
		gchar * newstr=g_strdup_printf("%s%s",str,&p[4]);
		g_free(str);
		str=newstr;
	}	
 return str;
}

//-----------------------------------------------------------------------------

#define	EXT_SUPER_MAGIC       0x137D
#define	EXT2_OLD_SUPER_MAGIC  0xEF51
#define	EXT2_SUPER_MAGIC      0xEF53
#define	EXT3_SUPER_MAGIC      0xEF53

int GetTypeFS(const char * source)
{
	struct statfs fs;
	if(!statfs(source,&fs))
	{	
//		printf("Read TypeFS 0x%X %s \n",fs.f_type,source);
		if(0x65735546==fs.f_type) return TYPE_FS_NTFS;
		if(EXT2_SUPER_MAGIC==fs.f_type) return TYPE_FS_EXT4;
		if(EXT3_SUPER_MAGIC==fs.f_type) return TYPE_FS_EXT4;
		if(EXT_SUPER_MAGIC==fs.f_type) return TYPE_FS_EXT4;
		if(EXT2_OLD_SUPER_MAGIC==fs.f_type) return TYPE_FS_EXT4;
		return TYPE_FS_UNKNOW;
	}	else
	{	
		printf("Error read TypeFS %s\n",source);
		return TYPE_FS_EXT4;
	}	
}

GList * GetListDisk(void)
{
	return List_disk;
}

//-----------------------------------------------------------------------------

InfoDisk * GetInfo(const char * source)
{
	InfoDisk * info;
	GList* l=g_list_first(List_disk);

	while (l!= NULL)
	{
		info=(InfoDisk *)l->data;
		if(!strcmp(source,info->path))
		{
			return info;
		}
       l = (GList*)g_slist_next(l);
	}
	return 0;
}

//-----------------------------------------------------------------------------

InfoDisk * Mount2Info(const char * source)
{
	InfoDisk * info=0;
	GList* l=g_list_first(List_disk);

	while (l!= NULL)
	{
		info=(InfoDisk *)l->data;
		if(info && info->mount)
		{	
			if(!strcmp(source,info->mount))
			{
 			   return info;
 			}
		}	
       l = (GList*)g_slist_next(l);
	}
 return 0;
}

//-----------------------------------------------------------------------------

void PrintDisk(void)
{
	InfoDisk * info=0;
	GList* l=g_list_first(List_disk);
	printf("Disk table\n");
	while (l!= NULL)
	{
		info=(InfoDisk *)l->data;
		if(info)
		{	
			printf("%s %s %d %d \n",
			       	info->path,
					info->mount,
					info->num_dev,
					info->full);	

		}	
       l = (GList*)g_slist_next(l);
	}
	printf("Disk table end\n");

}

//-----------------------------------------------------------------------------

void ClearList()
{
	InfoDisk * info=0;
	GList* l=g_list_first(List_disk);

	while (l!= NULL)
	{
		info=(InfoDisk *)l->data;
		if(info)
		{	
			if(info->path) g_free(info->path);
			if(info->mount) g_free(info->mount);
			if(info->label) g_free(info->label);
			if(info->path_trash) g_free(info->path_trash);
		}	
       l = (GList*)g_slist_next(l);
	}
	g_list_free(List_disk);
	List_disk=0;
}

//-----------------------------------------------------------------------------

void InitListDisk(void)
{

	ClearList();
	FILE * f;
	char str[1024];
	char temp[1024];

	f=fopen ("/proc/mounts","rt");
	if(!f) return;

	while(fgets(str,1023,f))
	{
		sscanf(str,"%s ",temp);
		if(strncmp("/dev",temp,4) && strncmp("//",temp,2)) continue;

		InfoDisk * info=GetInfo(temp);
		if(!info)
		{	
			info=(InfoDisk*)malloc(sizeof(InfoDisk));
			memset(info,0,sizeof(InfoDisk));
			List_disk=g_list_append(List_disk,info);
			info->path=pad040(temp);
			sscanf(&str[strlen(temp)+1]," %s ",temp);			
			info->mount=pad040(temp);
			struct stat file_stat;
			if(!lstat(info->mount, &file_stat)) info->num_dev=file_stat.st_dev;

			if(!strcmp("/",info->mount))
			{ 
				info->path_trash=g_strdup_printf("%s/.local/share/Trash",g_get_home_dir());
			}
			else info->path_trash=g_strdup_printf("%s/.Trash-%d",info->mount,geteuid());

 			info->path_trash=CheckTrash(info->path_trash);
		}

		float full;
		struct statfs fs;
		if(!statfs(info->mount,&fs))
		{	
			if(0x65735546==fs.f_type) info->fs_type=TYPE_FS_NTFS;
				else info->fs_type=TYPE_FS_EXT4;
			full=(100.0*(fs.f_blocks-fs.f_bavail))/fs.f_blocks;
			info->total_size=fs.f_blocks*fs.f_bsize;
			info->free_size=fs.f_bavail*fs.f_bsize;
			
//			gchar * total_size=showfilesize(fs.f_blocks*fs.f_bsize);
//			gchar * free_size=showfilesize(fs.f_bavail*fs.f_bsize);			
//			printf("%s  %s %s\n",info->mount,total_size,free_size);
			info->full=int(full+0.5);
		}	

	}	
	fclose(f);

	ClassString book = g_strdup("/dev/disk/by-label");	

	struct dirent ** entry;
	int n = scandir(book.s, &entry, sel, alphasort);
	if (n < 0) 
	{
		printf("Error read directory %s\n",book.s);
    }

	for (int i = 0; i < n; i++)
	{
		if(entry[i]->d_name[0]=='.') continue;

		ClassString dest = g_build_filename(book.s, entry[i]->d_name, NULL );	
		dest=g_file_read_link(dest.s,NULL);
		if(dest.s)
		{	
			dest=g_path_get_basename(dest.s);
			dest = g_build_filename("/dev",dest.s, NULL );	
			InfoDisk * info=GetInfo(dest.s);
			if(info && !info->label)	info->label=g_strdup(entry[i]->d_name);
		} else printf("error link %s\n",entry[i]->d_name);
	}

}

//-----------------------------------------------------------------------------

