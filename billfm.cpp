//fusesmb /home/vik/.net
//setvbuf
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

int SelectedSidePanel;
GtkProgressBar* ProgressBarCommon;
static pthread_mutex_t BusyProgress;
static void * ThreadProgress(void * arg);

typedef struct TPathButton
{
	GtkButton * button;
	gchar * fullpath;
	gchar * title;
	TPathButton * prev;
	TPathButton * next;	
} TPathButton; 


GList * PathButtons;

ClassPanel * Panels[TV_COUNT];
int mode[TV_COUNT];

ClassSidePanel side_panel;

GtkBuilder* 	builder;
GtkStatusbar* 	StatusBar;

GtkWidget* 	SidePanel;
GtkWidget* 	OnePanel;
GtkWindow*	topWindow;         
GtkEntry * EntryPath;

int ActivePanel=1;

gboolean OnSecondTimer(gpointer data);
int path_entry_try_load(const char * fullname);

void * ThreadCopy(void * arg);
void DrawProgress(void);
//-----------------------------------------------------------------------------
/*
ёйцукенгшщзхъфывапролджэячсмитьбю
ЁЙЦУКЕНГШЩЗХЪФЫВАПРОЛДЖЭЯЧСМИТЬБЮ
qwertyuiopasdfghjklzxcvbnm
QWERTYUIOPASDFGHJKLZXCVBNM*/

gchar * utf8toupper(const char * s)
{
	gchar * str=g_strdup(s);
	int	i=0;
	while(str[i])
	{	
		int y= 0xFF & int(str[i]);
		if(y&0x80)
		{
			int x= 0xFF & int(str[i+1]);
			if(y==0xD0)
			{
				if((x>=0xB0) && (x<=0xBF))
				{	
					str[i+1]-=0x20; 
				} 
					
			}	else
			if(y==0xD1)				
			{	
				if((x>=0x80) && (x<=0x8F))
				{	
					str[i]=0xd0;
					str[i+1]+=0x20;
				} else
				if(x==0x91) 
				{
					str[i]=0xD0;
					str[i+1]=0x81;
				}	
					
			}	
			i+=2;
		} else
		{
			str[i]=toupper(str[i]);
			i+=1;
		}	
	}	
 return str;
}

//-----------------------------------------------------------------------------

gchar * utf8tolower(const char * s)
{
	gchar * str=g_strdup(s);
	int	i=0;
	while(str[i])
	{	
		int y= 0xFF & int(str[i]);
		if(y&0x80)
		{
			if(y==0xD0)
			{
				int x= 0xFF & int(str[i+1]);
				if((x>=0x90) && (x<=0x9F))
				{	
					str[i+1]+=0x20; 
				}	else
				if((x>=0xA0) && (x<=0xaF))
				{	
					str[i]=0xd1;
					str[i+1]-=0x20;
				}	else
				if(x==0x81) 
				{
					str[i]=0xd1;
					str[i+1]=0x91;
				}	
			}	
			i+=2;
		} else
		{
			str[i]=tolower(str[i]);
			i+=1;
		}	
	}	
 return str;
}

//-----------------------------------------------------------------------------

void LoadDefaultPath( int    argc, char **argv )
{
    if(argc==1) 
	{
		Panels[0]->LoadDir(g_get_home_dir());
		Panels[1]->LoadDir(g_get_home_dir());
	}

	if(argc>1) 
	{
		if(path_entry_try_load((const char *)argv[1]))
		{
			Panels[ActivePanel]->LoadDir(g_get_home_dir());
		}
		Panels[1-ActivePanel]->LoadDir(g_get_home_dir());
	}
		
}	
//	gchar * path = g_strdup();


//-----------------------------------------------------------------------------

void ClearListStrings(GList ** list)
{
	if(list && *list)
	{	
		GList* l=g_list_first(*list);
		while (l!= NULL)
		{
			if(l->data)	g_free(l->data);
			l = (GList*)g_slist_next(l);
		}
		g_list_free(*list);
		*list=NULL;
	}	
}		


//------------------------------------------------------------------------------
//interface for path-entry.cpp
int path_entry_try_load(const char * fullname)
{
	struct stat filestat;
	int res=lstat(fullname,&filestat);
	if(!res) 
   	{
		if(S_ISDIR(filestat.st_mode))
		{	
     		Panels[ActivePanel]->LoadDir(fullname);
		} else
		{
           	ClassString tag = g_path_get_basename(fullname);
			ClassString path=g_path_get_dirname(fullname);
     		Panels[ActivePanel]->LoadDir(path.s);
   			Panels[ActivePanel]->SetCursor(tag.s);
		}	
	}
	return res;
}


