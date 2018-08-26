#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <sys/stat.h>
#include "MediaPlayer.h"
#include "fileutils.h"

G_DEFINE_TYPE (MediaPlayer, mediaplayer, G_TYPE_OBJECT);

typedef struct _MediaPlayerPriv
  {
  int repeats;
  MediaPlayerNotificationCallback callback;
  void *user_data1;
  int user_data2;
  } MediaPlayerPriv;


static MediaPlayer *mediaplayer_instance;

/*======================================================================
  mediaplayer_init
=====================================================================*/

static void mediaplayer_init (MediaPlayer *self)
{
  g_debug ("Initialising mediaplayer\n");
  self->dispose_has_run = FALSE;
  self->priv = (MediaPlayerPriv *) malloc (sizeof (MediaPlayerPriv));
  memset (self->priv, 0, sizeof (MediaPlayerPriv));
  self->priv->repeats = 0;
}


/*======================================================================
  mediaplayer_dispose
======================================================================*/

static void mediaplayer_dispose (GObject *_self)
{
  MediaPlayer *self = (MediaPlayer *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing mediaplayer\n");
  mediaplayer_stop (self);
  self->dispose_has_run = TRUE;
  if (self->priv)
  {
    free (self->priv);
    self->priv = NULL;
  }
  G_OBJECT_CLASS (mediaplayer_parent_class)->dispose (_self);
}


/*======================================================================
  mediaplayer_class_init
======================================================================*/
static void mediaplayer_class_init
  (MediaPlayerClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = mediaplayer_dispose;
}


/*======================================================================
  mediaplayer_new_protected
======================================================================*/
static MediaPlayer *mediaplayer_new_protected (void)
{
  MediaPlayer *self = g_object_new 
   (MEDIAPLAYER_TYPE, NULL);
  self->priv->repeats = 0;
  return self;
}

/*======================================================================
  mediaplayer_new
======================================================================*/
MediaPlayer *mediaplayer_get_instance (void)
{
  if (!mediaplayer_instance) 
    mediaplayer_instance = mediaplayer_new_protected ();
  return mediaplayer_instance;
}



/*======================================================================
  mediaplayer_play_file
======================================================================*/
GString *mediaplayer_get_script_file (const MediaPlayer *self)
  {
  GString *argv0 = fileutils_get_full_argv0();
  GString *dir = fileutils_get_dirname (argv0->str);

#ifdef WIN32
  char *scriptname = "grotz_playmedia.bat";
#else
  char *scriptname = "grotz_playmedia.sh";
#endif

  GString *script = fileutils_concat_path (dir->str, scriptname);

  g_string_free (dir, TRUE);
  g_string_free (argv0, TRUE);

  return script;
  }


/*======================================================================
  mediaplayer_handle_eos
  Playback finished -- invoke callback
======================================================================*/
void mediaplayer_handle_eos (MediaPlayer *self)
  {
  if (self->priv->callback)
    self->priv->callback (MEDIAPLAYER_NOTIFICATION_FINISHED,
      self->priv->user_data1, self->priv->user_data2);
  self->priv->user_data1 = 0;
  self->priv->user_data2 = 0;
  }

/*======================================================================
  mediaplayer_play_file
======================================================================*/
void mediaplayer_play_file (MediaPlayer *self, const char *file, double volume, 
     int repeats, MediaPlayerNotificationCallback callback, void *user_data1,
     int user_data2, GError **error)
  {
  g_debug ("mediaplayer_play_file file=%s volume=%f repeats=%d",
     file, volume, repeats);

  char svol[10];
  snprintf (svol, sizeof (svol), "%d", (int)((double)volume * 100.0));

  /*
  char *svol;
  if (volume < 0.1) svol = "50";
  else if (volume < 0.3) svol = "65";
  else if (volume < 0.4) svol = "70";
  else if (volume < 0.5) svol = "75";
  else if (volume < 0.6) svol = "80";
  else if (volume < 0.7) svol = "85";
  else if (volume < 0.8) svol = "90";
  else if (volume < 0.9) svol = "95";
  else svol = "100";
  */ 

  char srepeats[10];
  if (repeats < 0) repeats = 100; // Effectively forever
  snprintf (srepeats, sizeof (srepeats), "%d", repeats);

  self->priv->callback = callback;
  self->priv->user_data1 = user_data1;
  self->priv->user_data2 = user_data2;

  GString *script_file = mediaplayer_get_script_file (self);

  g_debug ("media script file is %s", script_file->str);

#ifdef WIN32
  // Dontcha just hate Windows?
  g_string_append (script_file, "\"");
  g_string_prepend (script_file, "\"");
  spawnlp(_P_NOWAIT, "cmd", "cmd", "/c", script_file->str, "p", 
    file, svol, srepeats, NULL);
#else
  if (fork() == 0)
    {
    // Child
    execlp (script_file->str, script_file->str, "p", 
    file, svol, srepeats, NULL);
    exit (0);
    }
#endif

  g_string_free (script_file, TRUE);

  // Unfortunately, we don't have any straightforward way of calling the
  // callback function when we invoke sounds this way
  // For Linux, the SIGCHLD alarm handerl should do it. No solution
  // for Windows yet
  }
 

/*======================================================================
  mediaplayer_stop_
======================================================================*/
void mediaplayer_stop (MediaPlayer *self)
  {
  g_debug ("mediaplayer_stop");

  GString *script_file = mediaplayer_get_script_file (self);

#ifdef WIN32
  // Dontcha just hate Windows?
  g_string_append (script_file, "\"");
  g_string_prepend (script_file, "\"");
  spawnlp(_P_NOWAIT, "cmd", "cmd", "/c", script_file->str, "s", NULL);
#else
  if (fork() == 0)
    {
    // Child
    execlp (script_file->str, script_file->str, "s", NULL);
    exit (0);
    }
#endif

  g_string_free (script_file, TRUE);
  }
 




