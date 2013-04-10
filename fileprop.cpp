#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "tree.h"
#include "utils.h"
#include "disks.h"
#include <pwd.h>
#include <grp.h>

//-----------------------------------------------------------------------------

const char* chmod_names[] =
    {
        "others_x", "others_w", "others_r", 
        "group_x",  "group_w",  "group_r",
		"owner_x",  "owner_w",   "owner_r",
		"sticky", "set_gid", "set_uid"
    };

int chod_mask[]=
{
	S_IXOTH,//	00001	все прочие имеют права выполнения
	S_IWOTH,//	00002	все прочие имеют права записи		
	S_IROTH,//	00004	все прочие имеют права чтения		
	S_IXGRP,//	00010	группа имеет права выполнения
	S_IWGRP,//	00020	группа имеет права записи
	S_IRGRP,//	00040	группа имеет права чтения
	S_IXUSR,//	00100	пользователь имеет право выполнения
	S_IWUSR,//	00200	пользователь имеет право записи
	S_IRUSR,//	00400	пользователь имеет право чтения
	S_ISVTX,//	0001000	бит принадлежности (смотри ниже)
	S_ISGID,//	0002000	бит setgid (смотри ниже)		
	S_ISUID//	0004000	бит setuid			
};

//-----------------------------------------------------------------------------

void DialogFileProperty(const char * fullname)
{
	ClassString str;
	GtkEntry * entry;

	GtkBuilder* builder=CreateForm("properties.glade");
	if(!builder) return;

	ClassString shortname = g_path_get_basename(fullname);
	entry = (GtkEntry*) gtk_builder_get_object( builder, "file_name");
    gtk_entry_set_text(entry,shortname.s);

	entry = (GtkEntry*) gtk_builder_get_object( builder, "fullpath");

	const char * trash_name=IsAlreadyTrashed(fullname);
    if(trash_name)
	{
		ClassString str=PrepareRestore(fullname);
		if(str.s)
		{	
			gtk_entry_set_text(entry,str.s);
			str=GetDeletedTime(fullname);
			GtkEntry * entry = (GtkEntry*) gtk_builder_get_object( builder, "deleted_entry");
		    gtk_entry_set_text(entry,str.s);
		}	
	} else
	{	
		gtk_entry_set_text(entry,fullname);
		GtkWidget * widget = (GtkWidget*) gtk_builder_get_object( builder, "deleted_entry");
		gtk_widget_destroy(widget);
//		gtk_widget_hide(widget);
		widget = (GtkWidget*) gtk_builder_get_object( builder, "deleted_label");
	    gtk_widget_hide(widget);
		gtk_widget_destroy(widget);		
	}	

	entry = (GtkEntry*) gtk_builder_get_object( builder, "mime_type");

	ClassString com=g_strdup_printf("file '%s' ",fullname);
	FILE * f = popen (com.s,"r");
	if(f)
	{	
		char str[1024];
		if(fgets(str,1023,f))
		{
			str[strlen(str)-1]=0;
			gtk_entry_set_text(entry,&str[strlen(fullname)+2]);	
		}
		pclose(f);
	}	

	struct stat filestat;
	if(!lstat(fullname, &filestat))
	{
		str=get_file_mtime_string(filestat.st_mtime);
		entry = (GtkEntry*) gtk_builder_get_object( builder, "mtime");
	    gtk_entry_set_text(entry,str.s);

		str=get_file_mtime_string(filestat.st_atime);
		entry = (GtkEntry*) gtk_builder_get_object( builder, "atime");
	    gtk_entry_set_text(entry,str.s);

		str=showfilesize(filestat.st_size);
		str=g_strdup_printf("%d %s",(int)filestat.st_size,str.s);
		entry = (GtkEntry*) gtk_builder_get_object( builder, "size");
	    gtk_entry_set_text(entry,str.s);

		str=showfilesize(filestat.st_blocks*filestat.st_blksize);
		str=g_strdup_printf("%ld %s",filestat.st_blocks*filestat.st_blksize,str.s);
		entry = (GtkEntry*) gtk_builder_get_object( builder, "size_on_disk");
	    gtk_entry_set_text(entry,str.s);

		struct passwd * puser;
		puser = getpwuid(filestat.st_uid);
		str=g_strdup_printf("%d:%s",(int)filestat.st_uid, puser->pw_name);
		entry = (GtkEntry*) gtk_builder_get_object( builder, "owner");
	    gtk_entry_set_text(entry,str.s);

		struct group* pgroup;
		pgroup = getgrgid(filestat.st_gid);
		str=g_strdup_printf("%d:%s",(int)filestat.st_gid,pgroup->gr_name);
		entry = (GtkEntry*) gtk_builder_get_object( builder, "group");
	    gtk_entry_set_text(entry,str.s);
		
		for(int i=0;i<12;i++)
		{
			GtkToggleButton * b=(GtkToggleButton*)gtk_builder_get_object( builder, chmod_names[i]);
			gtk_toggle_button_set_active(b, filestat.st_mode & chod_mask[i] );
		}

	}	

	GtkWindow* dlg=GTK_WINDOW(gtk_builder_get_object( builder, "dlg"));
	gtk_widget_show_all( GTK_WIDGET (dlg) );

	while ( gtk_dialog_run( GTK_DIALOG( dlg ) ) == GTK_RESPONSE_OK )
    {
		int mask=0;
		for(int i=0;i<12;i++)
		{
			GtkToggleButton * b=(GtkToggleButton*)gtk_builder_get_object( builder, chmod_names[i]);
			if(gtk_toggle_button_get_active(b))
			{
				mask|=chod_mask[i];
			}  
		}
		int res=chmod(fullname,mask);
		if(res)
		{
			ClassString mes = g_strdup_printf("Error set right.\n%s",strerror( errno ) );
       		ShowFileOperation(mes.s);
		}

		gtk_widget_destroy(GTK_WIDGET(dlg));
		return;
    }//while
	gtk_widget_destroy(GTK_WIDGET(dlg));
}

