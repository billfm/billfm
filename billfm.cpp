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

typedef void(*BUTTON_CALLBACK)(GtkButton*, int);

int SelectedSidePanel;

GtkProgressBar* ProgressBar;
GtkProgressBar* PulseBar;


static gchar * menu_path;
gchar * app_path;
gchar * config_path;


int InMenuPath(const char * path)
{
	return strncmp(path,menu_path,strlen(menu_path));
}

gchar * GetUtilPath()
{
	return util_path;
}

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
GtkEntry *  EntryPath;

int ActivePanel=1;

gboolean OnSecondTimer(gpointer data);
int path_entry_try_load(const char * fullname);

void * ThreadCopy(void * arg);

//-----------------------------------------------------------------------------

void OnButtonHiddenList( GtkButton* button, int data )
{
	Panels[ActivePanel]->OnlyHiddenList=1-Panels[ActivePanel]->OnlyHiddenList;
	Panels[ActivePanel]->reload();
}

//-----------------------------------------------------------------------------

void OnButtonBlackList( GtkButton* button, int data )
{
	Panels[ActivePanel]->OnlyBlackList=1-Panels[ActivePanel]->OnlyBlackList;
	Panels[ActivePanel]->reload();
}

//-----------------------------------------------------------------------------

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

static gboolean OnPanelKeypress( GtkWidget * widget, GdkEventKey* event, gpointer user_data)
{
	switch (event->keyval) 
	{
		case GDK_Delete:
			OnButtonDelete( NULL, 0 );
		break;

		case GDK_Right:
			OnButtonUp(0,Panels[ActivePanel]);
	    return TRUE;


		case GDK_Left:
			Panels[ActivePanel]->BackPanel();
	    return TRUE;
			
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

void InitFolder(gchar * argv0)
{
	struct stat dest_stat;
	app_path=g_path_get_dirname(argv0);
	printf("app_path %s\n",argv0);	
	ClassString str=g_path_get_dirname(PATH_INFO_LOG);
	CreateDirInDir(str.s);
	mknod(PATH_INFO_LOG,010777,0);
//	mknod(PATH_INFO_PROGRESS,010777,0);
	mknod(PATH_INFO_FIND,010777,0);		

	config_path=g_strdup_printf("%s/.config/billfm",g_get_home_dir());

	if(lstat(config_path, &dest_stat)) 
		config_path=g_strdup_printf("%s/.config/billfm",app_path);
	printf("config_path %s\n",config_path);
    str=g_build_filename(config_path,"bookmark",NULL);
	CreateDirInDir(str.s);

//	str = g_strdup_printf("%s/.config/billfm/setting",g_get_home_dir());
    str=g_build_filename(config_path,"setting",NULL);
	CreateDirInDir(str.s);

	const char tmp[]="/tmp/billfm/find";
	CreateDirInDir(tmp);


    str=g_build_filename(app_path,"copy-gets", NULL );


    if(lstat(str.s, &dest_stat))  util_path=g_strdup("copy-gets");
	 else util_path=g_strdup(str.s);

	printf("util_path %s\n",util_path);

    menu_path=g_build_filename(config_path,"menu",NULL);
	CreateDirInDir(menu_path);
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

void OnButtonChangeActive( GtkButton* button, int data )
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
	g_signal_connect(G_OBJECT(widget), "key-press-event", G_CALLBACK(OnPanelKeypress), (void*)panel);
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

GtkWidget * xpm_box(const char * xpm_filename)
{
    GtkWidget *box;
    GtkWidget *image;
    box = gtk_hbox_new (FALSE, 0);
    gtk_container_set_border_width (GTK_CONTAINER (box), 1);
    image = gtk_image_new_from_file (xpm_filename);
    gtk_box_pack_start (GTK_BOX (box), image, FALSE, FALSE, 1);
    gtk_widget_show (image);
    return box;
}

//-----------------------------------------------------------------------------

void CreateButton(GtkWidget * owner, const char * picname, BUTTON_CALLBACK func)
{
	GtkWidget * button = gtk_button_new();
    gtk_box_pack_start (GTK_BOX (owner), button, FALSE, FALSE, 0);
	GtkWidget* pic = xpm_box(picname);
	gtk_widget_show (pic);
    gtk_container_add (GTK_CONTAINER (button),pic);
	gtk_widget_show(button);
	g_signal_connect( G_OBJECT(button ), "clicked",     G_CALLBACK( func ), 0 );	
}

//-----------------------------------------------------------------------------

void BuildPathBox()
{
    GtkWidget * parent=(GtkWidget *)gtk_builder_get_object( builder, "hbox1");

	gboolean homogeneous=0;
    gint     spacing=1;
    gboolean expand=0;
    gboolean fill=0;
	guint    padding=0;

	GtkWidget * box;
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

	CreateButton(box, ICON_GNOME_BACK, OnButtonBack);
	CreateButton(box, ICON_GNOME_RELOAD, OnButtonReload);
	CreateButton(box, ICON_GNOME_ACTIVE, OnButtonChangeActive);
	CreateButton(box, ICON_GNOME_HIDE_LIST, OnButtonHiddenList);	
	CreateButton(box, ICON_GNOME_BLACK_LIST, OnButtonBlackList);		
	CreateButton(box, ICON_GNOME_SAME_PANEL, OnButtonSame);
	
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
    g_thread_init(NULL);
	gtk_init( &argc, &argv );
	InitFolder(argv[0]);
	
    ScanMime();		
	for(int i=0;i<TV_COUNT;i++)
	{	
		Panels[i] = new ClassTreePanel();
		Panels[i]->MyIndex=i;
	}	
	ClassString uid=g_strdup_printf("%d",geteuid());
	InitListDisk(g_get_home_dir(),uid.s);

	builder=CreateForm("main.glade");
	if(!builder) return -1;
		
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

    BuildPathBox();

	LoadDefaultPath(argc,argv); 

	Panels[0]->SetNotActive();

	ProgressBar=(GtkProgressBar *)gtk_builder_get_object( builder, "progressbar1" );
	PulseBar=(GtkProgressBar *)gtk_builder_get_object( builder, "progressbar2" );	
    gtk_progress_bar_pulse(PulseBar);
	gtk_widget_hide((GtkWidget*)ProgressBar);
	gtk_widget_hide((GtkWidget*)PulseBar);
	
	gtk_window_set_title(GTK_WINDOW(topWindow), "Bill-file-manager");

	gtk_window_set_icon(GTK_WINDOW(topWindow),IconPixbuf[ICON_GNOME_COMMANDER]);	

	OnMenuOnePanel(0,0);
	OnMenuSidePanel(0,0);

	InitDropbox();
    InitExtUtils();
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

//-----------------------------------------------------------------------------
