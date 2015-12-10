#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define STORYREADER_TYPE            (storyreader_get_type ())
#define STORYREADER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STORYREADER_TYPE, StoryReader))
#define STORYREADER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STORYREADER_TYPE, StoryReaderClass))
#define IS_STORYREADER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STORYREADER_TYPE))
#define IS_STORYREADER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STORYREADER_TYPE))
#define STORYREADER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STORYREADER_TYPE, StoryReaderClass))

typedef struct _StoryReader StoryReader;
typedef struct _StoryReaderClass  StoryReaderClass;
struct _Settings;
struct _Picture;
struct _Sound;
struct _MainWindow;

struct _StoryReader 
  {
  GObject base;
  gboolean dispose_has_run;
  struct _StoryReaderPriv *priv;
  };

struct _StoryReaderClass
  {
  GObjectClass parent_class;
  void (* storyreader) (StoryReader *it);
  };

GType storyreader_get_type (void);

StoryReader *storyreader_new (struct _MainWindow *main_window);

struct _Interpreter *storyreader_open (StoryReader *self, 
    const char *filename, const char *tempdir, 
    const struct _Settings *settings, GError **error);

void storyreader_close (StoryReader *self);
void storyreader_show_dialog (const StoryReader *self, GtkWindow *w);
const char *storyreader_get_title (const StoryReader *self);

int storyreader_get_num_pictures (const StoryReader *self);

const struct _Picture *storyreader_get_picture_by_znum 
  (const StoryReader *self, int znum);

const struct _Sound *storyreader_get_sound_by_znum (const StoryReader *self, 
    int znum);

G_END_DECLS




