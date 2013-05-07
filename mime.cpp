#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>

#include "tree.h"
#include "disks.h"
#include "utils.h"

#define	MAX_DEFAULT_ICON	256

static int IndexIcon=FIRST_FREE_INDEX; 

GdkPixbuf* IconPixbuf[MAX_DEFAULT_ICON];
GdkPixbuf* PixbufEmblemLink;
GdkPixbuf* PixbufEmblemDropboxOk;
GdkPixbuf* PixbufEmblemDropboxSync;

static GList * list_mime=NULL;
static GList * list_icon=NULL;
static GList * list_userbin=NULL;

typedef struct _InfoIcon
{
	gchar * name;
	int  icon_index;
} InfoIcon;

const char desktop_entry_name[] = "Desktop Entry";

int	InsertIconName(const char * name);
gchar * GetExt(const char * fullname);
static int GetMimeForFile(const char * name, struct stat * file_stat);
static InfoIcon *  find_userbin(const char * fullname, int enable_insert);
void InsertCommand( char * ext,  char * command);
static int GetMimeClassic(const char * path_folder, const char * full_name);


//-----------------------------------------------------------------------------

//int  find_userbin_mime(const char * p)
//{
// InfoIcon * info=find_userbin(p,1);
//	if(info) return info->icon_index; else return 0;
//}

//-----------------------------------------------------------------------------

static int IsExecute(const char * name, struct stat * file_stat, int dialog)
{

	if(S_ISDIR(file_stat->st_mode))		return 1;

	int res=ICON_GNOME_TEXT;
	int fs = GetTypeFS(name);

	if( (fs==TYPE_FS_EXT3 || fs==TYPE_FS_EXT4) && ((S_IXGRP | S_IXOTH | S_IXUSR) & file_stat->st_mode) )
	{	
 	 	char buffer[16];
		FILE * f=fopen(name,"r");
		if(f)
		{	
			fread(buffer,1,sizeof(buffer),f);
			fclose(f);
    	    buffer[15]=0;
			char mask[]="#!";
			if(!strncmp(mask,buffer,2)) return ICON_GNOME_X_SCRIPT;
	        char mask1[5]={0x7F,0x45,0x4C,0x46,0};
			if(!strncmp(mask1,buffer,4)) return ICON_GNOME_X_APP;		
	        char mask2[5]={0x4D,0x5A,0};
			if(!strncmp(mask2,buffer,2)) return ICON_GNOME_X_APP;		
			if(dialog)
			{	
				ClassString mes=g_strdup_printf("Не исполняемый файл c правами на исполнение %s\n sign: \n%s",
				                                name,buffer);
				ShowWarning(mes.s);
			}	
		}
	}
	return res;
}

//-----------------------------------------------------------------------------

gchar * GetDesktopKey(const char * file_name,const char * key_name)
{
	GKeyFile * file = g_key_file_new();
	gchar * value=0;
    if(g_key_file_load_from_file( file, file_name,  G_KEY_FILE_NONE, NULL ))
    {
        value = g_key_file_get_string ( file, desktop_entry_name,key_name, NULL);
    }

    g_key_file_free( file );
	return value;
}

//-----------------------------------------------------------------------------

gchar * GetIconDesktopName(const char * file_name)
{
	return GetDesktopKey(file_name,"Icon");
}

//-----------------------------------------------------------------------------

InfoMime *  find_mime(const char * p)
{ 
	if(!p) return NULL;
	InfoMime * info;
	GList* list=g_list_first (list_mime);

	while (list!= NULL)
	{
		info=(InfoMime *)list->data;
		if(!g_ascii_strcasecmp(p,info->type))
		{
		   return info;
		}
       list = (GList*)g_slist_next(list);
	}
 return NULL;
}
 
//-----------------------------------------------------------------------------

