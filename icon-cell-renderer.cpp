#include "icon-cell-renderer.h"
#include "tree.h"


static void     custom_cell_renderer_progress_init       (CustomCellRendererProgress      *cellprogress);

static void     custom_cell_renderer_progress_class_init (CustomCellRendererProgressClass *klass);

static void     custom_cell_renderer_progress_get_property  (GObject                    *object,
                                                             guint                       param_id,
                                                             GValue                     *value,
                                                             GParamSpec                 *pspec);

static void     custom_cell_renderer_progress_set_property  (GObject                    *object,
                                                             guint                       param_id,
                                                             const GValue               *value,
                                                             GParamSpec                 *pspec);

static void     custom_cell_renderer_progress_finalize (GObject *gobject);


/* These functions are the heart of our custom cell renderer: */

static void     custom_cell_renderer_progress_get_size   (GtkCellRenderer            *cell,
                                                          GtkWidget                  *widget,
                                                          GdkRectangle               *cell_area,
                                                          gint                       *x_offset,
                                                          gint                       *y_offset,
                                                          gint                       *width,
                                                          gint                       *height);

static void     custom_cell_renderer_progress_render     (GtkCellRenderer            *cell,
                                                          GdkWindow                  *window,
                                                          GtkWidget                  *widget,
                                                          GdkRectangle               *background_area,
                                                          GdkRectangle               *cell_area,
                                                          GdkRectangle               *expose_area,
                                                          guint                       flags);


enum
{
	PROP_PERCENTAGE = 1,
	PROP_MIME_TYPE,
	PROP_FILE_TYPE		
};

static   gpointer parent_class;


/***************************************************************************
 *
 *  custom_cell_renderer_progress_get_type: here we register our type with
 *                                          the GObject type system if we
 *                                          haven't done so yet. Everything
 *                                          else is done in the callbacks.
 *
 ***************************************************************************/

