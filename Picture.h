#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
  {
  PICTURE_FORMAT_PNG = 0,
  PICTURE_FORMAT_DUMMY = 1,
  PICTURE_FORMAT_JPG = 2
  } PictureFormat;

#define PICTURE_TYPE            (picture_get_type ())
#define PICTURE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), PICTURE_TYPE, Picture))
#define PICTURE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), PICTURE_TYPE, PictureClass))
#define IS_PICTURE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), PICTURE_TYPE))
#define IS_PICTURE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), PICTURE_TYPE))
#define PICTURE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), PICTURE_TYPE, PictureClass))

typedef struct _Picture Picture;
typedef struct _PictureClass  PictureClass;

// Don't forget to modify clone() if anything is added to this object
struct _Picture 
  {
  GObject base;
  gboolean dispose_has_run;
  int znum;
  int width;
  int height;
  int format;
  unsigned char *data;
  int data_len;
  };

struct _PictureClass
  {
  GObjectClass parent_class;
  void (* picture) (Picture *it);
  };

GType picture_get_type (void);

Picture *picture_new (int znum, int width, int height, PictureFormat format);
void picture_set_data (Picture *self, const unsigned char *data, int len); 
GdkPixbuf *picture_make_pixbuf (const Picture *self, int max_width,
  int max_height);

G_END_DECLS




