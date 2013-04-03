#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <dirent.h>
#include <errno.h>
#include <utime.h>
#include <unistd.h>

#include "disks.h"
#include "trash.h"
#include "utils.h"

#define BUFFERSIZE 1024

char rw_opt[]="iocharset=utf8,codepage=cp866,file_mode=0777,dir_mode=0777,username=quest,password=''";
char ro_opt[]="iocharset=utf8,codepage=cp866,username=quest,password=''";

GList * list_source=0;
const char * DestDir;
const char tmp[]="/tmp/billfm/find";
const char * SourceTar;
int CountReadStrings;

int PrintTar(const char * buf);
int PrintZip(const char * buf);
int PrintRar(const char * buf);
void LoadSearch(const char * mask,const char * text,const char * dest_dir);

//------------------------------------------------------------------------------

void mkfile(const char * fullname,long int size)
{
	FILE * f;
	f = fopen(fullname, "w");
	if (f == NULL)
	{
		ClassString dir=g_path_get_dirname(fullname);
		CreateDirInDir(dir.s);
		f = fopen(fullname, "w");
		if (f == NULL)
		{
			printf("Невозможно создать файл: %s\n", fullname);
			return;
		}
	}
	const char x[1]={0};
	fwrite(x, 1,1, f);
	fseek(f,size-1,0);
	fwrite(x, 1,1, f);
	fclose(f);
}

//------------------------------------------------------------------------------

void UtilsCreateLink2(const char * source, const char * dest_dir)
{
	ClassString link_name = g_path_get_basename(source);

	ClassString dir_name = g_path_get_dirname(source);
	dir_name = g_build_filename(tmp,dir_name.s,NULL);	
	struct stat file_stat;

	if(lstat(dir_name.s, &file_stat)) 
	{
		ClassString com = g_strdup_printf("mkdir -p '%s'",dir_name.s);
		system(com.s);
	}

	link_name =g_build_filename(dir_name.s,link_name.s, NULL );

	if(symlink(source,link_name.s))
	{   
		ClassString mes = g_strdup_printf("Error create link !\n Source file '%s'\n Link name '%s' \n Error (%d) '%s'",
    	           source,link_name.s,errno,strerror(errno));
		printf("%s\n",mes.s);
	}
}

//------------------------------------------------------------------------------

const char * PanelGetDestDir(void)
{
  return DestDir;
}

//------------------------------------------------------------------------------

GList * PanelGetSelected(void)
{
	return list_source;
}

//------------------------------------------------------------------------------

FILE * f;
char buffer[BUFFERSIZE];

void NexString(void)
{
 if(fgets(buffer, sizeof(buffer), f))
 {
  buffer[strlen(buffer)-1]=0;
 } else 
 {
  printf("Error GetString \n");
  exit(0);
 }
}

//------------------------------------------------------------------------------

void LoadList(void)
{
 while(fgets(buffer, sizeof(buffer), f))
 {
  buffer[strlen(buffer)-1]=0;
  list_source=g_list_append(list_source,g_strdup(buffer));
  printf("copy gets %s\n",buffer);
 }
 printf("copy gets end\n");
}

//------------------------------------------------------------------------------

