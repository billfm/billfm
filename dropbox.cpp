#include <gtk/gtk.h>
#include <stdio.h>
#include <stdlib.h>
#include <glib.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <fcntl.h>
#include <errno.h>
#include <unistd.h> 

#include "tree.h"
#include "utils.h"

#define COUNT_DROPBOX_MONITOR           2

GdkPixbuf* IconSync[4];

static int sock;
static GIOChannel * io_channel;

gchar * DropboxFolder=NULL;
int SidePanelSelectDropbox=0;

GList* list_dropbox[COUNT_DROPBOX_MONITOR];

static int UpdateListDropbox(int num);
void ClearListStrings(GList ** list);

static int ReadStatus();


//-----------------------------------------------------------------------------

static gboolean connect_socket()
{
	int flags = 0;
	int retval = -1;
	struct sockaddr_un address;
	socklen_t address_length = 0;
	struct timeval tv;
	sock = -1;

	// Initialize address structure
	memset(&address, 0x0, sizeof(address));
	address.sun_family = AF_UNIX;
	g_snprintf(address.sun_path, sizeof(address.sun_path),
		"%s/.dropbox/command_socket", g_get_home_dir());

	// Calculate the length of the address
	address_length = sizeof(address) - sizeof(address.sun_path) +
		strlen(address.sun_path);

	// Create socket
	retval = socket(PF_UNIX, SOCK_STREAM, 0);
	if(retval < 0)
		goto failed;
	sock = retval;

	// Set connect timeout
	tv.tv_sec = 0;
	tv.tv_usec = 1000 * 50;
	retval = setsockopt(sock, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof(tv));
	if(retval < 0)
		goto failed;
	retval = setsockopt(sock, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof(tv));
	if(retval < 0)
		goto failed;

	// Set native non-blocking, for connect timeout
	retval = fcntl(sock, F_GETFL, 0);
	if(retval < 0)
		goto failed;
	flags = retval;
	retval = fcntl(sock, F_SETFL, flags | O_NONBLOCK);
	if(retval < 0)
		goto failed;

	// Connect
	retval = connect(sock, (struct sockaddr*)&address, address_length);
	if(retval < 0 && errno == EINPROGRESS)
	{
		fd_set writers;
		tv.tv_sec = 1;
		tv.tv_usec = 0;

		FD_ZERO(&writers);
		FD_SET(sock, &writers);

		// Wait for the socket to be ready
		retval = select(sock+1, NULL, &writers, NULL, &tv);
		if(retval == 0)
			goto failed;

		// Try to connect again
		retval = connect(sock, (struct sockaddr*)&address, address_length);
		if(retval < 0)
			goto failed;
	}
	else if(retval < 0)
	{
		goto failed;
	}

	// Set socket to blocking
	retval = fcntl(sock, F_SETFL, flags);
	if(retval < 0)
		goto failed;

	return TRUE;

failed:
	if(sock != -1)
		close(sock);
	sock = -1;
	return FALSE;
}

//-----------------------------------------------------------------------------

void ConnectDropbox()
{
	if(!connect_socket())
	{
		fprintf(stderr, "Connecting failed\n");
		return;
	}

	io_channel = g_io_channel_unix_new(sock);
	g_io_channel_set_close_on_unref(io_channel, TRUE);
	g_io_channel_set_line_term(io_channel, "\n", -1);
}

//-----------------------------------------------------------------------------

static int WriteLine(const char * str)
{
	gsize bytes_written;
	GIOStatus status;

	do
	{
		status = g_io_channel_write_chars(io_channel, str, -1,
			&bytes_written, NULL);
	} while(status == G_IO_STATUS_AGAIN);

	if(status == G_IO_STATUS_ERROR)
	{
		fprintf(stderr, "dropbox_write() - G_IO_STATUS_ERROR\n");
		sock=-1;
		return G_IO_STATUS_ERROR;
	}
	return 0;
}

//-----------------------------------------------------------------------------

int GetDropboxStatus()
{
	if(sock<0) return 0;
	if(!WriteLine("get_dropbox_status\ndone\n"))
    {    
		g_io_channel_flush(io_channel, NULL);
		return ReadStatus();
	}
	return 0;
}

//-----------------------------------------------------------------------------
//static int line_number;
static gchar * ReadLine()
{
	if(sock<0) return 0;
	GIOStatus iostat;
	GError *tmp_error = NULL;
	gchar *line=0;
	gsize term_pos;	

	iostat = g_io_channel_read_line(io_channel, &line, NULL, &term_pos, &tmp_error);

	if (iostat == G_IO_STATUS_ERROR || tmp_error != NULL) 
	{
		printf("DropboxApi::ReadStatus G_IO_STATUS_ERROR\n");
		sock=-1;
		return 0;
    }

	if (iostat == G_IO_STATUS_EOF) 
	{
		printf("DropboxApi::ReadLine G_IO_STATUS_EOF connection closed\n");
		sock=-1;
		return 0;
	}

	if(line) line[term_pos]='\0';

//	printf("%d line-%s\n",line_number++,line);
	return line;	
}

//-----------------------------------------------------------------------------

static int ReadStatus()
{
	if(sock<0) return 0;
	int res=0;
	ClassString line;

	line=ReadLine();//ok
	if(!line.s) return 0;
	
	if(strcmp("ok", line.s))
	{	
		printf("1-ReadStatus %s\n",line.s);
		return 0;
	}	

	line=ReadLine();//status
	if(!line.s) return 0;

	if(!strcmp("status", line.s))
	{	
		if(SidePanelSelectDropbox) gtk_statusbar_push (StatusBar,0,DropboxFolder);
		res=1;
	} else
	{
		if(SidePanelSelectDropbox) gtk_statusbar_push (StatusBar,0,&line.s[7]);
	}

	line=ReadLine();//done
	if(!line.s) return 0;	

	if(strcmp("done", line.s))
	{	
		printf("3-ReadStatus %s\n",line.s);
		return 0;
	}	
	return res;
}

