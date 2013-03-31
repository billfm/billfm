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

char *replace_string( char* orig, char* str, char* replace, gboolean quote )
{   // replace all occurrences of str in orig with replace, optionally quoting
    char* rep;
    char* cur;
    char* result = NULL;
    char* old_result;
    char* s;

    if ( !orig || !( s = strstr( orig, str ) ) )
        return g_strdup( orig );  // str not in orig
    
    if ( !replace )
    {
        if ( quote )
            rep = g_strdup( "''" );
        else
            rep = g_strdup( "" );
    }
    else if ( quote )
        rep = g_strdup_printf( "'%s'", replace );
    else
        rep = g_strdup( replace );

    cur = orig;
    do
    {
        if ( result )
        {
            old_result = result;
            result = g_strdup_printf( "%s%s%s", old_result,
                                            g_strndup( cur, s - cur ), rep );
            g_free( old_result );
        }
        else
            result = g_strdup_printf( "%s%s", g_strndup( cur, s - cur ), rep );
        cur = s + strlen( str );
        s = strstr( cur, str );
    } while ( s );
    old_result = result;
    result = g_strdup_printf( "%s%s", old_result, cur );
    g_free( old_result );
    g_free( rep );
    return result;
    
/*
    // replace first occur of str in orig with rep
    char* buffer;
    char* buffer2;
    char *p;
    char* rep_good;

    if ( !( p = strstr( orig, str ) ) )
        return g_strdup( orig );  // str not in orig
    if ( !rep )
        rep_good = g_strdup_printf( "" );
    else
        rep_good = g_strdup( rep );
    buffer = g_strndup( orig, p - orig );
    if ( quote )
        buffer2 = g_strdup_printf( "%s'%s'%s", buffer, rep_good, p + strlen( str ) );
    else
        buffer2 = g_strdup_printf( "%s%s%s", buffer, rep_good, p + strlen( str ) );
    g_free( buffer );
    g_free( rep_good );
    return buffer2;
*/
}

//-----------------------------------------------------------------------------

char* bash_quote( char* str )
{  
    char* s1 = replace_string( str, (gchar*)"'", (gchar*)"'\\''", FALSE );
    char* s2 = g_strdup_printf( "'%s'", s1 );
    g_free( s1 );
    return s2;
}

//-----------------------------------------------------------------------------

void ptk_clipboard_copy_as_text( const char* working_dir,
                                      GList* files )  //MOD added
{
    GtkClipboard * clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GList *l;

    char* file_path;
    char* file_text;
    char* str;
    char* quoted;
    
    file_text = g_strdup( "" );
    for ( l = files; l; l = l->next )
    {

        file_path = g_build_filename( working_dir, "file name in clip", NULL );
        quoted = bash_quote( file_path );
        str = file_text;
        file_text = g_strdup_printf( "%s %s", str, quoted );
        g_free( str );
        g_free( quoted );
        g_free( file_path );
    }
    gtk_clipboard_set_text ( clip, file_text , -1 );
    g_free( file_text );
}

//-----------------------------------------------------------------------------

void ptk_clipboard_copy_name( const char* working_dir,
                                      GList* files )  //MOD added
{
    GtkClipboard* clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GList *l;

    char* file_text;
    gint fcount = 0;
    char* str;
    
    file_text = g_strdup( "" );
    for ( l = files; l; l = l->next )
    {

        str = file_text;
        if ( fcount == 0 )
            file_text = g_strdup_printf( "%s", "copy copr" );
        else if ( fcount == 1 )
            file_text = g_strdup_printf( "%s\n%s\n", file_text, "fdfd" );
        else
            file_text = g_strdup_printf( "%s%s\n", file_text, "dfdfsdfsd" );
        fcount++;
        g_free( str );
    }
    gtk_clipboard_set_text( clip, file_text , -1 );
    g_free( file_text );
}

//-----------------------------------------------------------------------------

void ptk_clipboard_paste_links( GtkWindow* parent_win,
                                const char* dest_dir )   //MOD added
{
    GtkClipboard * clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GdkAtom gnome_target;
    GdkAtom uri_list_target;
    gchar **uri_list, **puri;
    GtkSelectionData* sel_data = NULL;
    GList* files = NULL;
    gchar* file_path;


    int action;
    char* uri_list_str;

    gnome_target = gdk_atom_intern( "x-special/gnome-copied-files", FALSE );
    sel_data = gtk_clipboard_wait_for_contents( clip, gnome_target );
    if ( sel_data )
    {
        if ( sel_data->length <= 0 || sel_data->format != 8 )
            return ;

        uri_list_str = ( char* ) sel_data->data;
        action = GDK_ACTION_LINK;
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
        if ( ! sel_data )
            return ;
        if ( sel_data->length <= 0 || sel_data->format != 8 )
            return ;
        uri_list_str = ( char* ) sel_data->data;
        action = GDK_ACTION_LINK;
    }

    if ( uri_list_str )
    {
        puri = uri_list = g_uri_list_extract_uris( uri_list_str );
        while ( *puri )
        {
            file_path = g_filename_from_uri( *puri, NULL, NULL );
            if ( file_path )
            {
                files = g_list_prepend( files, file_path );
            }
            ++puri;
        }
        g_strfreev( uri_list );
        gtk_selection_data_free( sel_data );

/*run run run */
    }
}

