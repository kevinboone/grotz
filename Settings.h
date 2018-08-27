#pragma once

#include <gtk/gtk.h>
#include "colourutils.h" 

G_BEGIN_DECLS

#define SETTINGS_TYPE            (settings_get_type ())
#define SETTINGS(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SETTINGS_TYPE, Settings))
#define SETTINGS_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SETTINGS_TYPE, SettingsClass))
#define IS_SETTINGS(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SETTINGS_TYPE))
#define IS_SETTINGS_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SETTINGS_TYPE))
#define SETTINGS_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SETTINGS_TYPE, SettingsClass))

typedef struct _Settings Settings;
typedef struct _SettingsClass  SettingsClass;

// Don't forget to modify clone() if anything is added to this object
struct _Settings 
  {
  GObject base;
  gboolean dispose_has_run;
  int user_random_seed;
  int user_screen_height;
  int user_screen_width;
  int user_tandy_bit;
  int user_interpreter_number;
  char *font_name_main;
  char *font_name_fixed;
  int font_size;
  char *filename;
  int speed;
  int graphics_width;
  int graphics_height;
  RGB8COLOUR foreground_rgb8;
  RGB8COLOUR background_rgb8;
  int top_margin;
  int bottom_margin;
  int left_margin;
  int right_margin;
  int error_level; // 0-3
  gboolean ignore_game_colours;
  };

struct _SettingsClass
  {
  GObjectClass parent_class;
  void (* settings) (Settings *it);
  };

GType settings_get_type (void);

Settings *settings_new (const char *filename);

void settings_set_font_name (Settings *self, const char *name);
void settings_set_font_name_fixed (Settings *self, const char *name);
void settings_set_font_name_main (Settings *self, const char *name);
void settings_save (const Settings *self);
void settings_load (Settings *self);
Settings *settings_clone (const Settings *other);
void settings_copy (Settings *self, const Settings *other);

G_END_DECLS




