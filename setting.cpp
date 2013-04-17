#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

#include <gdk/gdk.h>
#include <gdk/gdkkeysyms.h>
#include <errno.h>
#include <sys/inotify.h>

#include "tree.h"
#include "disks.h"
#include "utils.h"
#include "fsmonitor.h"

//-----------------------------------------------------------------------------

#define		SIZE_BUF		256 

extern int mode[TV_COUNT];
extern gchar * DropboxFolder;

void	ReadSettings(void)
{
	ClassString setfile=g_build_filename(config_path,"bilfm.conf",NULL);
	FILE * read_fp = fopen(setfile.s, "rt"); 
	
	if(!read_fp) return;

	char buffer[SIZE_BUF+1];
	char buf[SIZE_BUF+1];
	char tail[SIZE_BUF+1];	
	int val;
	int size_x=-1;
	int size_y=-1;
	int pos_x=-1;
	int pos_y=-1;

	while( fgets(buffer, SIZE_BUF, read_fp))
	{

		tail[0]=0;
		buffer[strlen(buffer)-1]=0;
		sscanf(buffer,"%s %d %s", buf, &val,tail);

		if(!strcmp(buf,"dropbox") && val)//dropbox
		{
			if(tail[0]=='/')
			{
				if(g_file_test(tail,G_FILE_TEST_EXISTS ))
				{			
					DropboxFolder=g_strdup(tail);
				}
			} else
			if(tail[0]=='~')
			{
				DropboxFolder=g_build_filename(g_get_home_dir(),&tail[2],NULL);
				if(!g_file_test(DropboxFolder,G_FILE_TEST_EXISTS )) DropboxFolder=0;
			} 
				
		} else
			
		if(!strcmp(buf,"mode_panel0"))//mode0
		{
			mode[0]=val;
		} else
		if(!strcmp(buf,"mode_panel1"))//mode1
		{
			mode[1]=val;
		} else
			if(!strcmp(buf,"side_panel"))//side 
		{
			ShowSide = 1-(val&1);
		} else
		if(!strcmp(buf,"one_panel"))//one
		{
			ShowOne = 1-(val&1);
		} else
		if(!strcmp(buf,"position_panel"))//pos
		{
			GtkPaned * p=(GtkPaned *)gtk_builder_get_object( builder, "hpaned1" );
			gtk_paned_set_position ( p, val);
		} else
		if(!strcmp(buf,"left_main_window"))
		{
			pos_x = val;
		} else
		if(!strcmp(buf,"top_main_window"))
		{
			pos_y = val;
		} else
		if(!strcmp(buf,"right_main_window"))
		{
			size_x = val;
		} else
		if(!strcmp(buf,"bottom_main_window"))
		{
			size_y = val;
		} 


		if( (size_x>=0) && (size_y>=0) )
		{
			gtk_window_resize( GTK_WINDOW( topWindow ),size_x, size_y );
		}
		if( (pos_x>=0) && (pos_y>=0) )
		{
			gtk_window_move ( GTK_WINDOW( topWindow ),pos_x, pos_y );
		}
	}
	pclose(read_fp);   
}

//-----------------------------------------------------------------------------

void	SaveSetting(void)
{
	ClassString setfile=g_build_filename(config_path,"bilfm.conf",NULL);
	FILE * fp = fopen(setfile.s, "wt+"); 

	char buf[SIZE_BUF+1];

	int size_x=800;
	int size_y=600;
	int pos_x=10;
	int pos_y=10;

//	printf("save %d\n", ShowSide);
	sprintf(buf, "side_panel %d\n", ShowSide);
	fwrite(buf, strlen(buf), 1, fp);
	sprintf(buf, "one_panel %d\n", ShowOne);
	fwrite(buf, strlen(buf), 1, fp);

	GtkPaned * p=(GtkPaned *)gtk_builder_get_object( builder, "hpaned1" );
	sprintf(buf, "position_panel %d\n", gtk_paned_get_position (p) );
	fwrite(buf, strlen(buf), 1, fp);
	
	gtk_window_get_position ( GTK_WINDOW( topWindow ),&pos_x, &pos_y ); 
	gtk_window_get_size( GTK_WINDOW( topWindow ),&size_x, &size_y );

	sprintf(buf, "left_main_window %d\n", pos_x);
	fwrite(buf, strlen(buf), 1, fp);

	sprintf(buf, "top_main_window %d\n", pos_y);
	fwrite(buf, strlen(buf), 1, fp);

	sprintf(buf, "right_main_window %d\n", size_x);
	fwrite(buf, strlen(buf), 1, fp);

	sprintf(buf, "bottom_main_window %d\n", size_y);
	fwrite(buf, strlen(buf), 1, fp);

	sprintf(buf, "bottom_main_window %d\n", size_y);
	fwrite(buf, strlen(buf), 1, fp);
	
	sprintf(buf, "mode_panel0 %d\n", Panels[0]->MyMode);
	fwrite(buf, strlen(buf), 1, fp);

	sprintf(buf, "mode_panel1 %d\n", Panels[1]->MyMode);
	fwrite(buf, strlen(buf), 1, fp);

	if(DropboxFolder)
	{
		sprintf(buf, "dropbox 1 %s\n", DropboxFolder);
		fwrite(buf, strlen(buf), 1, fp);
	}	

	pclose( fp );   
}

//-----------------------------------------------------------------------------