#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "Settings.h"

G_DEFINE_TYPE (Settings, settings, G_TYPE_OBJECT);


/*======================================================================
  settings_init
=====================================================================*/

static void settings_init (Settings *self)
{
  g_debug ("Initialising settings\n");
  self->dispose_has_run = FALSE;
  self->user_random_seed = -1;
  self->user_screen_height = 25;
  self->user_screen_width = 80;
  self->user_tandy_bit = 0;
  self->user_interpreter_number = 0;
  self->font_name_main = strdup ("Serif");
  self->font_name_fixed = strdup ("Monospace");
  self->font_size = 10;
  self->filename = NULL;
  self->speed = 0; // No timed input
  self->graphics_width = 320;
  self->graphics_height = 200;
  self->foreground_rgb8 = RGB8BLACK;
  self->background_rgb8 = RGB8WHITE;
  self->top_margin = 0;
  self->bottom_margin = 0;
  self->left_margin = 10;
  self->right_margin = 10;
  self->error_level = 1; // Report once
  self->ignore_game_colours = FALSE;
}


/*======================================================================
  settings_dispose
======================================================================*/

static void settings_dispose (GObject *_self)
{
  Settings *self = (Settings *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing settings\n");
  self->dispose_has_run = TRUE;
  if (self->filename)
  {
    free (self->filename);
    self->filename = NULL;
  }
}


/*======================================================================
  settings_class_init
======================================================================*/
static void settings_class_init
  (SettingsClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = settings_dispose;
}


/*======================================================================
  settings_new
  We will copy the filename internally
======================================================================*/
Settings *settings_new (const char *filename)
{
  Settings *self = g_object_new 
   (SETTINGS_TYPE, NULL);
  self->filename = strdup (filename);
  return self;
}


/*======================================================================
  settings_clone
  Deep copy of the specified settings object
======================================================================*/
Settings *settings_clone (const Settings *other)
{
  Settings *self = g_object_new 
   (SETTINGS_TYPE, NULL);
  settings_copy (self, other);
  return self;
}


/*======================================================================
  settings_clone
  Deep copy of the specified settings object onto this one
======================================================================*/
void settings_copy (Settings *self, const Settings *other)
{
  if (other->filename)
    self->filename = strdup (other->filename);
  if (other->font_name_main)
    self->font_name_main = strdup (other->font_name_main);
  if (other->font_name_fixed)
    self->font_name_fixed = strdup (other->font_name_fixed);
  self->user_screen_width = other->user_screen_width;
  self->user_screen_height = other->user_screen_height;
  self->user_random_seed = other->user_random_seed;
  self->user_tandy_bit = other->user_tandy_bit;
  self->user_interpreter_number = other->user_interpreter_number;
  self->font_size = other->font_size;
  self->speed = other->speed;
  self->foreground_rgb8 = other->foreground_rgb8;
  self->background_rgb8 = other->background_rgb8;
  self->graphics_width = other->graphics_width;
  self->graphics_height = other->graphics_height;
  self->top_margin = other->top_margin;
  self->bottom_margin = other->bottom_margin;
  self->left_margin = other->left_margin;
  self->right_margin = other->right_margin;
  self->error_level = other->error_level;
  self->ignore_game_colours = other->ignore_game_colours;
}


/*======================================================================
  settings_set_font_name_main
======================================================================*/
void settings_set_font_name_main (Settings *self, const char *name)
  {
  if (name == NULL) return;
  if (strlen (name) == 0) return;
  if (self->font_name_main) free (self->font_name_main);
  self->font_name_main = strdup (name);
  }


/*======================================================================
  settings_set_font_name_fixed
======================================================================*/
void settings_set_font_name_fixed (Settings *self, const char *name)
  {
  if (name == NULL) return;
  if (strlen (name) == 0) return;
  if (self->font_name_fixed) free (self->font_name_fixed);
  self->font_name_fixed = strdup (name);
  }


/*======================================================================
  settings_save
======================================================================*/
void settings_save (const Settings *self)
  {
  g_message ("Saving settings to %s", self->filename);
  FILE *f = fopen (self->filename, "wt");
  if (f)
    {
    fprintf (f, "[UI]\n");
    fprintf (f, "user_screen_width=%d\n", self->user_screen_width);
    fprintf (f, "user_screen_height=%d\n", self->user_screen_height);
    fprintf (f, "font_name_main=%s\n", self->font_name_main);
    fprintf (f, "font_name_fixed=%s\n", self->font_name_fixed);
    fprintf (f, "font_size=%d\n", self->font_size);
    fprintf (f, "background_rgb8=%d\n", self->background_rgb8);
    fprintf (f, "foreground_rgb8=%d\n", self->foreground_rgb8);
    fprintf (f, "graphics_width=%d\n", self->graphics_width);
    fprintf (f, "graphics_height=%d\n", self->graphics_height);
    fprintf (f, "top_margin=%d\n", self->top_margin);
    fprintf (f, "bottom_margin=%d\n", self->bottom_margin);
    fprintf (f, "left_margin=%d\n", self->left_margin);
    fprintf (f, "right_margin=%d\n", self->right_margin);
    fprintf (f, "error_level=%d\n", self->error_level);
    fprintf (f, "ignore_game_colours=%d\n", self->ignore_game_colours);

    fprintf (f, "[frotz]\n");
    fprintf (f, "user_interpreter_number=%d\n", self->user_interpreter_number);
    fprintf (f, "user_random_seed=%d\n", self->user_random_seed);
    fprintf (f, "speed=%d\n", self->speed);
    fclose (f);
    }
  else
    g_message ("Can't write file %s", self->filename); 
  }


/*======================================================================
  settings_load
======================================================================*/
void settings_load (Settings *self)
  {
  g_message ("Reading settings from %s", self->filename);

  GError *error = NULL;
  GKeyFile *kf = g_key_file_new();
  g_key_file_load_from_file (kf, self->filename, 0, &error);
  if (error)
    {
    // This is not necessarily an error -- ini file need not exist
    g_message ("Failed to read from %s", self->filename);
    g_error_free (error);
    }
  else
    {
    error = NULL;
    if (error) g_error_free (error); error = NULL;
    settings_set_font_name_fixed (self, g_key_file_get_value 
      (kf, "UI", "font_name_fixed", &error));
    if (error) g_error_free (error); error = NULL;
    settings_set_font_name_main (self, g_key_file_get_value 
      (kf, "UI", "font_name_main", &error));
    if (error) g_error_free (error); error = NULL;
    int v = g_key_file_get_integer (kf, "UI", "user_screen_width", &error);
    if (!error) self->user_screen_width = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "graphics_height", &error);
    if (!error) self->graphics_height = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "graphics_width", &error);
    if (!error) self->graphics_width = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "user_screen_height", &error);
    if (!error) self->user_screen_height = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "font_size", &error);
    if (!error) self->font_size = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "foreground_rgb8", &error);
    if (!error) self->foreground_rgb8 = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "background_rgb8", &error);
    if (!error) self->background_rgb8 = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "frotz", "speed", &error);
    if (!error) self->speed = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "frotz", "user_interpreter_number", &error);
    if (!error) self->user_interpreter_number = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "frotz", "user_random_seed", &error);
    if (!error) self->user_random_seed = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "top_margin", &error);
    if (!error) self->top_margin = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "bottom_margin", &error);
    if (!error) self->bottom_margin = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "left_margin", &error);
    if (!error) self->left_margin = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "right_margin", &error);
    if (!error) self->right_margin = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "error_level", &error);
    if (!error) self->error_level = v;
    if (error) g_error_free (error); error = NULL;
    v = g_key_file_get_integer (kf, "UI", "ignore_game_colours", &error);
    if (!error) self->ignore_game_colours = v;
    if (error) g_error_free (error); error = NULL;
    }

  g_key_file_free (kf);
  }