GdkPixbuf * GetIconByName(const char * name, int size)
{
    if(name[0]=='~')
	{	
		ClassString str=g_strdup_printf("%s%s",g_get_home_dir(),&name[1]);
		return gdk_pixbuf_new_from_file_at_scale(str.s,size,size,1,0);
	}	
	if(name[0]=='/')
	{
		return gdk_pixbuf_new_from_file_at_scale(name,size,size,1,0);
	}

	if(name[0]=='.')
	{
		ClassString str=g_strdup_printf("%s%s",app_path,&name[1]);
		return gdk_pixbuf_new_from_file_at_scale(str.s,size,size,1,0);
	}
	
	GtkIconTheme * icon_theme = gtk_icon_theme_get_default();
	GtkIconInfo* inf = gtk_icon_theme_lookup_icon( icon_theme, name, size, 
	                                              GTK_ICON_LOOKUP_USE_BUILTIN );
	if(!inf)
	{
		ClassString ext=GetExt(name);
		if(ext.s)
		{
			gchar * new_name=g_strdup(name);
			new_name[strlen(name)-1-strlen(ext.s)]=0;
			inf = gtk_icon_theme_lookup_icon( icon_theme, new_name, size, 
	                                              GTK_ICON_LOOKUP_USE_BUILTIN );			
		}
	}

	if(inf)
	{
		const char * file = gtk_icon_info_get_filename( inf );
		if(file)
		{	
			GdkPixbuf * pixbuf=gdk_pixbuf_new_from_file( file, NULL );
			if(pixbuf)
			{	
				return gdk_pixbuf_scale_simple(pixbuf,size,size,GDK_INTERP_BILINEAR);
			}	
		}	
			
	} else
	{
		printf("Not found inf %s\n",name);
	}	
		

	return 0;
}

//-----------------------------------------------------------------------------

gchar * GetExt(const char * fullname)
{
    ClassString name=g_path_get_basename(fullname);

	const char * p=strstr(&name.s[1],".");
	const char * p1;
	if(!p) return 0;
	p++;

	while(p && (*p))
	{
		p1=strstr(p,".");
		if(!p1) break;
//		if(p1[1]==0) return 0;
		p1++;
		if(*p1) p=p1; else break;
	}

	p1=p;

	if((strlen(p)>4)  
	   && strcmp(p,"desktop")
	   && strcmp(p,"anjuta")
	   && strcmp(p,"glade")	   
	       ) return 0;
		
	while(*p)
	{
		if(*p<'0' || *p>'9') return g_strdup(p1);
		p++;
	}

	return 0;
}

//-----------------------------------------------------------------------------

int	InsertIconName(const char * name) 
{
	InfoIcon * info;
	GList* list=g_list_first(list_icon);

	while (list!= NULL)
	{
		info=(InfoIcon *)list->data;
		if(!strcmp(name,info->name))
		{
//		printf("find : %s %s %d %d\n",info->name,name,info->icon_index,strcmp(name,info->name));
			return info->icon_index;
		}
       list = (GList*)g_slist_next(list);
	}

	int i=IndexIcon;
	IconPixbuf[i]=GetIconByName(name,ICON_SIZE);
	if(IconPixbuf[i])
	{
		InfoIcon * info=new InfoIcon;
		info->name=g_strdup(name);
		info->icon_index=IndexIcon;
		list_icon=g_list_append(list_icon,info);
//		printf("add icon : %s %d\n",info->name,info->icon_index);
		if(IndexIcon<MAX_DEFAULT_ICON-1) IndexIcon++;
		 else printf("Load max icons !");
		return i;
	} else
	{
		printf("not InsertIconName : %s \n",name);		
	}
	return 0;
}

//-----------------------------------------------------------------------------

GList * GetMimeListCommand(const char *name)
{
	if(!name) return 0; 

	ClassString command;
	gchar * p=GetExt(name);
	if(!p) return 0;

	InfoMime * info= find_mime(p);	
	if(info)
	{
		return g_list_first (info->list_command);
	}
	g_free(p);
	return 0;
}

//-----------------------------------------------------------------------------

