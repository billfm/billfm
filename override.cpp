#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#include <errno.h>
#include "utils.h"


static GtkDialog *dialog;
static GtkEntry* EntryCommand;
static gchar * new_filename;
static GtkLabel * prompt;
int result=-1;

//-----------------------------------------------------------------------------

void CreateNewDirDialog(const char * workdir)
{
    const char* file_name;
    GtkLabel* prompt;
    int result;
    GtkWidget* dlg;
	GtkWidget* box;
    GtkWidget* entry;

    dlg = gtk_dialog_new_with_buttons( "Create new folder",
                                       NULL,
                                       (GtkDialogFlags)0,
                                       GTK_STOCK_CANCEL,
                                       GTK_RESPONSE_CANCEL,
                                       GTK_STOCK_OK,
                                       GTK_RESPONSE_OK,
	                                   NULL );
	
    gtk_dialog_set_alternative_button_order( GTK_DIALOG(dlg), GTK_RESPONSE_OK, GTK_RESPONSE_CANCEL, -1 );

    box = ( ( GtkDialog* ) dlg )->vbox;
    prompt = (GtkLabel* ) gtk_label_new( "Input a name for the new folder:" );
    gtk_box_pack_start( GTK_BOX( box ), (GtkWidget*)prompt, FALSE, FALSE, 4 );

    entry = gtk_entry_new();
    gtk_entry_set_text( GTK_ENTRY( entry ),  "NewFolder"  );
    gtk_box_pack_start( GTK_BOX( box ), entry, FALSE, FALSE, 4 );

    gtk_dialog_set_default_response( ( GtkDialog* ) dlg, GTK_RESPONSE_OK );
    gtk_entry_set_activates_default ( GTK_ENTRY( entry ), TRUE );
    gtk_widget_show_all( box );

/* */
	while ( gtk_dialog_run( GTK_DIALOG( dlg ) ) == GTK_RESPONSE_OK )
    {
        file_name = gtk_entry_get_text((GtkEntry*)entry);
        ClassString full_path = g_build_filename(  workdir, file_name, NULL );

		if ( g_file_test( full_path.s, G_FILE_TEST_EXISTS ) )
        {
            gtk_label_set_text( prompt,
                                 "The file name you specified already exists.\n"
                                   "Please input a new one:"  );
            continue;
        }

		result = mkdir( full_path.s, 0755 );
		if(result)
		{
			ClassString mes = g_strdup_printf("Error create folder `%s`.\n%s", 
			                              full_path.s,strerror( errno ) );
			ShowFileOperation(mes.s);
		}

        break;
    }

	gtk_widget_destroy( dlg );

}
//-----------------------------------------------------------------------------

int DialogYesNo(const char * mes)
{
	GtkWidget * dlg = gtk_message_dialog_new(0,
                                      GTK_DIALOG_MODAL,
                                      GTK_MESSAGE_WARNING,
                                      GTK_BUTTONS_YES_NO,
                                      "%s",mes);
	gtk_dialog_set_default_response (GTK_DIALOG (dlg), GTK_RESPONSE_YES);
	int ret = gtk_dialog_run( GTK_DIALOG( dlg ) );
	gtk_widget_destroy( dlg );
	return  (ret==GTK_RESPONSE_YES);
}

//-----------------------------------------------------------------------------

static void OnDialogOverrideClick( GtkButton* button, int res ) 
{
    result=res;
	if(result==RESPONSE_RENAME)
	{
		const char * file_name = gtk_entry_get_text( EntryCommand );

		if ( g_file_test( file_name, G_FILE_TEST_EXISTS ) )
        {
            gtk_label_set_text( prompt,
                                 "The file name you specified already exists.\n"
                                   "Please input a new one:" );
			gtk_widget_set_visible((GtkWidget*)prompt,1);
			return;
		} else 
		{
		    new_filename=g_strdup(file_name);
		}
	}
	gtk_widget_destroy( (GtkWidget*) dialog );
	dialog=NULL;
}

//-----------------------------------------------------------------------------

GtkBuilder* CreateForm(const char * name)
{
	GtkBuilder* builder = gtk_builder_new();
	GError *error = NULL;
	ClassString form = g_strdup_printf("%s/billfm/forms/%s",g_get_home_dir(),name);
	if (!g_file_test( form.s, G_FILE_TEST_EXISTS ))
	{
		form = g_strdup_printf("/usr/share/billfm/%s",name);
	}	

	if( ! gtk_builder_add_from_file( builder, form.s, &error ) )
	{
		g_warning( "%s\n", error->message );
		ShowMessage3(0,"Ошибка загрузки формы",error->message);
		g_free( error );
		return NULL;
	} 
 return builder;
}

//-----------------------------------------------------------------------------

