#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <errno.h>
#include <sys/inotify.h>
#include <pthread.h> 
#include <iostream>


#include "fsmonitor.h"

#define COUNT_FOR_STOP_MONITOR     50
#define COUNT_FOR_END_OPERATION    10
#define COUNT_FS_MONITOR           2
#define COUNT_STATIC_FS_MONITOR    2

#define EVENT_SIZE  ( sizeof (struct inotify_event) )
#define BUF_LEN     ( 1024 * ( EVENT_SIZE + 16 ) )

typedef struct
{
	gchar * path;
	GList * list_file;
    int count_change;
    int old_count_change;
	int wd;
	int fd;
	int num;
	int InDropbox;	
} FolderMonitor;

static pthread_mutex_t BusyMonitor;

static FolderMonitor * FsMonitor[COUNT_FS_MONITOR + COUNT_STATIC_FS_MONITOR];
static void * ThreadMonitor(void * arg);

//-----------------------------------------------------------------------------

int InsertFile(FolderMonitor*m, gchar * fullname, const char * mode)
{
	GList* l=g_list_first(m->list_file);

	while(l!= NULL)
	{
		gchar * name=(gchar *)l->data;
		if(name && !strcmp(name,fullname))
		{
			return 0;
		}
       l = (GList*)g_slist_next(l);
	}

	m->list_file=g_list_append(m->list_file,g_strdup(mode));
	m->list_file=g_list_append(m->list_file,fullname);
    m->count_change++;
	return 1;
}

//-----------------------------------------------------------------------------

static void FsMonitorCreateFiles(int num)
{
    FolderMonitor * monitor=FsMonitor[num];
	GList* l=g_list_first(monitor->list_file);

	while (l!= NULL)
	{
		if(l->data && !strcmp((const char *)l->data,"create"))
		{	
			l = (GList*)g_slist_next(l);
			if(l->data)	PanelInsertItem(num,(const char*)l->data);
			l = (GList*)g_slist_next(l);
		}	else
		if(l->data && !strcmp((const char *)l->data,"delete"))
		{	
			l = (GList*)g_slist_next(l);
	   		if(l->data)	PanelDeleteItem(num,(const char*)l->data);
			l = (GList*)g_slist_next(l);
		}	

	}
	ClearListStrings(&monitor->list_file);
}

//-----------------------------------------------------------------------------

void FsMonitorDone(void)
{
	pthread_mutex_lock(&BusyMonitor);

	for(int i=0;i<COUNT_FS_MONITOR;i++)
	{

		FolderMonitor * m=FsMonitor[i];
		if(m->count_change-m->old_count_change>COUNT_FOR_END_OPERATION)
    	{
		  m->old_count_change=m->count_change;
          continue; 
	    }	
        if(m->count_change<COUNT_FOR_STOP_MONITOR)
		{	
			FsMonitorCreateFiles(i);
			m->count_change=0;
	        m->old_count_change=0;
		} else 
		{
			printf("m->count_change=%d\n",m->count_change);
			pthread_mutex_unlock(&BusyMonitor);
			PanelReload(i);
        	pthread_mutex_lock(&BusyMonitor);			
		}
	}	

	pthread_mutex_unlock(&BusyMonitor);
}

//----------------------------------------------------------------------------- 

void FsMonitorInit(void)
{

	pthread_t thread_monitor_id[COUNT_FS_MONITOR + COUNT_STATIC_FS_MONITOR];
	pthread_mutex_init(&BusyMonitor,NULL);
	for(int i=0;i<COUNT_FS_MONITOR + COUNT_STATIC_FS_MONITOR;i++)
	{	
 	    FolderMonitor * monitor=FsMonitor[i]=(FolderMonitor*)malloc(sizeof(FolderMonitor));
	 	memset(monitor,0,sizeof(FolderMonitor));
		monitor->num=i;
		monitor->fd = inotify_init();
    	if(monitor->fd<0)
		{
          printf("Error inotify_init [%d]\n",i);
		}	
		else pthread_create(&thread_monitor_id[i], NULL,ThreadMonitor, monitor);
	}

	FolderMonitor * monitor;
	
	pthread_mutex_lock(&BusyMonitor);
	monitor=FsMonitor[2];
	monitor->path=g_strdup("/etc/mtab");
	monitor->wd = inotify_add_watch(monitor->fd,monitor->path,IN_ALL_EVENTS);

	monitor=FsMonitor[3];
//	monitor->path = g_build_filename(g_get_home_dir(),".gvfs", NULL );
	monitor->path = g_build_filename(g_get_home_dir(),".local/share/Trash/files", NULL );	
	monitor->wd = inotify_add_watch(monitor->fd,monitor->path,IN_ALL_EVENTS);
	pthread_mutex_unlock(&BusyMonitor);	
}