char * path_entry_active_path()
{
 return g_strdup(Panels[ActivePanel]->MyPath);
}

//-----------------------------------------------------------------------------

void OnButtonPrintPath( GtkButton* button, TPathButton * data ) 
{
	gchar * path = g_strdup(data->fullpath);
	Panels[ActivePanel]->SavePath=1;
	Panels[ActivePanel]->LoadDir(path);

	if(data->next)
	{
		gchar * tag = g_path_get_basename(data->next->fullpath);
		Panels[ActivePanel]->SetCursor(tag);
		g_free (tag);
	}

}

//-----------------------------------------------------------------------------

static gboolean on_main_window_release( GtkWidget * widget, GdkEventKey* event, gpointer user_data)
{

       	switch (event->keyval)
	{
		case GDK_Shift_L:
		{
			char s3[] = "View F3";
			char s4[] = "Edit F4";
			char s5[] = "Trash F8";			
			SetButtonTitle(0,s3);
			SetButtonTitle(1,s4);
			SetButtonTitle(5,s5);			
		}
		break;
		
	}
    return FALSE;
}

//-----------------------------------------------------------------------------

static gboolean on_main_window_keypress( GtkWidget * widget, GdkEventKey* event, gpointer user_data)
{

	if (event->state & 8)//alt
	{
		switch (event->keyval)
		{
       		case GDK_F7:
			{
				NewSearch(Panels[ActivePanel]->get_path());
				break;
			}
		}
	    return FALSE;
	}
		
	if (event->state & GDK_CONTROL_MASK)
	{

	switch (event->keyval)
	{
    	case GDK_h:
			OnButtonHidden( NULL, 14 );
		break;

    	case GDK_u:
			ClassString p0=g_strdup(Panels[0]->MyPath);
			ClassString p1=g_strdup(Panels[1]->MyPath);				
			Panels[0]->LoadDir(p1.s);
			Panels[1]->LoadDir(p0.s);
			ClickActivePanel(0,0,Panels[1-ActivePanel]);
			break;
	}
	    return FALSE;
	}

	if (event->state & GDK_SHIFT_MASK)
	{

		switch (event->keyval)
		{
       		case GDK_F3:
       		case GDK_F4:
				OnShiftEdit( NULL, event->keyval==GDK_F4 ? 1 :0 );
			break;

       		case GDK_F7:
				OnShiftSearch(0,0);
			break;

       		case GDK_F8:
				ClassString mes=g_strdup("Удалить безвозвратно?");
				if(DialogYesNo(mes.s))
				{
					GList * l = Panels[ActivePanel]->GetSelectedFiles();
					UtilsUnlink(l);
				}	
			break;
		}
	    return FALSE;
	} 

	switch (event->keyval) 
	{
		case GDK_Right:
			Panels[ActivePanel]->BackPanel();
		break;

		case GDK_Left:
			OnButtonUp(0,Panels[ActivePanel]);
		break;

		case GDK_F2:
			Panels[ActivePanel]->OnButtonEditCell();
		break;

   		case GDK_F3:
			OnButtonEdit( NULL, 0 );
		break;

   		case GDK_F4:
			OnButtonEdit( NULL, 1 );
		break;

   		case GDK_F5:
			OnButtonCopy( NULL, 2 );
		break;

   		case GDK_F6:
			OnButtonMove( NULL, 3 );
		break;

   		case GDK_F7:
			OnButtonNewDir( NULL, 4 );
		break;

   		case GDK_F8:
			OnButtonDelete( NULL, 1 );
		break;

		case GDK_F9:
		{
			ClassString com = g_strdup_printf("lxterminal --working-directory='%s' &", 
			                                  Panels[ActivePanel]->get_path() );
			system(com.s);
		}	
		break;

		case GDK_Delete:
			OnButtonDelete( NULL, 0 );
		break;

		case GDK_Shift_L:
			char s3[] = "New F3";
			char s4[] = "New F4";
			char s5[] = "Unlink F8";				
			SetButtonTitle( 0 , s3 );
			SetButtonTitle( 1 , s4 );
			SetButtonTitle( 5 , s5 );				
			break;
	}	

    return FALSE;
}

//-----------------------------------------------------------------------------