int DialogOverride( gchar * source, const char ** newdest )
{
	GtkBuilder* builder=CreateForm("override.glade");
	if(!builder) return -1;

	const char * dest=*newdest;

	prompt = (GtkLabel*)gtk_builder_get_object( builder, "prompt");
	gtk_widget_set_visible((GtkWidget*)prompt,0);	
	int OneFileFlag=0;
	if(!strcmp(*newdest,source))
	{
		OneFileFlag=1;
        gtk_label_set_text(prompt,"Источник и цель совпадают!");
		gtk_widget_set_visible((GtkWidget*)prompt,1);		
	}	
	
	dialog = (GtkDialog *) gtk_builder_get_object( builder, "dialog1");
	EntryCommand = (GtkEntry*) gtk_builder_get_object( builder, "entry1");
	
	GtkButton* button;

	const char* items[] =
    {
		"Переименовать",
		"Переписать",
		"Переписать все",
		"Пропустить",
		"Пропустить все",
		"Отменить",
		0
	};

	const char *names[] =
    {
		"bRename",
		"bOver",
		"bOverAll",
		"bSkip",
		"bSkipAll",
		"bCancel",		
		0
	};

	int results[]=
	{
		RESPONSE_RENAME,
		RESPONSE_OVERWRITE,
		RESPONSE_OVERWRITEALL,
		RESPONSE_SKIP,
		RESPONSE_SKIPALL,
		RESPONSE_CANCEL,
		-1,
		0
	};

	int i=0;
	while(items[i])
	{
		button = (GtkButton*) gtk_builder_get_object( builder,names[i]);
		gtk_button_set_label(button,items[i]);
		g_signal_connect(G_OBJECT(button),"clicked",G_CALLBACK(OnDialogOverrideClick),
		                 (void*)results[i]);
		if(OneFileFlag && (i==2  || i==1))
			gtk_widget_set_visible((GtkWidget*)button,0);
		i++;
	}
		
	result = -1;
	GtkLabel * label = (GtkLabel*) gtk_builder_get_object( builder, "source_name");
	gchar * mes=g_strdup_printf("Новым: %s",source);
	gtk_label_set_text ((GtkLabel *) label, mes);
	g_free(mes);

	label = (GtkLabel*) gtk_builder_get_object( builder, "dest_name");
	mes=g_strdup_printf("Заменить: %s",dest);
	gtk_label_set_text ((GtkLabel *) label, mes);
	g_free(mes);

	char buf[ 64 ];

	struct stat source_stat;
    if(!lstat(source, &source_stat)) 
	{
	    strftime( buf, sizeof( buf ),"%Y-%m-%d %H:%M", localtime( &source_stat.st_mtime ) );
    	if(S_ISDIR(source_stat.st_mode))
		{	
			gtk_window_set_title(GTK_WINDOW(dialog), "Каталог существует");
			mes=g_strdup_printf("%s",buf);
		} else
		{
			gtk_window_set_title(GTK_WINDOW(dialog), "Файл существует");
			mes=g_strdup_printf("%d - %s, %s",
		                    (int)source_stat.st_size,
		                    showfilesize(source_stat.st_size),
		                    buf);
		}	
    	label = (GtkLabel*) gtk_builder_get_object( builder, "source_prop");		
		gtk_label_set_text ((GtkLabel *) label, mes);
		g_free(mes);
	} 

	struct stat dest_stat;
    if(!lstat(dest, &dest_stat)) 
	{
	    strftime( buf, sizeof( buf ),"%Y-%m-%d %H:%M", localtime( &dest_stat.st_mtime ) );
    	if(S_ISDIR(dest_stat.st_mode))
		{	
			mes=g_strdup_printf("%s",buf);
		} else
		{
			mes=g_strdup_printf("%d - %s, %s",
		                    (int)dest_stat.st_size,
		                    showfilesize(dest_stat.st_size),
		                    buf);

		}	
		
    	label = (GtkLabel*) gtk_builder_get_object( builder, "dest_prop");		
		gtk_label_set_text ((GtkLabel *) label, mes);
		g_free(mes);
	} 
	
    gtk_entry_set_text(EntryCommand,dest);

	gtk_window_set_keep_above((GtkWindow*) dialog, 1);
	gtk_dialog_run (dialog);
    if(dialog)	gtk_widget_destroy( (GtkWidget*) dialog );

	if(result==RESPONSE_RENAME) *newdest=new_filename;
	
	return result;
}

//-----------------------------------------------------------------------------
static int ModeCopyLink=1;

static void OnCheckedClick( GtkButton* button, int res ) 
{
	printf("ModeCopyLink=%d\n",res);
	ModeCopyLink=res;
}

//-----------------------------------------------------------------------------