int main( int    argc, char **argv )
{
 int operation=TASK_COPY;
 gtk_init( &argc, &argv );
 
 InitListDisk();

 f=fopen("/tmp/billfm.txt","rt"); 
 if(!f)
 {
  printf("Not open file !\n");
  exit(0);
 }

 NexString();

 if(!strcmp(buffer,"MOVE"))
 {
   operation=TASK_MOVE;
 } else 
 if(!strcmp(buffer,"COPY"))
 {
   operation=TASK_COPY;
 } else 
 if(!strcmp(buffer,"CREATE_DIR"))
 {
   operation=TASK_CREATE_DIR;
 } else 
 if(!strcmp(buffer,"CLEAR_TRASH"))
 {
   operation=TASK_CLEAR_TRASH;
 } else 
 if(!strcmp(buffer,"FIND"))
 {
   operation=TASK_FIND;
 } else 
 if(!strcmp(buffer,"SMB_MOUNT"))
 {
   operation=TASK_SMB_MOUNT;
 } else 
 if(!strcmp(buffer,"READ_TAR"))
 {
   operation=TASK_READ_TAR;
 } else 
 {
  printf("Operation unknow\n");
  exit(0);
 }

 NexString();
 DestDir=g_strdup(buffer);
// printf("dest %s\n",PanelGetDestDir());

	const char * dest_dir=PanelGetDestDir();
	if(operation==TASK_COPY)
	{
		LoadList();
		DialogCopy(operation, dest_dir,list_source); 
	} else
	if(operation==TASK_MOVE)
	{
		LoadList();
		DialogCopy(operation, dest_dir,list_source); 
	} else
	if(operation==TASK_CREATE_DIR)
	{
		CreateNewDirDialog(dest_dir);
	} else
	if(operation==TASK_CLEAR_TRASH)
	{
		InitListDisk();
		UtilsClearTrash();
	} else
	if(operation==TASK_FIND)
	{
		NexString();
		const char * mask =g_strdup(buffer);
		NexString();
		const char * text =g_strdup(buffer);
		LoadSearch(mask,text,DestDir);
	} else
	if(operation==TASK_SMB_MOUNT)
	{
		NexString();
		const char * source =g_strdup(buffer);
		NexString();
		const char * opt =g_strdup(buffer);
		if(!strcmp(opt,"rw")) opt=rw_opt; else opt=ro_opt;
		ClassString com=g_strdup_printf("mount -t cifs \"%s\" \"%s\" -o %s", source,DestDir,opt);
		printf("%s\n",com.s);
		system(com.s);
	} else
	if(operation==TASK_READ_TAR)
	{
		chdir(DestDir);
		NexString();
		SourceTar =g_strdup(buffer);
		ClassString outfile=g_build_filename(DestDir,"dir.lst",NULL);
		if(IsTar(SourceTar))
		{
			ClassString com=g_strdup_printf("tar -tvf '%s' > dir.lst",SourceTar);
			system(com.s); 
			LoadGets(outfile.s,PrintTar);
		} else
		if(IsZip(SourceTar))
		{
			ClassString com=g_strdup_printf("unzip -l  '%s' > dir.lst",SourceTar);
			system(com.s); 
			ClassString outfile=g_build_filename(DestDir,"dir.lst",NULL);
			CountReadStrings=0;
			LoadGets(outfile.s,PrintZip);
		}
		if(IsRar(SourceTar))
		{
			ClassString com=g_strdup_printf("unrar vt  '%s' > dir.lst",SourceTar);
			system(com.s); 
			ClassString outfile=g_build_filename(DestDir,"dir.lst",NULL);
			CountReadStrings=0;
			LoadGets(outfile.s,PrintRar);
		}

	}
 fclose(f);
}

//------------------------------------------------------------------------------

int PrintTar(const char * buf)
{
	long int size;
	gchar * right;
	gchar * name;
        struct tm ltm;

	gchar * p=(gchar*)buf;
	char str[strlen(buf)+1];

	sscanf(p,"%s",str);
	right=g_strdup(str);

	p=strstr(p,str);
	p+=strlen(str);
	sscanf(p,"%s",str); //owner/group

	p=strstr(p,str);
	p+=strlen(str);
	sscanf(p,"%s",str);
	sscanf(p,"%ld",&size);

	p=strstr(p,str);//date
	p+=strlen(str);
	sscanf(p," %s ",str);
	int y,m,d,h;
	sscanf(p," %d-%d-%d ",&y,&m,&d);
	ltm.tm_year=y-1900;
	ltm.tm_mon=m-1;
	ltm.tm_mday=d;

	p=strstr(p,str);//time
	p+=strlen(str);
	sscanf(p," %s ",str);
	sscanf(p," %d:%d ",&h,&m);
	ltm.tm_hour=h;
	ltm.tm_min=m;
	ltm.tm_sec=0;

	p=strstr(p,str); //name
	p+=strlen(str);
	while(*p==0x20) p++;
	name=g_strdup(p);

	gchar * fullname=g_build_filename(DestDir,name,NULL);

	time_t ut=mktime(&ltm);
	struct utimbuf times;
	times.actime = ut;
        times.modtime = ut;

	if(right[0]=='d') 
	{
		CreateDirInDir(fullname);
	} else
	{
		mkfile(fullname,size);
	        utime(fullname, &times );
	}
	return 0;
} 

//------------------------------------------------------------------------------