void ScanUsrbin(void)
{
    if(list_userbin) return;
	
//	ClassString setfile = g_strdup_printf("%s/.config/billfm/usrbin.txt",g_get_home_dir());
	ClassString setfile=g_build_filename(config_path,"usrbin.txt",NULL);
	FILE * f = fopen (setfile.s,"rt");
	if(!f)
	{	
		printf("Not open %s\n",setfile.s);
		return;
	}	
	char str[256];
	char name[256];

	while(fgets(str,255,f))
	{
		name[0]=0;
		int mime;
		sscanf(str,"%d %s ",&mime,name);
		if(!strlen(name)) break;

			InfoIcon * info=find_userbin(name,0);
			if(info)
			{
				printf("File %s already exists in userbin.txt\n",name);
			} else
			{	
				InfoIcon * info=new InfoIcon;
				info->name=g_strdup(name);
                info->icon_index=mime;
				list_userbin=g_list_append(list_userbin,info);
			}
	}
}

//-----------------------------------------------------------------------------

static int GetMimeClassic(const char * path_folder, const char * full_name)
{

	struct stat file_stat;

	if(lstat( full_name, &file_stat ))	return ICON_GNOME_TEXT;

	if(S_ISDIR(file_stat.st_mode))	return ICON_GNOME_FOLDER;

	int mime_type=ICON_GNOME_TEXT;

	if(S_ISLNK(file_stat.st_mode))
	{	
		ClassString link=g_file_read_link(full_name,NULL);
		if(link.s)
		{	
			if(link.s[0]!='/')	link = g_build_filename(path_folder,link.s,NULL);					
		}
		if(link.s && !lstat( link.s, &file_stat ) ) 
		{
			mime_type=GetMimeForFile(link.s,&file_stat);
			mime_type|=MIME_FLAG_LINK;
		}	else
		{
			mime_type=ICON_GNOME_BREAK_LINK;
			mime_type|=MIME_FLAG_LINK;					
			printf("error l=%s n=%s\n",link.s,full_name);
		}
 	} else mime_type=GetMimeForFile(full_name,&file_stat);


	return mime_type;
}

//-----------------------------------------------------------------------------

static int GetMimeForFile(const char * name, struct stat * file_stat)
{

	if(S_ISDIR(file_stat->st_mode))	return 1;
	if(S_ISFIFO(file_stat->st_mode))	return 0;
	ClassString p=GetExt(name);
	if(p.s)
	{	
		if(!strcmp(p.s,"desktop"))
		{
			ClassString s=GetIconDesktopName(name);
			if(!s.s)
			{	
				return 0;
			}	
			return InsertIconName(s.s);
		}
		InfoMime * info= find_mime(p.s);	
		if(info)
		{
			if(info->icon_index) return info->icon_index;
		}
	}

	return IsExecute(name,file_stat,0);
}

//-----------------------------------------------------------------------------

InfoIcon *  find_userbin(const char * fullname, int enable_insert)
{ 
	if(!fullname) return NULL;
	InfoIcon * info;
	GList* list=g_list_first (list_userbin);

	while (list!= NULL)
	{
		info=(InfoIcon *)list->data;
		if(!strcmp(fullname,info->name))
		{
		   return info;
		}
       list = (GList*)g_slist_next(list);
	}

	if(!enable_insert) return NULL;
	
//	ClassString setfile = g_strdup_printf("%s/.config/billfm/usrbin.txt",g_get_home_dir());
	ClassString setfile=g_build_filename(config_path,"usrbin.txt",NULL);
	FILE * f = fopen (setfile.s,"a+");
	if(!f)
	{	
		printf("Not open %s\n",setfile.s);
		return NULL;
	}	
	int mime=GetMimeClassic("/usr/bin",fullname);
	ClassString str=g_strdup_printf("%d %s\n",mime,fullname);
	fwrite(str.s,strlen(str.s),1,f);

	info=new InfoIcon;
	info->name=g_strdup(fullname);
    info->icon_index=mime;
	list_userbin=g_list_append(list_userbin,info);

	fclose(f);
	return info;
}

//----------------------------------------------------------------------------- 
 