//-----------------------------------------------------------------------------

void ptk_clipboard_paste_targets( GtkWindow* parent_win,
                                const char* dest_dir )   //MOD added
{
    GtkClipboard * clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GdkAtom gnome_target;
    GdkAtom uri_list_target;
    gchar **uri_list, **puri;
    GtkSelectionData* sel_data = NULL;
    GList* files = NULL;
    gchar* file_path;
    gint missing_targets = 0;
    

    int action;
    char* uri_list_str;

    gnome_target = gdk_atom_intern( "x-special/gnome-copied-files", FALSE );
    sel_data = gtk_clipboard_wait_for_contents( clip, gnome_target );
    if ( sel_data )
    {
        if ( sel_data->length <= 0 || sel_data->format != 8 )
            return ;

        uri_list_str = ( char* ) sel_data->data;
        action = GDK_ACTION_COPY;
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
        if ( ! sel_data )
            return ;
        if ( sel_data->length <= 0 || sel_data->format != 8 )
            return ;
        uri_list_str = ( char* ) sel_data->data;
        action = GDK_ACTION_COPY;
    }

    if ( uri_list_str )
    {
        puri = uri_list = g_uri_list_extract_uris( uri_list_str );
        while ( *puri )
        {
            file_path = g_filename_from_uri( *puri, NULL, NULL );
            if ( file_path )
            {
                if ( g_file_test( file_path, G_FILE_TEST_IS_SYMLINK ) )
                {
                    file_path = g_file_read_link ( file_path, NULL );
                }
                if ( file_path )
                {
                    if ( g_file_test( file_path, G_FILE_TEST_EXISTS ) )             
                        files = g_list_prepend( files, file_path );
                    else
                        missing_targets++;
                }
            }
            ++puri;
        }
        g_strfreev( uri_list );
        gtk_selection_data_free( sel_data );

/*run run run*/        
        if ( missing_targets > 0 )
{
 /*           ptk_show_error( GTK_WINDOW( parent_win ),
                            g_strdup_printf ( "Error" ),
                            g_strdup_printf ( "%i target%s missing",
                            missing_targets, 
                            missing_targets > 1 ? g_strdup_printf ( "s are" ) : 
                            g_strdup_printf ( " is" ) ) );*/
}
    }
}


//-----------------------------------------------------------------------------

GList* ptk_clipboard_paste_files( int * action )
{
    GtkClipboard * clip = gtk_clipboard_get( GDK_SELECTION_CLIPBOARD );
    GdkAtom gnome_target;
    GdkAtom uri_list_target;
    gchar **uri_list, **puri;
    GtkSelectionData* sel_data = NULL;
    gchar* file_path;


    char* uri_list_str;

	if(paste_list)
	{
	        g_list_foreach( paste_list, ( GFunc ) g_free, NULL );
	        g_list_free( paste_list );
	        paste_list = NULL;
	}

    gnome_target = gdk_atom_intern( "x-special/gnome-copied-files", FALSE );
    sel_data = gtk_clipboard_wait_for_contents( clip, gnome_target );
    if ( sel_data )
    {
        if ( sel_data->length <= 0 || sel_data->format != 8 )
            return paste_list;

        uri_list_str = ( char* ) sel_data->data;
//printf("->%s<-\n",sel_data->data);
		if ( 0 == strncmp( ( char* ) sel_data->data, "cut", 3 ) )
            *action = GDK_ACTION_MOVE;
        else
            *action = GDK_ACTION_COPY;

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
        if ( ! sel_data )
            return paste_list;
        if ( sel_data->length <= 0 || sel_data->format != 8 )
            return paste_list;
        uri_list_str = ( char* ) sel_data->data;
//printf("->%s<-\n",sel_data->data);	
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
//printf("%s\n",  file_path);
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
    clipboard_action=(GdkDragAction)action;
    gtk_target_list_add_text_targets( target_list, 0 );
    n_targets = g_list_length( target_list->list ) + 2;

    targets = g_new0( GtkTargetEntry, n_targets );
    target = target_list->list;
    for ( i = 0; target; ++i, target = g_list_next( target ) )
    {
        pair = ( GtkTargetPair* ) target->data;
        targets[ i ].target = gdk_atom_name ( pair->target );
    }
    targets[ i ].target = (gchar*)"x-special/gnome-copied-files";
    targets[ i + 1 ].target = (gchar*)"text/uri-list";

    gtk_target_list_unref ( target_list );

    clipboard_file_list = files;

	gtk_clipboard_set_with_data ( clip, targets, n_targets,
                                  clipboard_get_data,
                                  clipboard_clean_data,
                                  NULL );

    g_free( targets );
}

//-----------------------------------------------------------------------------