int PrintZip(const char * buf)
{
//406  1999-10-12 01:27   rxLib567/RX/RxNews.txt
//  0  2005-04-27 12:07   rxLib567/RX/Units/

	if(CountReadStrings++<3) return 0;
	if(buf[0]=='-') return 1;
	long int size;
	gchar * right;
	gchar * name;
        struct tm ltm;

	gchar * p=(gchar*)buf;
	char str[strlen(buf)+1];
	sscanf(p," %s ",str);
	sscanf(p," %ld ",&size);

	p=strstr(p,str);//date
	p+=strlen(str);
	sscanf(p," %s ",str);
	int y,m,d,h;
	sscanf(p," %d-%d-%d ",&y,&m,&d);
	ltm.tm_year=y-1900;
	ltm.tm_mon=m-1;
	ltm.tm_mday=d;

	p=strstr(p,str);//time
	p+=strlen(str);
	sscanf(p," %s ",str);
	sscanf(p," %d:%d ",&h,&m);
	ltm.tm_hour=h;
	ltm.tm_min=m;
	ltm.tm_sec=0;

	p=strstr(p,str);//name
	p+=strlen(str);
	while(*p==0x20) p++;
	name=g_strdup(p);

	gchar * fullname=g_build_filename(DestDir,name,NULL);

	time_t ut=mktime(&ltm);
	struct utimbuf times;
	times.actime = ut;
        times.modtime = ut;

	if(!size && fullname[strlen(fullname)-1]=='/') 
	{
		CreateDirInDir(fullname);
	} else
	{
		mkfile(fullname,size);
	        utime(fullname, &times );
	}

	return 0;
} 

//------------------------------------------------------------------------------

int PrintRar(const char * buf)
{
//406  1999-10-12 01:27   rxLib567/RX/RxNews.txt
//  0  2005-04-27 12:07   rxLib567/RX/Units/
	static gchar * last;
	if(CountReadStrings==0)
	{
		if(buf[0]=='-')
		{
			last=0;
			CountReadStrings=3;
		}
		return 0;
	}

	CountReadStrings++;

	if(buf[0]=='-') return 1;

	if((CountReadStrings&3)==0)
	{
		if(last) g_free(last);
		last=g_strdup(buf);
//printf("0:%s\n",buf);
		return 0;
	} else 
        if((CountReadStrings&3)==1)
	{
	}
	else
	{
		CountReadStrings++;
		return 0;
	}

	long int size;
	gchar * right;
	gchar * name=last;
	while(*name==0x20) name++; 
        struct tm ltm;

	gchar * p=(gchar*)buf;
	char str[strlen(buf)+1];
	sscanf(p," %s ",str);
	sscanf(p," %ld ",&size);
	p=strstr(p,"%");
	p+=1;
//	printf("%s %ld %s\n",name,size,p);
	int y,m,d,h,mn;
//	sscanf(p," %d-%d-%d ",&d,&m,&y);
	sscanf(p," %d-%d-%d %d:%d",&d,&m,&y,&h,&mn);

	ltm.tm_year=y;
	if(ltm.tm_year<50) ltm.tm_year+=100;
	ltm.tm_mon=m-1;
	ltm.tm_mday=d;
	ltm.tm_hour=h;
	ltm.tm_min=mn;
	ltm.tm_sec=0;

//	printf("'%s' %ld %d-%d-%d %d:%d\n",name,size,ltm.tm_year,ltm.tm_mon,ltm.tm_mday,h,mn);

	gchar * fullname=g_build_filename(DestDir,name,NULL);

	time_t ut=mktime(&ltm);
	struct utimbuf times;
	times.actime = ut;
        times.modtime = ut;

	if(!size) 
	{
		CreateDirInDir(fullname);
	} else
	{
		mkfile(fullname,size);
	        utime(fullname, &times );
	}

	return 0;
} 

//------------------------------------------------------------------------------

void LoadSearch(const char * mask,const char * text,const char * dest_dir)
{
	char buf[128];
	int progress=open(PATH_INFO_FIND,O_WRONLY);
	sprintf(buf,"%d %d\n",0,1);
	write(progress,buf,strlen(buf));                          

	FILE *read_fp;
	#define		SIZE_BUF		1024 
	char buffer[SIZE_BUF];

	ClassString com = g_strdup_printf("find %s -mindepth 0 -type f -name '%s' | xargs grep -l \"%s\" ",
	                                  dest_dir, mask, text );        
	read_fp = popen(com.s, "r");
	int count=0; 	
	while(fgets(buffer, SIZE_BUF, read_fp))
	{
		buffer[strlen(buffer)-1]=0;
		UtilsCreateLink(buffer,tmp);
		count++;
	}

	pclose(read_fp);   
	sprintf(buf,"%d %d\n",count,count);
	write(progress,buf,strlen(buf));                          
	close(progress);
	printf("End search - %d\n",count);
}

//------------------------------------------------------------------------------
