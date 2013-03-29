#ifndef _icon_cell_renderer_included_
  #define _icon_cell_renderer_included_

  #include <gtk/gtk.h>


  #define CUSTOM_TYPE_CELL_RENDERER_PROGRESS             (custom_cell_renderer_progress_get_type())
  #define CUSTOM_CELL_RENDERER_PROGRESS(obj)             (G_TYPE_CHECK_INSTANCE_CAST((obj),  CUSTOM_TYPE_CELL_RENDERER_PROGRESS, CustomCellRendererProgress))
  #define CUSTOM_CELL_RENDERER_PROGRESS_CLASS(klass)     (G_TYPE_CHECK_CLASS_CAST ((klass),  CUSTOM_TYPE_CELL_RENDERER_PROGRESS, CustomCellRendererProgressClass))
  #define CUSTOM_IS_CELL_PROGRESS_PROGRESS(obj)          (G_TYPE_CHECK_INSTANCE_TYPE ((obj), CUSTOM_TYPE_CELL_RENDERER_PROGRESS))
  #define CUSTOM_IS_CELL_PROGRESS_PROGRESS_CLASS(klass)  (G_TYPE_CHECK_CLASS_TYPE ((klass),  CUSTOM_TYPE_CELL_RENDERER_PROGRESS))
  #define CUSTOM_CELL_RENDERER_PROGRESS_GET_CLASS(obj)   (G_TYPE_INSTANCE_GET_CLASS ((obj),  CUSTOM_TYPE_CELL_RENDERER_PROGRESS, CustomCellRendererProgressClass))

  typedef struct _CustomCellRendererProgress CustomCellRendererProgress;
  typedef struct _CustomCellRendererProgressClass CustomCellRendererProgressClass;

  /* CustomCellRendererProgress: Our custom cell renderer
   *   structure. Extend according to need */

  struct _CustomCellRendererProgress
  {
    GtkCellRendererPixbuf	parent;
    gdouble					progress;
	int						mime_type;
	int						file_type;	  
  };
 

  struct _CustomCellRendererProgressClass
  {
    GtkCellRendererPixbufClass  parent_class;
  };


  GType                icon_cell_renderer_get_type (void);

  GtkCellRenderer     *icon_cell_renderer_new (void);


  #endif

