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

static size_t progress_size;
static size_t progress_done;
static int puls_end;

static int PulseVisible;

static pthread_mutex_t BusyProgress;
gpointer ThreadProgress(gpointer data);

//-----------------------------------------------------------------------------

void StartProgressBar()
{
	unlink(PATH_INFO_PROGRESS);
//	if(mknod(PATH_INFO_PROGRESS,010777,0))
	if(mkfifo(PATH_INFO_PROGRESS,0777))
	{
		printf("Error make fifo\n");
	}	
	progress_value=0;
	ClassString str=g_strdup("");
   	gtk_progress_bar_set_text(ProgressBar,str.s);
	gtk_progress_bar_update(ProgressBar,0);
	gtk_widget_show((GtkWidget*)ProgressBar);
	g_thread_create(ThreadProgress, 0, FALSE, NULL);
}

//-----------------------------------------------------------------------------
 
void ExternalFileCopy(uid_t user,int operation)
{
	GList * l=PanelGetSelected();
	const char * dest_dir=PanelGetDestDir();

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
 
void ExternalClearTrash(const char * dest_dir)
{
	ClassString com=g_strdup_printf("gksudo %s &",util_path);
	
	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"CLEAR_TRASH\n");
	fprintf(f,"%s\n",dest_dir);

	fclose(f);
	system(com.s);
	system("sudo -K"); 	
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
//	g_thread_create(ThreadProgress, 0, FALSE, NULL);
}

//------------------------------------------------------------------------------
				progress_value=double(done)/all;

void DrawProgress(void)
{
	ClassString str;
	pthread_mutex_lock(&BusyProgress);	
    double value=progress_value;
    int puls=puls_end;
	progress_value=0;
	pthread_mutex_unlock(&BusyProgress);	

	if(PulseVisible)
	{
		if(puls)
		{	
			str=g_strdup_printf("Выполнено");
	    	gtk_progress_bar_set_text(PulseBar,str.s);
			gtk_progress_bar_update(PulseBar,value);
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

gpointer async_lengthy_func(gpointer data)
{
    long int all=1;
    long int done=0;	

	int progress_id=open(PATH_INFO_FIND,O_RDONLY);	
	int len;
	char buf[16];

	while(1)
	{		
		int size=read(progress_id,&buf[len],1);
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
				puls_end=(done==all);
				pthread_mutex_unlock(&BusyProgress);
		    } 
		}
	}
    close(progress_id);
	return 0;
}

//-----------------------------------------------------------------------------

void StartPulseBar()
{
	PulseVisible=1;
    puls_end=0;
	ClassString str=g_strdup("");
   	gtk_progress_bar_set_text(PulseBar,str.s);
	gtk_progress_bar_update(PulseBar,0);
	gtk_widget_show((GtkWidget*)PulseBar);
	g_thread_create(async_lengthy_func, 0, FALSE, NULL);
}

//-----------------------------------------------------------------------------

void ExternalFind(const char * mask,const char * text, const char * dest_dir)
{
	ClearDir(PATH_FIND);
	ClassString com=g_strdup_printf("%s &",util_path);

	FILE * f=fopen("/tmp/billfm.txt","w+"); 
	fprintf(f,"FIND\n");
	fprintf(f,"%s\n",dest_dir);
	fprintf(f,"%s\n",mask);
	fprintf(f,"%s\n",text);	
	fclose(f);
    StartPulseBar();
	system(com.s);
}


//------------------------------------------------------------------------------

gpointer ThreadProgress(gpointer data)
{
    long int all=1;
    long int done=0;	
	int progress_id=open(PATH_INFO_PROGRESS,O_RDONLY);	
	int len;
	char buf[128];
	int size;
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
				progress_size=all;
				progress_done=done;
				pthread_mutex_unlock(&BusyProgress);	
		    } 
		} else break;
//		if(size<0) break;
//		else sleep(1);
	}
	printf("end read %s %d\n",buf,size);
	close(progress_id);
	return 0;
}

//-----------------------------------------------------------------------------