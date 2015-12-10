#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "Picture.h"

G_DEFINE_TYPE (Picture, picture, G_TYPE_OBJECT);


/*======================================================================
  picture_init
=====================================================================*/

static void picture_init (Picture *self)
{
  g_debug ("Initialising picture\n");
  self->dispose_has_run = FALSE;
}


/*======================================================================
  picture_dispose
======================================================================*/

static void picture_dispose (GObject *_self)
{
  Picture *self = (Picture *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing picture\n");
  self->dispose_has_run = TRUE;
  if (self->data)
  {
    free (self->data);
    self->data = NULL;
  }
  G_OBJECT_CLASS (picture_parent_class)->dispose (_self);
}


/*======================================================================
  picture_class_init
======================================================================*/
static void picture_class_init
  (PictureClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = picture_dispose;
}


/*======================================================================
  picture_new
======================================================================*/
Picture *picture_new (int znum, int width, int height, PictureFormat format)
{
  Picture *self = g_object_new 
   (PICTURE_TYPE, NULL);
  self->format = format;
  self->width = width;
  self->height = height;
  self->znum = znum;
  return self;
}


/*======================================================================
  picture_set_data
======================================================================*/
void picture_set_data (Picture *self, const unsigned char *data, int len)
{
  if (self->data)
    free (self->data);

  self->data = (unsigned char *)malloc (len);
  memcpy (self->data, data, len);
  self->data_len = len;
}


/*======================================================================
  picture_make_pixbuf
  Caller must free the pixbuf if non-null. Bear in mind that some
  zcode pictures are dummies, and have no data attached. So these
  will not render into pixbufs
======================================================================*/
GdkPixbuf *picture_make_pixbuf (const Picture *self, int max_width,
  int max_height) 
{
  if (self->data == NULL) return NULL;

  GError *error = NULL;
  GInputStream *s = g_memory_input_stream_new_from_data 
    (self->data, self->data_len, NULL); 
  GdkPixbuf *pb = gdk_pixbuf_new_from_stream (s, NULL, &error);
  g_object_unref (s);
  if (error)
    {
    g_debug (error->message);
    g_error_free (error);
    return NULL;
    }

  error = NULL;

  int pbw = gdk_pixbuf_get_width (pb);
  int pbh = gdk_pixbuf_get_height (pb);

  if (pbw < max_width && pbw < max_height) return pb;

  int new_w = pbw;
  int new_h = pbh;

  gboolean portrait = FALSE;
  if (pbh > pbw) portrait = TRUE;

  if (portrait)
    {
    new_h = max_height;
    new_w = max_height *pbw / pbh;
    }
  else
    {
    new_w = max_width;
    new_h = max_width *pbh / pbw;
    }

  // Aspect ratio should be correct at this point, but may be
  //  still too large

  double scale = 1.0;
  if (new_w > max_width)
    scale = (double) max_width / (double) new_w; 
  if (new_h > max_height)
    scale = (double) max_height / (double) new_h; 
 
  new_w *= scale;
  new_h *= scale;

  GdkPixbuf *pbs = gdk_pixbuf_scale_simple (pb, 
        new_w, new_h, 
        GDK_INTERP_BILINEAR);    

  g_object_unref (pb);
  return pbs;
}  




