#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define METADATA_TYPE            (metadata_get_type ())
#define METADATA(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), METADATA_TYPE, MetaData))
#define METADATA_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), METADATA_TYPE, MetaDataClass))
#define IS_METADATA(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), METADATA_TYPE))
#define IS_METADATA_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), METADATA_TYPE))
#define METADATA_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), METADATA_TYPE, MetaDataClass))

typedef struct _MetaData MetaData;
typedef struct _MetaDataClass  MetaDataClass;

struct _MetaData 
  {
  GObject base;
  gboolean dispose_has_run;
  char *author;
  char *title;
  char *description;
  int fspc_image;
  };

struct _MetaDataClass
  {
  GObjectClass parent_class;
  void (* metadata) (MetaData *it);
  };

GType metadata_get_type (void);

MetaData *metadata_new_from_file (const char *filename, GError **error);

G_END_DECLS