//-----------------------------------------------------------------------------

gchar * get_file_mtime_string(time_t mtime)
{
	char buf[ 64 ];
	strftime( buf, sizeof(buf),"%Y-%m-%d %H:%M",localtime(&mtime));
    return g_strdup( buf );
}

//-----------------------------------------------------------------------------

gchar * get_file_rigth_string( mode_t mode )
{
	char perm[16];
	perm[ 0 ] = S_ISDIR( mode ) ? 'd' : ( S_ISLNK( mode ) ? 'l' : '-' );
    perm[ 1 ] = ( mode & S_IRUSR ) ? 'r' : '-';
    perm[ 2 ] = ( mode & S_IWUSR ) ? 'w' : '-';
	if(S_ISUID & mode) perm[3] = 's'; else perm[3] = ( mode & S_IXUSR ) ? 'x' : '-';
    perm[ 4 ] = ( mode & S_IRGRP ) ? 'R' : '-';
    perm[ 5 ] = ( mode & S_IWGRP ) ? 'W' : '-';
	if(S_ISGID & mode) perm[6] = 'S'; else perm[6] = ( mode & S_IXGRP ) ? 'X' : '-';
    perm[ 7 ] = ( mode & S_IROTH ) ? 'r' : '-';
    perm[ 8 ] = ( mode & S_IWOTH ) ? 'w' : '-';
    if ( mode & S_ISVTX ) //MOD
    {
        if ( mode & S_IXOTH )
            perm[ 9 ] = 't';
        else
            perm[ 9 ] = 'T';        
    }
    else
        perm[ 9 ] = ( mode & S_IXOTH ) ? 'x' : '-';
    perm[ 10 ] = '\0';
	return g_strdup(perm);
}

//-----------------------------------------------------------------------------
//find /home/test -type f -exec chmod 644 {} \;
//find /home/test -type d -exec chmod 755 {} \;
