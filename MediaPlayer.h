#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

typedef enum
  {
  MEDIAPLAYER_NOTIFICATION_FINISHED = 0
  } MediaPlayerNotificationType;

#define MEDIAPLAYER_TYPE            (mediaplayer_get_type ())
#define MEDIAPLAYER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MEDIAPLAYER_TYPE, MediaPlayer))
#define MEDIAPLAYER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MEDIAPLAYER_TYPE, MediaPlayerClass))
#define IS_MEDIAPLAYER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MEDIAPLAYER_TYPE))
#define IS_MEDIAPLAYER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MEDIAPLAYER_TYPE))
#define MEDIAPLAYER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MEDIAPLAYER_TYPE, MediaPlayerClass))

typedef struct _MediaPlayer MediaPlayer;
typedef struct _MediaPlayerClass  MediaPlayerClass;

typedef void (*MediaPlayerNotificationCallback) 
  (MediaPlayerNotificationType notification, void *user_data1, int user_data2); 

// Don't forget to modify clone() if anything is added to this object
struct _MediaPlayer 
  {
  GObject base;
  gboolean dispose_has_run;
  struct _MediaPlayerPriv *priv;
  };

struct _MediaPlayerClass
  {
  GObjectClass parent_class;
  void (* mediaplayer) (MediaPlayer *it);
  };

GType mediaplayer_get_type (void);

MediaPlayer *mediaplayer_get_instance (void);

void mediaplayer_play_file (MediaPlayer *self, const char *file, double volume, 
     int repeats, MediaPlayerNotificationCallback callback, void *user_data1,
     int user_data2, GError **error);

void mediaplayer_stop (MediaPlayer *self);

G_END_DECLS