GType
custom_cell_renderer_progress_get_type (void)
{
  static GType cell_progress_type = 0;

  if (cell_progress_type)
    return cell_progress_type;

  if (1)
  {
    static const GTypeInfo cell_progress_info =
    {
      sizeof (CustomCellRendererProgressClass),
      NULL,                                                     /* base_init */
      NULL,                                                     /* base_finalize */
      (GClassInitFunc) custom_cell_renderer_progress_class_init,
      NULL,                                                     /* class_finalize */
      NULL,                                                     /* class_data */
      sizeof (CustomCellRendererProgress),
      0,                                                        /* n_preallocs */
      (GInstanceInitFunc) custom_cell_renderer_progress_init,
    };

    /* Derive from GtkCellRenderer */
    cell_progress_type = g_type_register_static (GTK_TYPE_CELL_RENDERER_PIXBUF, //VV GTK_TYPE_CELL_RENDERER,
                                                 "CustomCellRendererProgress",
                                                  &cell_progress_info,
                                                  (GTypeFlags)0);
  }

  return cell_progress_type;
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_init: set some default properties of the
 *                                      parent (GtkCellRenderer).
 *
 ***************************************************************************/

static void
custom_cell_renderer_progress_init (CustomCellRendererProgress *cellrendererprogress)
{
  GTK_CELL_RENDERER(cellrendererprogress)->mode = GTK_CELL_RENDERER_MODE_INERT;
  GTK_CELL_RENDERER(cellrendererprogress)->xpad = 2;
  GTK_CELL_RENDERER(cellrendererprogress)->ypad = 2;
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_class_init:
 *
 *  set up our own get_property and set_property functions, and
 *  override the parent's functions that we need to implement.
 *  And make our new "percentage" property known to the type system.
 *  If you want cells that can be activated on their own (ie. not
 *  just the whole row selected) or cells that are editable, you
 *  will need to override 'activate' and 'start_editing' as well.
 *
 ***************************************************************************/
typedef void (AAA)(GtkCellRenderer*, GdkWindow*, GtkWidget*, GdkRectangle*, GdkRectangle*, GdkRectangle*, guint);
typedef void (BBB)(GtkCellRenderer*, GdkDrawable*, GtkWidget*, GdkRectangle*, GdkRectangle*, GdkRectangle*, GtkCellRendererState);

static void
custom_cell_renderer_progress_class_init (CustomCellRendererProgressClass *klass)
{
  GtkCellRendererClass *cell_class   = GTK_CELL_RENDERER_CLASS(klass);
  GObjectClass         *object_class = G_OBJECT_CLASS(klass);

  parent_class           = g_type_class_peek_parent (klass);
  object_class->finalize = custom_cell_renderer_progress_finalize;

  /* Hook up functions to set and get our
   *   custom cell renderer properties */
  object_class->get_property = custom_cell_renderer_progress_get_property;
  object_class->set_property = custom_cell_renderer_progress_set_property;

  /* Override the two crucial functions that are the heart
   *   of a cell renderer in the parent class */
  cell_class->get_size = custom_cell_renderer_progress_get_size;

  cell_class->render   = (BBB*)custom_cell_renderer_progress_render;

  /* Install our very own properties */
	g_object_class_install_property (object_class,
                                   PROP_MIME_TYPE,
                                   g_param_spec_int ("mime_type",
                                                     "mime_type",
                                                     "For Icon mime_type",
                                                         0, 0xFFFF, 0,
                                                         (GParamFlags)G_PARAM_READWRITE));
	g_object_class_install_property (object_class,
                                   PROP_FILE_TYPE,
                                   g_param_spec_int ("file_type",
                                                     "file_type",
                                                     "For Icon file_type",
                                                         0, 0xFFFF, 0,
                                                         (GParamFlags)G_PARAM_READWRITE));

	g_object_class_install_property (object_class,
                                   PROP_PERCENTAGE,
                                   g_param_spec_double ("percentage",
                                                        "Percentage",
                                                         "The fractional progress to display",
                                                         0, 1, 0,
                                                         (GParamFlags)G_PARAM_READWRITE));
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_finalize: free any resources here
 *
 ***************************************************************************/

static void
custom_cell_renderer_progress_finalize (GObject *object)
{
/*
  CustomCellRendererProgress *cellrendererprogress = CUSTOM_CELL_RENDERER_PROGRESS(object);
*/

  /* Free any dynamically allocated resources here */

  (* G_OBJECT_CLASS (parent_class)->finalize) (object);
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_get_property: as it says
 *
 ***************************************************************************/

static void
custom_cell_renderer_progress_get_property (GObject    *object,
                                            guint       param_id,
                                            GValue     *value,
                                            GParamSpec *psec)
{
  CustomCellRendererProgress  *cellprogress = CUSTOM_CELL_RENDERER_PROGRESS(object);

  switch (param_id)
  {
    case PROP_PERCENTAGE:
      g_value_set_double(value, cellprogress->progress);
      break;

	case PROP_MIME_TYPE:
      g_value_set_int(value, cellprogress->mime_type);
      break;

	case PROP_FILE_TYPE:
      g_value_set_int(value, cellprogress->file_type);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID (object, param_id, psec);
      break;
  }
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_set_property: as it says
 *
 ***************************************************************************/

static void
custom_cell_renderer_progress_set_property (GObject      *object,
                                            guint         param_id,
                                            const GValue *value,
                                            GParamSpec   *pspec)
{
  CustomCellRendererProgress *cellprogress = CUSTOM_CELL_RENDERER_PROGRESS (object);

  switch (param_id)
  {
    case PROP_PERCENTAGE:
      cellprogress->progress = g_value_get_double(value);
      break;

	case PROP_MIME_TYPE:
      cellprogress->mime_type = g_value_get_int(value);
      break;

	case PROP_FILE_TYPE:
      cellprogress->file_type = g_value_get_int(value);
      break;

    default:
      G_OBJECT_WARN_INVALID_PROPERTY_ID(object, param_id, pspec);
      break;
  }
}

/***************************************************************************
 *
 *  custom_cell_renderer_progress_new: return a new cell renderer instance
 *
 ***************************************************************************/

GtkCellRenderer *
icon_cell_renderer_new (void)
{
  return (GtkCellRenderer*)g_object_new(CUSTOM_TYPE_CELL_RENDERER_PROGRESS, NULL);
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_get_size: crucial - calculate the size
 *                                          of our cell, taking into account
 *                                          padding and alignment properties
 *                                          of parent.
 *
 ***************************************************************************/

#define FIXED_WIDTH   16
#define FIXED_HEIGHT  16

static void
custom_cell_renderer_progress_get_size (GtkCellRenderer *cell,
                                        GtkWidget       *widget,
                                        GdkRectangle    *cell_area,
                                        gint            *x_offset,
                                        gint            *y_offset,
                                        gint            *width,
                                        gint            *height)
{

 
	gint calc_width;
	gint calc_height;

  calc_width  = (gint) cell->xpad * 2 + FIXED_WIDTH;
  calc_height = (gint) cell->ypad * 2 + FIXED_HEIGHT;

  if (width)
    *width = calc_width;

  if (height)
    *height = calc_height;

  if (cell_area)
  {
    if (x_offset)
    {
      *x_offset = cell->xalign * (cell_area->width - calc_width);
      *x_offset = MAX (*x_offset, 0);
    }

    if (y_offset)
    {
      *y_offset = cell->yalign * (cell_area->height - calc_height);
      *y_offset = MAX (*y_offset, 0);
    }
  }
}


/***************************************************************************
 *
 *  custom_cell_renderer_progress_render: crucial - do the rendering.
 *
 ***************************************************************************/
//static int xxx;
static void
custom_cell_renderer_progress_render (GtkCellRenderer *cell,
                                      GdkWindow       *window,
                                      GtkWidget       *widget,
                                      GdkRectangle    *background_area,
                                      GdkRectangle    *cell_area,
                                      GdkRectangle    *expose_area,
                                      guint            flags)
{

	GdkRectangle pix_rect;
	cairo_t *cr;

	CustomCellRendererProgress *cellprogress = CUSTOM_CELL_RENDERER_PROGRESS (cell);

	GtkCellRendererPixbuf * cellpixbuf = ( GtkCellRendererPixbuf * ) cell;
	GdkPixbuf *pixbuf = cellpixbuf->pixbuf;

    pix_rect.x = cell_area->x+cell->ypad;
    pix_rect.y = cell_area->y+cell->ypad;
	pix_rect.width=16;
	pix_rect.height=16;

	if(!pixbuf)
	{
//printf("%d->%d\n",xxx++,cellprogress->mime_type);
		pixbuf=IconPixbuf[0xFF & cellprogress->mime_type];
		if(!pixbuf) return;

		cr = gdk_cairo_create ( window );
		gdk_cairo_set_source_pixbuf ( cr, pixbuf,
                                          pix_rect.x,
                                          pix_rect.y);

		gdk_cairo_rectangle ( cr, &pix_rect );
        cairo_fill ( cr );
		if((cellprogress->mime_type & MIME_FLAG_LINK) && PixbufEmblemLink)
		{
//			pix_rect.x+=8;
//			pix_rect.y+=8;
			pix_rect.width=8;
			pix_rect.height=8;

			gdk_cairo_set_source_pixbuf ( cr, PixbufEmblemLink,
                                          pix_rect.x,
                                          pix_rect.y);

			gdk_cairo_rectangle ( cr, &pix_rect );
  
    	    cairo_fill ( cr );
		}
		
		if(cellprogress->mime_type & MIME_FLAG_DROPBOX_OK)	
			pixbuf=PixbufEmblemDropboxOk; else
		if(cellprogress->mime_type & MIME_FLAG_DROPBOX_SYNC) 
			pixbuf=PixbufEmblemDropboxSync; else pixbuf=0;

		if(pixbuf) 
		{
			pix_rect.x+=8;
			pix_rect.y+=8;
			pix_rect.width=8;
			pix_rect.height=8;
			gdk_cairo_set_source_pixbuf(cr,pixbuf,pix_rect.x,pix_rect.y);
			gdk_cairo_rectangle ( cr, &pix_rect );
    	    cairo_fill ( cr );
		}

		cairo_destroy(cr);
	}
	
}