void InitFolder(void)
{
    ClassString str=g_path_get_dirname(PATH_INFO_LOG);
	CreateDirInDir(str.s);
	mknod(PATH_INFO_LOG,010777,0);
//	int log_id=open(PATH_INFO_LOG,O_RDONLY);	
	mknod(PATH_INFO_PROGRESS,010777,0);	
	str = g_strdup_printf("%s/.config/billfm/bookmark",g_get_home_dir());
	CreateDirInDir(str.s);

	str = g_strdup_printf("%s/.config/billfm/setting",g_get_home_dir());
	CreateDirInDir(str.s);

	const char tmp[]="/tmp/billfm/find";
	CreateDirInDir(tmp);
}

//-----------------------------------------------------------------------------

void OnButtonUp( GtkButton* button, void * datauser )
{
	ClassPanel * panel=(ClassPanel *)(datauser);
	ClassString path = g_path_get_dirname( panel->MyPath );
	panel->SavePath=1;
	panel->LoadDir(path.s);
}

//-----------------------------------------------------------------------------

void SetActivePanel(int index)
{
	SelectedSidePanel=0;
	SidePanelSelectDropbox=0;
    Panels[ActivePanel]->SetNotActive();
	ActivePanel=index;
	Panels[ActivePanel]->SetActive();
	DrawPathButton(Panels[ActivePanel]->MyPath,0);
	chdir(Panels[ActivePanel]->MyPath);
}

//-----------------------------------------------------------------------------

void OnButtonChangeActive( GtkButton* button, void * datauser )
{
	SetActivePanel(1-ActivePanel);
}

//-----------------------------------------------------------------------------

void InitPanel(ClassPanel * panel)
{
	gchar * str;
	str = g_strdup_printf("label%d", (panel->MyIndex&1) + 1);
	panel->but_path = (GtkButton*)gtk_builder_get_object( builder, str );
	g_signal_connect( G_OBJECT( panel->but_path ), "clicked", G_CALLBACK(OnButtonUp), (void*)panel );
	g_free(str);

	panel->CreatePanel();

	str = g_strdup_printf("scrolledwindow%d", panel->MyIndex + 1);
	GtkWidget * widget = panel->GetWidget();
	gtk_container_add( GTK_CONTAINER(gtk_builder_get_object( builder, str)), widget );
	g_free(str);

/**/
	gtk_widget_show( widget );	
}


//-----------------------------------------------------------------------------

void fm_main_quit  (void)
{
//	SaveSetting();
	gtk_main_quit();
} 

//-----------------------------------------------------------------------------

