#include <glib.h>
#include <gtk/gtk.h>
#include <string.h>


static GdkDragAction clipboard_action = GDK_ACTION_DEFAULT;

static GList* clipboard_file_list = NULL;
static GList* paste_list = NULL;

//-----------------------------------------------------------------------------

static void clipboard_get_data ( GtkClipboard *clipboard,
                                 GtkSelectionData *selection_data,
                                 guint info,
                                 gpointer user_data )
{
    GdkAtom uri_list_target = gdk_atom_intern( "text/uri-list", FALSE );
    GdkAtom gnome_target = gdk_atom_intern( "x-special/gnome-copied-files", FALSE );
    GList* l;
    gchar* file_name;
    const char* action;
    gboolean use_uri = FALSE;

    GString* list;

    if ( ! clipboard_file_list )
        return ;

    list = g_string_sized_new( 8192 );

    if ( selection_data->target == gnome_target )
    {
        action = clipboard_action == GDK_ACTION_MOVE ? "cut\n" : "copy\n";
        g_string_append( list, action );
        use_uri = TRUE;
    }
    else if ( selection_data->target == uri_list_target )
        use_uri = TRUE;

    for ( l = clipboard_file_list; l; l = l->next )
    {
        if ( use_uri )
        {
            file_name = g_filename_to_uri( ( char* ) l->data, NULL, NULL );
        }
        else
        {
            file_name = g_filename_display_name( ( char* ) l->data );
        }
        g_string_append( list, file_name );
        g_free( file_name );

        if ( selection_data->target != uri_list_target )
            g_string_append_c( list, '\n' );
        else
            g_string_append( list, "\r\n" );
    }

    gtk_selection_data_set ( selection_data, selection_data->target, 8,
                             ( guchar* ) list->str, list->len + 1 );
    /* g_debug( "clipboard data:\n%s\n\n", list->str ); */
    g_string_free( list, TRUE );
}

//-----------------------------------------------------------------------------

static void clipboard_clean_data ( GtkClipboard *clipboard,
                                   gpointer user_data )
{
/*
	if ( clipboard_file_list )
    {
        g_list_foreach( clipboard_file_list, ( GFunc ) g_free, NULL );
        g_list_free( clipboard_file_list );
        clipboard_file_list = NULL;
    }*/
    clipboard_action = GDK_ACTION_DEFAULT; 
}

//-----------------------------------------------------------------------------

GList*  ptk_clipboard_paste_files( int * _action )
{
	if(paste_list)
	{
	        g_list_foreach( paste_list, ( GFunc ) g_free, NULL );
	        g_list_free( paste_list );
	        paste_list = NULL;
	}
		
	GtkClipboard * clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GdkAtom gnome_target;
    GdkAtom uri_list_target;
    gchar **uri_list, **puri;
    GtkSelectionData* sel_data = NULL;
//    GList* files = NULL;
    gchar* file_path;


    char* uri_list_str;

    gnome_target = gdk_atom_intern( "x-special/gnome-copied-files", FALSE );
    sel_data = gtk_clipboard_wait_for_contents( clip, gnome_target );
    if ( sel_data )
    {
        if ( sel_data->length <= 0 || sel_data->format != 8 ) return 0;

        uri_list_str = ( char* ) sel_data->data;
        if ( 0 == strncmp( ( char* ) sel_data->data, "cut", 3 ) )
            *_action = GDK_ACTION_MOVE;
        else
            *_action = GDK_ACTION_COPY;

        if ( uri_list_str )
        {
            while ( *uri_list_str && *uri_list_str != '\n' )
                ++uri_list_str;
        }
    }
    else
    {
        uri_list_target = gdk_atom_intern( "text/uri-list", FALSE );
        sel_data = gtk_clipboard_wait_for_contents( clip, uri_list_target );
        if ( ! sel_data ) return 0;
        if ( sel_data->length <= 0 || sel_data->format != 8 )  return 0;
        uri_list_str = ( char* ) sel_data->data;

        if ( clipboard_action == GDK_ACTION_MOVE )
            *_action = GDK_ACTION_MOVE;
        else
            *_action = GDK_ACTION_COPY;
    }

    if ( uri_list_str )
    {
        puri = uri_list = g_uri_list_extract_uris( uri_list_str );
        while ( *puri )
        {
            file_path = g_filename_from_uri( *puri, NULL, NULL );
            if ( file_path )
            {
                paste_list = g_list_prepend( paste_list, file_path );
            }
            ++puri;
        }
        g_strfreev( uri_list );
        gtk_selection_data_free( sel_data );

    }
	return paste_list;
}

//-----------------------------------------------------------------------------

void ptk_clipboard_cut_or_copy_files( GList* files, int action )
{
    GtkClipboard * clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GtkTargetList* target_list = gtk_target_list_new( NULL, 0 );
    GList* target;
    gint i, n_targets;
    GtkTargetEntry* targets;
    GtkTargetPair* pair;
    GList *l;

    GList* file_list = NULL;

    gtk_target_list_add_text_targets( target_list, 0 );
    n_targets = g_list_length( target_list->list ) + 2;

    targets = g_new0( GtkTargetEntry, n_targets );
    target = target_list->list;
    for ( i = 0; target; ++i, target = g_list_next( target ) )
    {
        pair = ( GtkTargetPair* ) target->data;
        targets[ i ].target = gdk_atom_name ( pair->target );
    }
    targets[ i ].target = g_strdup("x-special/gnome-copied-files");
    targets[ i + 1 ].target = g_strdup("text/uri-list");

    gtk_target_list_unref ( target_list );

    for ( l = files; l; l = l->next )
    {
        file_list = g_list_prepend( file_list, l->data );
    }

    gtk_clipboard_set_with_data ( clip, targets, n_targets,
                                  clipboard_get_data,
                                  clipboard_clean_data,
                                  NULL );

    g_free( targets );

    clipboard_file_list = file_list;
    clipboard_action = (GdkDragAction)action;
}

//-----------------------------------------------------------------------------
