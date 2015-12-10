#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
  {
  SOUND_FORMAT_AIFF = 0,
  SOUND_FORMAT_OGG = 1,
  SOUND_FORMAT_MOD = 2,
  } SoundFormat;

#define SOUND_TYPE            (sound_get_type ())
#define SOUND(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SOUND_TYPE, Sound))
#define SOUND_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SOUND_TYPE, SoundClass))
#define IS_SOUND(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SOUND_TYPE))
#define IS_SOUND_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SOUND_TYPE))
#define SOUND_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SOUND_TYPE, SoundClass))

typedef struct _Sound Sound;
typedef struct _SoundClass  SoundClass;

struct _Sound 
  {
  GObject base;
  gboolean dispose_has_run;
  int znum;
  int format;
  unsigned char *data;
  int data_len;
  };

struct _SoundClass
  {
  GObjectClass parent_class;
  void (* sound) (Sound *it);
  };

GType sound_get_type (void);

Sound *sound_new (int znum, SoundFormat format);
void sound_set_data (Sound *self, const unsigned char *data, int len); 

G_END_DECLS