void BuildPathBox(GtkWidget * parent)
{
	gboolean homogeneous=0;
    gint     spacing=1;
    gboolean expand=0;
    gboolean fill=0;
	guint    padding=0;

	GtkWidget *box;
	char str[16];
	str[0]=0;
    /* Создаём новый контейнер hbox соответствующий homogeneous
     * и установкам интервала */
    box = gtk_hbox_new (homogeneous, spacing);

	TPathButton * prev=0;
	for(int i=0; i<10; i++)
	{
        TPathButton * data = new TPathButton;
		data->prev=prev;
		data->next=0;
		if(prev) prev->next=data;
		prev=data;
		data->button = (GtkButton *)gtk_button_new();
		gtk_box_pack_start (GTK_BOX (box),(GtkWidget *) data->button, expand, fill, padding);
		gtk_widget_show ((GtkWidget *)data->button);
		data->fullpath = 0;
		if(data->button)
		{
			char * begin=&str[strlen(str)];
			sprintf(begin,"%d ",i);
			gtk_button_set_label(data->button,str ); 
			PathButtons=g_list_prepend( PathButtons, (void*)data);
			g_signal_connect( G_OBJECT( data->button ), "clicked",     G_CALLBACK( OnButtonPrintPath ), data );
		} else break;
	}

    EntryPath = (GtkEntry*)ptk_path_entry_new();
    gtk_box_pack_start (GTK_BOX (box), (GtkWidget*)EntryPath, FALSE, FALSE, 0);
    gtk_widget_show ((GtkWidget*)EntryPath);


    gtk_box_pack_start (GTK_BOX (parent), box, FALSE, FALSE, 0);
    gtk_widget_show (box);

	box = gtk_hbox_new (homogeneous, spacing);
	
	GtkWidget* button = gtk_button_new_with_label("Back");
    gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	g_signal_connect( G_OBJECT(button ), "clicked",     G_CALLBACK( OnButtonBack ), 0 );	

	button = gtk_button_new_with_label("Home");
    gtk_box_pack_start (GTK_BOX (box), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	g_signal_connect( G_OBJECT(button ), "clicked",     G_CALLBACK( OnButtonHome ), 0 );	

	button = gtk_button_new_with_label("Reload");
    gtk_box_pack_end (GTK_BOX (box), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	g_signal_connect( G_OBJECT(button ), "clicked",     G_CALLBACK( OnButtonReload ), 0 );	

	button = gtk_button_new_with_label("Active");
    gtk_box_pack_end (GTK_BOX (box), button, FALSE, FALSE, 0);
	gtk_widget_show(button);
	g_signal_connect( G_OBJECT(button ), "clicked",     G_CALLBACK( OnButtonChangeActive ), 0 );	

	GtkWidget *box1=(GtkWidget *)gtk_builder_get_object( builder, "hbox2");	
    gtk_box_pack_start (GTK_BOX (box1), box, FALSE, FALSE, 0);
    gtk_widget_show (box);
}

//-----------------------------------------------------------------------------

void DrawPathButton(const char * path, int save_path)
{
	gtk_entry_set_text(EntryPath,"");
	TPathButton * info;	
	gchar * str=g_strdup(path);
	GList * d = g_list_first (PathButtons);	

	if((d) && (save_path))
	{
		info =(TPathButton*)d->data;
        if(info->fullpath) 
		{	
			if(!strncmp(info->fullpath,path,strlen(path)))
			{
				return;
			}
		}

	}
	int root_flag=1;
	while(d)
	{
        info =(TPathButton*)d->data;
		gchar * tag = g_path_get_basename(str);
        if(info->fullpath) g_free(info->fullpath);
		info->fullpath=str;
		if(!strcmp(tag,"/"))
		{
			gtk_widget_set_visible((GtkWidget*)info->button, root_flag);
			root_flag=0;
		}	
		 else gtk_widget_set_visible((GtkWidget*)info->button, 1);
		gtk_button_set_label((GtkButton*)info->button, tag); 
		g_free(tag);
		d=g_list_next(d);		
		str=g_path_get_dirname(str);
	}
}

//------------------------------------------------------------------------------

int main( int    argc, char **argv )
{
	GError *error = NULL;
	gtk_init( &argc, &argv );
//	init_default_icon();
    ScanMime();		
	for(int i=0;i<TV_COUNT;i++)
	{	
		Panels[i] = new ClassTreePanel();
		Panels[i]->MyIndex=i;
	}	

	InitListDisk();

	builder = gtk_builder_new();
//	ClassString main_glade = g_strdup_printf("%s/.config/billfm/main.glade",g_get_home_dir());
	ClassString main_glade = g_strdup_printf("%s/billfm/main.glade",g_get_home_dir());
	if( ! gtk_builder_add_from_file( builder, main_glade.s, &error ) )
	{
		g_warning( "%s\n", error->message );
		return 1;
	} 


	InitFolder();


	topWindow = GTK_WINDOW(gtk_builder_get_object(builder, "topWindow"));
	ReadSettings();

	g_signal_connect( G_OBJECT( topWindow ), "destroy",  G_CALLBACK( fm_main_quit ), NULL );

	StatusBar = (GtkStatusbar*) gtk_builder_get_object( builder, "statusbar1");
	g_signal_connect( G_OBJECT( topWindow ), "key-press-event", G_CALLBACK( on_main_window_keypress ), NULL );
	g_signal_connect( G_OBJECT( topWindow ), "key-release-event", G_CALLBACK( on_main_window_release ), NULL );

	FsMonitorInit();	
	for(int i=0;i<TV_COUNT;i++)
	{
		CreatePanel(i,mode[i]);
	}
	
	side_panel.MyIndex=4;
	side_panel.CreatePanel();
	ConnectMemuSignal();

	gchar * str = g_strdup( "scrolledwindow3" );
	SidePanel = (GtkWidget *)gtk_builder_get_object( builder, "vbox_side" );
	GtkWidget * widget = side_panel.GetWidget();
	gtk_container_add( GTK_CONTAINER(gtk_builder_get_object( builder, str)), widget );
	g_free(str);

	g_signal_connect(widget, "button-press-event", (GCallback) OnButtonPressedSidePanel,(void*) &side_panel );

    OnePanel = (GtkWidget *)gtk_builder_get_object( builder, "vbox3");

	InitButton();

	gtk_widget_show_all( GTK_WIDGET (topWindow) );	

	side_panel.LoadDevice();
    GtkWidget *box1=(GtkWidget *)gtk_builder_get_object( builder, "hbox1");

    BuildPathBox (box1);

	LoadDefaultPath(argc,argv); 

	Panels[0]->SetNotActive();

	ProgressBarCommon=(GtkProgressBar *)gtk_builder_get_object( builder, "progressbar1" );
//	InitFarCopy(pb);
	
	gtk_window_set_title(GTK_WINDOW(topWindow), "Bill-file-manager");
	gtk_window_set_icon(GTK_WINDOW(topWindow),GetIconByName("gnome-commander",ICON_SIZE));

	OnMenuOnePanel(0,0);
	OnMenuSidePanel(0,0);

	InitDropbox();

	pthread_t thread_id;
	pthread_mutex_init(&BusyProgress,NULL);
	pthread_create(&thread_id, NULL,ThreadProgress,0);

	g_timeout_add_seconds(1,OnSecondTimer,NULL);

	gtk_main();
	return  0;
}


//------------------------------------------------------------------------------

gboolean OnSecondTimer(gpointer data)
{
 	DrawIconDropbox();
	FsMonitorDone();
    if(FsMonitorChangeMtab())
	{
      side_panel.LoadDevice();
	}
	DrawProgress();
	return TRUE;
}
 

//-----------------------------------------------------------------------------

gboolean ClickActivePanel(GtkWidget* widget, GdkEventFocus* event, gpointer user_data)
{
	SetActivePanel( ((ClassPanel*)user_data)->MyIndex &1);
	return FALSE; 
}

//------------------------------------------------------------------------------

void	OnButtonSudoCopy( GtkButton* button, int index_operation )
{
	ExternalFileCopy(0,TASK_COPY);
}

//-----------------------------------------------------------------------------

void MountGvfsArchive(void)
{
	ClassString filename=Panels[ActivePanel]->GetSelectedItem();
	ClassString mount=g_path_get_basename(filename.s);
	mount=g_build_filename(g_get_home_dir(),".gvfs",mount.s,NULL);
	struct stat file_stat;
	
	if(!lstat( mount.s, &file_stat ) ) 
	{
		if(S_ISDIR(file_stat.st_mode))
		{	
			Panels[ActivePanel]->LoadDir(mount.s);
			return;
		}	
	}		

//	if(!CreateDirInDir(mount.s))
//	{	
		ClassString com = g_strdup_printf("/usr/lib/gvfs/gvfsd-archive file='%s' &",
		                                  filename.s);
//		ClassString com = g_strdup_printf("fuse-zip '%s' '%s'",filename.s,mount.s);
		printf("%s\n",com.s);
		system(com.s);
		printf("%s\n",mount.s);
//		Panels[ActivePanel]->LoadDir(mount.s);
//	}	
	
}

//-----------------------------------------------------------------------------

void	OnButtonTest( GtkButton* button, int index_operation )
{

}

//------------------------------------------------------------------------------
float progress_value;

void DrawProgress(void)
{
	ClassString str;
	pthread_mutex_lock(&BusyProgress);	
    double value=progress_value;
	progress_value=0;
	pthread_mutex_unlock(&BusyProgress);	

	if(value==1.0)
	{	
		str=g_strdup_printf("Выполнено");
	}	
	 else
	{	
		str=g_strdup_printf("%3.1f",(value*100));
	}	
    gtk_progress_bar_set_text(ProgressBarCommon,str.s);
	gtk_progress_bar_update(ProgressBarCommon,value);

	if(value>0)
	{	
		gtk_widget_show((GtkWidget*)ProgressBarCommon);
	} else
	{	
		gtk_widget_hide((GtkWidget*)ProgressBarCommon);
	}	
}

//------------------------------------------------------------------------------

static void * ThreadProgress(void * arg)
{
    long int all=1;
    long int done=0;	
//	printf("Value=%f\n",value);
	int progress_id=open(PATH_INFO_PROGRESS,O_RDONLY);	
	int len;
	char buf[128];

//	while(progress_id>0)
	while(1)
	{		
//		printf("Value while =%f\n",progress_value);
		int size=read(progress_id,&buf[len],1);
    	if(size>0)
		{	
		  	len+=size;
			if(buf[len-1]=='\n')
		    { 
		      buf[len-1]=0;
//		      printf("%s\n",buf);
			  sscanf(buf,"%ld %ld",&done,&all);	
		      len=0;
		    } 
			pthread_mutex_lock(&BusyProgress);	
			progress_value=double(done)/all;
			pthread_mutex_unlock(&BusyProgress);	
		} else sleep(1);
	}
	return 0;
}