int DialogCopy(int task, const char * dest, GList * l)
{
    ModeCopyLink=1;
	GtkBuilder* builder=CreateForm("copy.glade");
	if(!builder) return -1;

	ClassString source;
	if(task==TASK_COPY) source=g_strdup("Копировать:"); else source=g_strdup("Переместить:");
	for ( int i=0; l; l = l->next,i++ )
	{
        source=g_strdup_printf("%s\n %s",source.s, (const char*) l->data);
		if(i>5)
		{	
	        source=g_strdup_printf("%s\n ...",source.s);
			break;
		}	
	}

	GtkButton* button;

	const char* items[] =
    {
		"Выполнить",
		"Отменить",
		0
	};

	const char *names[] =
    {
		"bOk",
		"bCancel",		
		0
	};

	for(int i=0; items[i]; i++)
	{
		button = (GtkButton*) gtk_builder_get_object( builder,names[i]);
		gtk_button_set_label(button,items[i]);
	}

	GtkLabel * label = (GtkLabel*) gtk_builder_get_object( builder, "label1");
	ClassString mes=g_strdup_printf("%s\n\n В каталог :\n %s\n",source.s,dest);
	gtk_label_set_text ((GtkLabel *) label, mes.s);

	dialog = (GtkDialog *) gtk_builder_get_object( builder, "dialog1");	
	gtk_window_set_keep_above((GtkWindow*) dialog, 1);

	if(task==TASK_COPY) 
    	gtk_window_set_title(GTK_WINDOW(dialog), "Копировать файлы");
	else 
    	gtk_window_set_title(GTK_WINDOW(dialog), "Перемещать файлы");

	int res=gtk_dialog_run (dialog);
    if(dialog)
	{	
		gtk_widget_hide((GtkWidget*)dialog);	
		gtk_widget_destroy((GtkWidget*)dialog);
	}	

	if(res==1)
	{	
		InfoOperation fo;
		fo.func=task;
		fo.mode_link=ModeCopyLink;
		ProcessCopyFiles(dest,&fo);
	}	
	return res;
}	
	
//-----------------------------------------------------------------------------

int LinkDialogCopy(InfoOperation * fo, const char * source, const char * dest)
{
    ModeCopyLink=1;
	GtkBuilder* builder=CreateForm("copy.glade");
	if(!builder) return -1;

	ClassString prompt;
	if(fo->func==TASK_COPY) prompt=g_strdup("Копировать:"); else prompt=g_strdup("Переместить:");
    prompt=g_strdup_printf("%s\n %s\nв %s",prompt.s, source,dest);

	GtkLabel * label = (GtkLabel*) gtk_builder_get_object( builder, "label1");
	gtk_label_set_text ((GtkLabel *) label, prompt.s);
	
	GtkButton* button;

	const char* items[] =
    {
		"Выполнить",
		"Отменить",
		0
	};

	const char *names[] =
    {
		"bOk",
		"bCancel",		
		0
	};

	for(int i=0; items[i]; i++)
	{
		button = (GtkButton*) gtk_builder_get_object( builder,names[i]);
		gtk_button_set_label(button,items[i]);
	}


	dialog = (GtkDialog *) gtk_builder_get_object( builder, "dialog1");	
	gtk_window_set_keep_above((GtkWindow*) dialog, 1);

	if(fo->func==TASK_COPY) 
    	gtk_window_set_title(GTK_WINDOW(dialog), "Копировать файлы");
	else 
    	gtk_window_set_title(GTK_WINDOW(dialog), "Перемещать файлы");

	GtkWidget *radio1=0, *radio2=0, *radio3=0, *box=0;
	box = (GtkWidget *)gtk_builder_get_object( builder, "vbox2");
	if(box)
	{	
		radio1 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
							"Создать на старый источник (-l) (default)");
		g_signal_connect(G_OBJECT(radio1),"clicked",G_CALLBACK(OnCheckedClick),(void*)1);
   
    	radio2 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
                            "Сохранить как есть (-P)");
   		g_signal_connect(G_OBJECT(radio2),"clicked",G_CALLBACK(OnCheckedClick),(void*)2);

		if(fo->func==TASK_COPY) 
		{	
		   	radio3 = gtk_radio_button_new_with_label_from_widget (GTK_RADIO_BUTTON (radio1),
								"Копировать файл по ссылке ");
			g_signal_connect(G_OBJECT(radio3),"clicked",G_CALLBACK(OnCheckedClick),(void*)3);
		}	
		
		gtk_box_pack_start (GTK_BOX (box), radio1, TRUE, TRUE, 2);
		gtk_box_pack_start (GTK_BOX (box), radio2, TRUE, TRUE, 2);
		if(fo->func==TASK_COPY) 
		{	
			gtk_box_pack_start (GTK_BOX (box), radio3, TRUE, TRUE, 2);	
		}
		
		gtk_widget_show_all ((GtkWidget*) dialog);
	}
   

	gtk_dialog_run (dialog);
    if(dialog)	gtk_widget_destroy( (GtkWidget*) dialog );
	fo->mode_link=ModeCopyLink;
	return 1;
}	
	
//-----------------------------------------------------------------------------