int  GetDeviceType(const char * _dev)
{
	ClassString buf=g_strdup(_dev);
    gchar * dev=buf.s+5;
	int optical=1;
	int mime_type=ICON_GNOME_HARD;
	struct stat file_stat;
    char format[]="/sys/block/%s/removable";
	char temp[64];

	sprintf(temp,format,dev);
	if(lstat( temp, &file_stat ) ) 
	{
		dev[strlen(dev)-1]=0;
       	sprintf(temp,format,dev);    		
   		optical=0;
	}		

	if(!lstat( temp, &file_stat )) 
	{
		FILE * f=fopen(temp,"rt");
		char buffer[16];
		fread(buffer,1,10,f);
		fclose(f); 
		buffer[1]=0;

		if(buffer[0]=='1')
		{	
			if(optical) mime_type=ICON_GNOME_OPTICAL; else mime_type=ICON_GNOME_REMOVABLE;
		}	
	}	
//	g_free(dev);
	return 	mime_type;
}

//-----------------------------------------------------------------------------

gchar * GetNetworkPath(void)
{
	gchar * res=g_strdup_printf("%s/.net",g_get_home_dir()); 
	struct stat filestat;
	if(lstat(res,&filestat))  return 0;
	return res;	
}	

//-----------------------------------------------------------------------------

gchar * GetPathFind(void)
{
	return g_strdup_printf("/tmp/billfm/find"); 
}

//-----------------------------------------------------------------------------

gchar * CheckDesktopFile(const char * name)
{
  if(name[0]=='/') return g_strdup(name); 

  ClassString str=GetExt(name);
  
  if(!str.s || strcmp(str.s,"desktop") )
  {
	  str=g_strdup_printf("%s.desktop",name);	
  } else str=g_strdup(name);
  if(name[0]=='~') str=g_strdup_printf("%s%s",g_get_home_dir(),&str.s[1]);
	else str=g_strdup_printf("/usr/share/applications/%s",str.s);
	
 return g_strdup(str.s);
}

//-----------------------------------------------------------------------------

gchar * GetDesktopCommand(const char * name)
{
    ClassString fullname=CheckDesktopFile(name);
	gchar * command=GetDesktopKey(fullname.s,"Exec");
	if(command)
	{	
		char * p=strstr(command,"%");
		if(p) *p=0;
		return command;
	}
	return 0;
}

//-----------------------------------------------------------------------------

void InsertCommand( char * ext, char * fullname)
{
	InfoMime * info=find_mime(ext);
	if(!info)
	{
//		printf("Load new type %s - %s\n",ext,fullname );
		info=new InfoMime;
		info->list_command=NULL;
		info->type=g_strdup(ext);
		info->icon_index=0;
		list_mime=g_list_append(list_mime,info);

		ClassString iconname=GetDesktopKey(fullname,"Icon");
		if(iconname.s) info->icon_index=InsertIconName(iconname.s);	else info->icon_index=0;
	}// else printf("add in type %s - %s\n",ext,fullname );

	info->list_command=g_list_append(info->list_command,fullname);
}

//-----------------------------------------------------------------------------

static int LoadedIconDefault;
static int LoadIcon(const char * buf)
{ 
	if(buf[0]==';') return 0;
	int i=LoadedIconDefault++;
	if(i<FIRST_FREE_INDEX)	IconPixbuf[i]=GetIconByName(buf,ICON_SIZE);
	return 0;
}
 
void init_default_icon(void)
{
	ClassString setfile=g_build_filename(config_path,"default-icon.txt",NULL);
	LoadGets(setfile.s,LoadIcon);
    setfile=g_build_filename(config_path,"/icons/emblem-symbolic-link.png",NULL);
	PixbufEmblemLink=gdk_pixbuf_new_from_file_at_scale(setfile.s,8,8,1,0);
}

//-----------------------------------------------------------------------------

static int LoadCommand(const char * buf)
{
	if(buf[0]==';') return 0;
	char ext[strlen(buf)+1];
	char com[strlen(buf)+1];
	ext[0]=0;
	com[0]=0;		
	sscanf(buf," %s %s",ext,com);
	if((ext[0]==0) || (com[0]==0)) return 0;
	gchar * fullname=CheckDesktopFile(com);
	if(fullname) InsertCommand(ext,fullname);
	return 0;
}

