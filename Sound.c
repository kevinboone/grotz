#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "Sound.h"

G_DEFINE_TYPE (Sound, sound, G_TYPE_OBJECT);


/*======================================================================
  sound_init
=====================================================================*/

static void sound_init (Sound *self)
{
  g_debug ("Initialising sound\n");
  self->dispose_has_run = FALSE;
}


/*======================================================================
  sound_dispose
======================================================================*/

static void sound_dispose (GObject *_self)
{
  Sound *self = (Sound *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing sound\n");
  self->dispose_has_run = TRUE;
  if (self->data)
  {
    free (self->data);
    self->data = NULL;
  }
  G_OBJECT_CLASS (sound_parent_class)->dispose (_self);
}


/*======================================================================
  sound_class_init
======================================================================*/
static void sound_class_init
  (SoundClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = sound_dispose;
}


/*======================================================================
  sound_new
======================================================================*/
Sound *sound_new (int znum, SoundFormat format)
{
  Sound *self = g_object_new 
   (SOUND_TYPE, NULL);
  self->format = format;
  self->znum = znum;
  return self;
}


/*======================================================================
  sound_set_data
======================================================================*/
void sound_set_data (Sound *self, const unsigned char *data, int len)
{
  if (self->data)
    free (self->data);

  self->data = (unsigned char *)malloc (len);
  memcpy (self->data, data, len);
  self->data_len = len;
}