//-----------------------------------------------------------------------------


void DrawIconDropbox()
{
    if(!DropboxFolder)	return;
	
	if(sock<0)
	{
		side_panel.SetDropboxIcon(0);
		return;
	}	

	static int icon;
	int mode=MIME_FLAG_DROPBOX_SYNC;
	PixbufEmblemDropboxSync=IconSync[3&(icon++)];

	int res=UpdateListDropbox(0);	
	res|=UpdateListDropbox(1);	

	if(!res)
	{
		if(GetDropboxStatus())
		{	
			mode=MIME_FLAG_DROPBOX_OK;
		}	
	}
	side_panel.SetDropboxIcon(mode);
}

//-----------------------------------------------------------------------------

void InitDropbox()
{
	ConnectDropbox();
    ClassString setfile=g_build_filename(config_path,"/icons/emblem-default.png",NULL);
	PixbufEmblemDropboxOk=gdk_pixbuf_new_from_file_at_scale(setfile.s,8,8,1,0);
	if(!PixbufEmblemDropboxOk)
	{	
		printf("Not load %s\n",setfile.s);
		return;
	}	

	setfile=g_build_filename(config_path,"/icons/emblem-dropbox-syncing.png",NULL);
	IconSync[0]=gdk_pixbuf_new_from_file(setfile.s, NULL);
	if(!IconSync[0])
	{	
		printf("Not load %s\n",setfile.s);
		return;
	}	
	IconSync[1]=gdk_pixbuf_rotate_simple(IconSync[0],GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
	IconSync[2]=gdk_pixbuf_rotate_simple(IconSync[1],GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);
	IconSync[3]=gdk_pixbuf_rotate_simple(IconSync[2],GDK_PIXBUF_ROTATE_COUNTERCLOCKWISE);	
	IconSync[0]=gdk_pixbuf_scale_simple(IconSync[0],8,8,GDK_INTERP_NEAREST);//GDK_INTERP_BILINEAR);
	IconSync[1]=gdk_pixbuf_scale_simple(IconSync[1],8,8,GDK_INTERP_NEAREST);	
	IconSync[2]=gdk_pixbuf_scale_simple(IconSync[2],8,8,GDK_INTERP_NEAREST);
	IconSync[3]=gdk_pixbuf_scale_simple(IconSync[3],8,8,GDK_INTERP_NEAREST);
	PixbufEmblemDropboxSync=IconSync[0];

}

//-----------------------------------------------------------------------------

static int ReadStatusFile()
{
	if(sock<0) return 0;
	ClassString line;
	int res=MIME_FLAG_DROPBOX_SYNC;

	line=ReadLine();
	if(!line.s) return 0;
	
	if(strcmp("ok", line.s))
	{	
		if(strcmp("notok", line.s))
		{	
			printf("1-ReadStatusFile %s\n",line.s);
		}	
	}	

	line=ReadLine();
	if(!line.s) return 0;
	
	if(strstr(line.s,"up to date"))
	{	
		res=MIME_FLAG_DROPBOX_OK;
	} else
	if(strstr(line.s,"unwatched"))
	{	
		res=0;//MIME_FLAG_DROPBOX_OK;
	} else
		
	{
		if(strcmp("Illegal Argument: The 'path' argument does not exist", line.s))
		if(strcmp("status	syncing", line.s))			
		{	
			printf("2-ReadStatusFile %s\n",line.s);
		}	
	}

	line=ReadLine();
	if(!line.s) return 0;
	
	if(strcmp("done", line.s))
	{	
		printf("3-ReadStatusFile %s\n",line.s);
	}	

	return res;
}

//-----------------------------------------------------------------------------

int GetDropboxStatusFile(const char * fullname)
{
	if(sock<0) return 0;
	WriteLine("icon_overlay_file_status\npath\t");	

	ClassString com=g_strdup_printf("%s\n",fullname);
	WriteLine(com.s);
	WriteLine("done\n");

	g_io_channel_flush(io_channel, NULL);
	return ReadStatusFile();
}

//-----------------------------------------------------------------------------

int InDropbox(int num,const char * path)
{
    int res=0;

	ClearListStrings(&list_dropbox[num]);
	
	if(DropboxFolder && path)
	{
		if(!strncmp(DropboxFolder,path,strlen(DropboxFolder))) res=1;
	}

	return res;
}

//-----------------------------------------------------------------------------

void InsertInListDropbox(int num, const char * fullname)
{
	ClassPanel * p = Panels[num];
	if(!fullname) return;
	if(!p->InDropboxFlag) return;

	GList* l=g_list_first(list_dropbox[num]);

	while(l)
	{
		gchar * path=(gchar*)l->data;
		if(path)
		{
    		if(!strcmp(path,fullname)) return;
		} else 	printf("insert item null \n");
		l = (GList*)g_slist_next(l);
	}

//    printf("insert %s\n",fullname);
	list_dropbox[num]=g_list_append(list_dropbox[num],g_strdup(fullname));
}

//-----------------------------------------------------------------------------

static int UpdateListDropbox(int num)
{
	GList* l=g_list_first(list_dropbox[num]);
	int res=0;
	while(l)
	{
		gchar * path=(gchar*)l->data;
		if(path)
		{
			if(PanelUpdateItemDropbox(num,path))
			{
				g_free(l->data);
				l->data=0;
			} else res++;
		}	
		l = (GList*)g_slist_next(l);
	}
	list_dropbox[num]=g_list_remove_all(list_dropbox[num],0);
	return res;
}		

//-----------------------------------------------------------------------------