//-----------------------------------------------------------------------------

void ScanMime(void)
{
	init_default_icon();

//	ClassString setfile = g_strdup_printf("%s/.config/billfm/mime2command.txt",g_get_home_dir());
	ClassString setfile=g_build_filename(config_path,"mime2command.txt",NULL);
	LoadGets(setfile.s,LoadCommand);

	ScanUsrbin();	
}

//-----------------------------------------------------------------------------

gchar * GetArchivePath(void)
{
	return g_strdup_printf("/tmp/billfm/archives");
}

//-----------------------------------------------------------------------------

int IsArchve(const char * fullname)
{
	if(IsTar(fullname)) return 1;
	if(IsZip(fullname)) return 1;
	if(IsRar(fullname)) return 1;
	if(IsDeb(fullname)) return 1;	
	return 0;
}

//-----------------------------------------------------------------------------

void ClickExecFile(const char * name, struct stat * file_stat)
{
	 
	if(IsArchve(name))
	{   
		const char * dirname=Panels[ActivePanel]->SetArchiveName(name);
		ExternalListTar(name,dirname);
		Panels[ActivePanel]->LoadDir(dirname);
		return;		
	}	

	ClassString command;
	gchar * p=GetExt(name);
	if(p)
	{	
		if(!strcmp(p,"desktop"))
		{
			ClassString  command=GetDesktopKey(name,"Exec");
			command = g_strdup_printf("%s &",command.s);
			system(command.s);
			return;
		}	

		InfoMime * info= find_mime(p);	
		if(info)
		{
			GList *list=g_list_first (info->list_command);
			if(list)
			{	
				command=GetDesktopCommand((gchar*)list->data);
				if(command.s)
				{
					command=g_strdup_printf("%s '%s' &",command.s,name);
				}	else printf("Не найдена команда %s\n",(gchar*)list->data);
				
			}	
		}
		g_free(p);
	} else
	
	if(IsExecute(name,file_stat,1))
	{
		command = g_strdup_printf("\"%s\" &",name);
	}

	if(command.s) system(command.s);
}

//-----------------------------------------------------------------------------

int GetMime(const char * dirname, const char * fullname)
{
	if(!strcmp(dirname,"/usr/bin")) 
	{ 
		 InfoIcon * info=find_userbin(fullname,1);
  		 if(info) return info->icon_index; 
		  else return GetMimeClassic(dirname,fullname);
	}

	if(!InMenuPath(fullname))
	{
		int mime_type=GetMimeClassic(dirname,fullname);
		mime_type&=0xFF;
    	return mime_type;		
	}	
		
//	int mime_type=GetMimeClassic(dirname,fullname);
//	int type = mime_type&0xFF;
//	int flags = mime_type & (~0xFF);
	
	ClassString net_path=GetNetworkPath();
	if(net_path.s && !strncmp(net_path.s,dirname,strlen(net_path.s)) /*&& (type==ICON_GNOME_FS_FOLDER)*/)
	{	
		int mime_type=ICON_GNOME_TEXT;	
		ClassString is_domain=g_path_get_dirname(fullname);
		ClassString is_computer=g_path_get_dirname(is_domain.s);
		ClassString is_shared=g_path_get_dirname(is_computer.s);

		if(!strcmp(is_domain.s,net_path.s)) 
		{
			mime_type=ICON_GNOME_NETWORK;// + flags;
		} else
		if(!strcmp(is_computer.s,net_path.s)) 
		{
			mime_type=ICON_GNOME_COMPUTER;// + flags;
		} else
		if(!strcmp(is_shared.s,net_path.s)) 
		{
			mime_type=ICON_GNOME_SMB_SHARE;// + flags;
		} else
		{	
			mime_type=GetMimeClassic(dirname,fullname);
		}
		return mime_type;		
	}
	
	return GetMimeClassic(dirname,fullname);
}

//-----------------------------------------------------------------------------