//-----------------------------------------------------------------------------
//int count;

static void * ThreadMonitor(void * arg)
{
	FolderMonitor * monitor=(FolderMonitor *)arg;
	int length, i = 0;
	char buffer[BUF_LEN];

//	int count=0;

	while(1)
	{
//        sleep(1);
		length = read( monitor->fd, buffer, BUF_LEN );  
//		if ( length < 0 ) 	perror( "Read:" );
		i=0;
		while ( i < length ) 
		{
			struct inotify_event *event = ( struct inotify_event * ) &buffer[ i ];
			if(monitor->num==3)
			{
				if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) 
				{
//					printf("create %d %x %d\n", count++,event->mask,event->len);
                    monitor->count_change++;
				} else
				if ( event->mask & (IN_DELETE | IN_MOVED_FROM)) 
				{
//					printf("delete %d %x %d\n", count++,event->mask,event->len);
                    monitor->count_change++;					
				} //else printf("event %d %x %d\n", count++,event->mask,event->len);
			}
			if(monitor->num==2)
			{
//				if ( event->mask & (IN_DELETE_SELF | IN_IGNORED))				
				if ( event->mask & IN_IGNORED)
				{	
//					printf("IN_DELETE_SELF \n");
					pthread_mutex_lock(&BusyMonitor);
					monitor->wd = inotify_add_watch(monitor->fd,monitor->path,IN_ALL_EVENTS);
                    monitor->count_change++;
					pthread_mutex_unlock(&BusyMonitor);	
				}	else 
				if ( event->mask & IN_MODIFY)
				{	
//					printf("MODIFY\n");
					pthread_mutex_lock(&BusyMonitor);
                    monitor->count_change++;
					pthread_mutex_unlock(&BusyMonitor);
				} // else printf("test %d %x %d\n", count++,event->mask,event->len);
				
			}	
			if ( event->len ) 
			{
				if(event->mask & (IN_CREATE | IN_MOVED_TO | IN_MODIFY)) 
				{
					pthread_mutex_lock(&BusyMonitor);
					gchar * fullname=g_build_filename(monitor->path,event->name,NULL);
					InsertFile(monitor, fullname, "create");
//					monitor->file_create=g_list_append(monitor->file_create,g_strdup("create"));
//					monitor->file_create=g_list_append(monitor->file_create,fullname);
//					monitor->count_change++;
					pthread_mutex_unlock(&BusyMonitor);
				} else 
				if ( event->mask & (IN_DELETE | IN_MOVED_FROM)) 
				{
					pthread_mutex_lock(&BusyMonitor);
					gchar * fullname=g_build_filename(monitor->path,event->name,NULL);
					InsertFile(monitor, fullname, "delete");					
//					monitor->file_create=g_list_append(monitor->file_create,g_strdup("delete"));
//					monitor->file_create=g_list_append(monitor->file_create,fullname);
//                    monitor->count_change++;
					pthread_mutex_unlock(&BusyMonitor);
				}
					
		    }
    		i += EVENT_SIZE + event->len;
	  }

	}
	return NULL;
} 

//-----------------------------------------------------------------------------

void FsMonitorChangePath(int num,const char * fullname, int InDropbox)
{
	if(num>COUNT_FS_MONITOR) return;
	int flags=IN_CREATE | IN_DELETE | IN_MOVE | (IN_MODIFY*InDropbox);
	
	pthread_mutex_lock(&BusyMonitor);
    FolderMonitor * monitor=FsMonitor[num];
	
	if(monitor->path) g_free(monitor->path);
	monitor->path=g_strdup(fullname);
	ClearListStrings(&monitor->list_file);
	if(monitor->fd && monitor->wd) inotify_rm_watch(monitor->fd,monitor->wd);
	monitor->wd = inotify_add_watch(monitor->fd,monitor->path,flags);
    monitor->count_change=0;
    monitor->old_count_change=0;
    monitor->InDropbox=InDropbox;
	pthread_mutex_unlock(&BusyMonitor);	
}

//-----------------------------------------------------------------------------

int FsMonitorChangeMtab(void)
{
    int res;
    FolderMonitor * monitor=FsMonitor[2];
	pthread_mutex_lock(&BusyMonitor);

	res=monitor->count_change;
	monitor->count_change=0;

	monitor=FsMonitor[3];
	res+=monitor->count_change;
	monitor->count_change=0;
	
	pthread_mutex_unlock(&BusyMonitor);
	return res;
}

//-----------------------------------------------------------------------------