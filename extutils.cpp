#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <errno.h>
#include <sys/inotify.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>

#include "tree.h"
#include "disks.h"
#include "utils.h"
#include "fsmonitor.h"


extern GtkProgressBar* ProgressBar;
extern GtkProgressBar* PulseBar;

typedef struct
{
	size_t size;
	size_t done;
	char * path;
} PROGRESS;

static PROGRESS data_find;
static PROGRESS data_copy;

static int PulseVisible;

static pthread_mutex_t BusyProgress;
gpointer ThreadProgress(gpointer data);
gpointer ThreadPulse(gpointer data);

//-----------------------------------------------------------------------------

void StartPulseBar()
{
	struct stat file_stat;
	if(!lstat( PATH_INFO_FIND, &file_stat ) ) 
	{
		unlink(PATH_INFO_FIND);
	}		

	if(mkfifo(PATH_INFO_FIND,0777))
	{
		printf("Error make fifo progress\n");
	}	

	PulseVisible=1;
	ClassString str=g_strdup("");
   	gtk_progress_bar_set_text(PulseBar,str.s);
	gtk_progress_bar_update(PulseBar,0);
	gtk_widget_show((GtkWidget*)PulseBar);
	g_thread_create(ThreadProgress, &data_find, FALSE, NULL);
}

//-----------------------------------------------------------------------------

void StartProgressBar()
{
	struct stat file_stat;
	if(!lstat( PATH_INFO_PROGRESS, &file_stat ) ) 
	{
		unlink(PATH_INFO_PROGRESS);
	}		

	if(mkfifo(PATH_INFO_PROGRESS,0777))
	{
		printf("Error make fifo progress\n");
	}	

	ClassString str=g_strdup("");
   	gtk_progress_bar_set_text(ProgressBar,str.s);
	gtk_progress_bar_update(ProgressBar,0);
	gtk_widget_show((GtkWidget*)ProgressBar);
    g_thread_create(ThreadProgress, &data_copy, FALSE, NULL);
}

//-----------------------------------------------------------------------------

void ExternalListTar(const char * fullname, const char * dest_dir)
{
	CreateDirInDir(dest_dir);
	ClassString com=g_strdup_printf("%s &",util_path);
	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"READ_TAR\n");
	fprintf(f,"%s\n",dest_dir);
	fprintf(f,"%s\n",fullname);
	fclose(f);
	system(com.s);
}

//-----------------------------------------------------------------------------
 
void ExternalCreateDir(const char * dest_dir)
{
	ClassString com=g_strdup_printf("gksudo %s &",util_path);

	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"CREATE_DIR\n");
	fprintf(f,"%s\n",dest_dir);

	fclose(f);
	system(com.s);
	system("sudo -K"); 	
}

//------------------------------------------------------------------------------

void InitExtUtils(void)
{
	pthread_mutex_init(&BusyProgress,NULL);
    data_copy.path=g_strdup(PATH_INFO_PROGRESS);
    data_find.path=g_strdup(PATH_INFO_FIND);	
}

//------------------------------------------------------------------------------

void DrawProgress(void)
{
	ClassString str;
	pthread_mutex_lock(&BusyProgress);	
	double value=double(data_copy.done)/data_copy.size;
    int puls_end= (data_find.done==data_find.size)>0;
    int find_count=data_find.size;
	pthread_mutex_unlock(&BusyProgress);	

	if(PulseVisible)
	{
		if(puls_end)
		{	
			str=g_strdup_printf("Найдено %d",find_count);
	    	gtk_progress_bar_set_text(PulseBar,str.s);
			gtk_progress_bar_update(PulseBar,0);
		} else 	gtk_progress_bar_pulse(GTK_PROGRESS_BAR(PulseBar));
	}	
	if(value==1.0)
	{	
		str=g_strdup_printf("Выполнено");
	} else
	{	
		str=g_strdup_printf("%3.1f",(value*100));
	}	
	if(value>0 && value<=1.0)
	{	
    	gtk_progress_bar_set_text(ProgressBar,str.s);
		gtk_progress_bar_update(ProgressBar,value);
		gtk_widget_show((GtkWidget*)ProgressBar);
	}	
}

//-----------------------------------------------------------------------------

void ExternalFind(const char * mask,const char * text, const char * dest_dir, int mode)
{
	ClearDir(PATH_FIND);
	ClassString com=g_strdup_printf("%s &",util_path);

	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"FIND\n");
	fprintf(f,"%s\n",dest_dir);
	fprintf(f,"%s\n",mask);
	fprintf(f,"%s\n",text);	
	if(mode) fprintf(f,"FOLDER\n"); else fprintf(f,"LIST\n");		
	fclose(f);
    StartPulseBar();
	system(com.s);
}


//------------------------------------------------------------------------------

gpointer ThreadProgress(gpointer _data)
{
    PROGRESS * data=(PROGRESS*)_data;
	long int all=1;
    long int done=0;	
	int len;
	char buf[128];
	int size;
    int progress_id=open(data->path,O_RDONLY);
	while(progress_id>0)
	{		
		size=read(progress_id,&buf[len],1);
		if(size>0)
		{	
		  	len+=size;
			if(buf[len-1]=='\n')
		    { 
				buf[len-1]=0;
				sscanf(buf,"%ld %ld",&done,&all);
//				printf("%s\n",buf);
				len=0;
				pthread_mutex_lock(&BusyProgress);	
				data->size=all;
				data->done=done;
				pthread_mutex_unlock(&BusyProgress);	
		    } 
		} else
		{	
			if(size<0)
			{	
				ClassString mes = g_strdup_printf("Error (%d) '%s'",errno,strerror(errno));
				printf("%s\n",mes.s);
			}	
			break;
		}	
	}
	printf("end read %s %d\n",buf,size);
	if(progress_id>0) close(progress_id);
	return 0;
}

//-----------------------------------------------------------------------------
 
void ExternalClearTrash(const char * dest_dir)
{
	ClassString com=g_strdup_printf("gksudo %s &",util_path);
	
	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"CLEAR_TRASH\n");
	fprintf(f,"%s\n",dest_dir);
	fprintf(f,"%s\n",g_get_home_dir());
	fprintf(f,"%d\n",geteuid());	


	fclose(f);
	system(com.s);
	system("sudo -K"); 	
}

//-----------------------------------------------------------------------------
 
void ExternalFileCopy4(uid_t user,int operation,GList * l, const char * dest_dir)
{

	ClassString com;	
	if(user!=getuid())
	{	
		com=g_strdup_printf("gksudo %s &",util_path);
	} else	
	{	
		com=g_strdup_printf("%s &",util_path);
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
    StartProgressBar();
	system(com.s);
	system("sudo -K"); 	
}

//-----------------------------------------------------------------------------
 
void ExternalFileCopy(uid_t user,int operation)
{
	GList * l=PanelGetSelected();
	const char * dest_dir=PanelGetDestDir();
	ExternalFileCopy4(user,operation,l,dest_dir);
}

//-----------------------------------------------------------------------------

