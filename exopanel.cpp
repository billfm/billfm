#include <gtk/gtk.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>


#include "tree.h" 

//-----------------------------------------------------------------------------

void  OnDoubleClick_IconView (ExoIconView *icon_view,  GtkTreePath *path, gpointer userdata)
{
	ClassPanel * panel=(ClassIconPanel*)userdata;
	panel->OnDoubleClick( path );
}

//-----------------------------------------------------------------------------

static void SelectionChangeExo (ExoIconView *icon_view,  gpointer userdata)
{
//	ClassPanel * panel=(ClassPanel*)userdata;
//	panel->InfoFileStatusBar();
}

//-----------------------------------------------------------------------------

void ClassIconPanel::CreatePanel(void) 
{
	iconview = (ExoIconView* ) exo_icon_view_new ();
	
	g_signal_connect((GtkWidget *) iconview , "selection-changed", (GCallback) SelectionChangeExo, this);
	g_signal_connect((GtkWidget *) iconview , "item-activated", (GCallback) OnDoubleClick_IconView, this);
	g_signal_connect((GtkWidget *) iconview , "focus-in-event", (GCallback) ClickActivePanel, this );

        exo_icon_view_set_pixbuf_column ( iconview, COL_ICON );

 exo_icon_view_set_orientation (iconview, GTK_ORIENTATION_HORIZONTAL );
//   GtkCellRenderer   *render;
//   render = gtk_cell_renderer_pixbuf_new();
//   gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( iconview ), render, TRUE );

//   gtk_cell_layout_add_attribute ( GTK_CELL_LAYOUT ( iconview ), render, "pixbuf", COL_ICON );
                                        

   exo_icon_view_set_text_column ( iconview, MODEL_TEXT_NAME );

//   render = gtk_cell_renderer_text_new();
//   gtk_cell_layout_pack_start( GTK_CELL_LAYOUT( iconview ), render, TRUE );
//   gtk_cell_layout_add_attribute ( GTK_CELL_LAYOUT ( iconview ), render,  "text", FIRST_TEXT_COL );

	exo_icon_view_set_selection_mode(iconview, GTK_SELECTION_MULTIPLE);
}

//-----------------------------------------------------------------------------

void ClassIconPanel::SetCursor(void)
{
	GtkTreeIter   iter;
	GtkTreeModel * model = GetModel();
	if(model)
 	if(gtk_tree_model_get_iter_first ( model, &iter))
	{
		GtkTreePath *  path =    gtk_tree_model_get_path ( model, &iter);
		if(path) exo_icon_view_select_path ( iconview, path );
	}
}

//-----------------------------------------------------------------------------

GList * ClassIconPanel::get_selected(GtkTreeModel** model)
{
        *model = exo_icon_view_get_model( iconview );
        return exo_icon_view_get_selected_items( iconview );
}

//-----------------------------------------------------------------------------

GtkTreeModel * ClassIconPanel::GetModel(void)
{
	return exo_icon_view_get_model(iconview);
}

//-----------------------------------------------------------------------------

void ClassIconPanel::SetModel(GtkTreeModel * model)
{
	GtkTreeModel * old_model=GetModel();
	if(old_model) g_object_unref( G_OBJECT( old_model ) ); //со старой моделью надо что-то делать

	if(model)
	{ 
		exo_icon_view_set_model(iconview, model);
	}
//	g_object_unref( G_OBJECT( model ) ); //со старой моделью надо что-то делать
}

//-----------------------------------------------------------------------------

GtkWidget* ClassIconPanel::GetWidget(void)
{
	return (GtkWidget*)iconview;
}

//-----------------------------------------------------------------------------

void ClassIconPanel::SetVisibleColumn(int i, int val)
{
}

//-----------------------------------------------------------------------------

void ClassIconPanel::SetColumnTitle(int i, char * name)
{
}

//-----------------------------------------------------------------------------

void ClassIconPanel::DeInit(void)
{
	GObject * g = G_OBJECT( GetWidget());
	if(g)
	{
		g_object_disconnect(g, "any_signal::selection-changed", (GCallback) SelectionChangeExo, this, NULL);
		g_object_disconnect(g, "any_signal::item-activated", (GCallback) OnDoubleClick_IconView, this, NULL);
		g_object_disconnect(g, "any_signal::focus-in-event", (GCallback) ClickActivePanel, this, NULL );
		SetModel(0);
		gtk_widget_destroy( GetWidget() );
		iconview = 0;
	}
}

//-----------------------------------------------------------------------------

void ClassIconPanel::SelectAll(void)
{
	exo_icon_view_select_all(iconview);
}

//-----------------------------------------------------------------------------

void ClassIconPanel::UnSelectAll(void)
{
	exo_icon_view_unselect_all(iconview);
}

//-----------------------------------------------------------------------------

void ClassIconPanel::SelectPattern(const char * pattern)
{
	//if(check_pattern (model,path)) exo_icon_view_select_path ( EXO_ICON_VIEW( file_browser->folder_view ), path );
}
//-----------------------------------------------------------------------------
