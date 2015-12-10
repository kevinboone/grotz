#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define SETTINGSDIALOG_TYPE            (settingsdialog_get_type ())
#define SETTINGSDIALOG(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), SETTINGSDIALOG_TYPE, SettingsDialog))
#define SETTINGSDIALOG_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), SETTINGSDIALOG_TYPE, SettingsDialogClass))
#define IS_SETTINGSDIALOG(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), SETTINGSDIALOG_TYPE))
#define IS_SETTINGSDIALOG_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), SETTINGSDIALOG_TYPE))
#define SETTINGSDIALOG_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), SETTINGSDIALOG_TYPE, SettingsDialogClass))

typedef struct _SettingsDialog SettingsDialog;
typedef struct _SettingsDialogClass  SettingsDialogClass;

struct _SettingsDialog 
  {
  GtkDialog base;
  //Private:
  gboolean dispose_has_run;
  struct _Settings *newsettings;
  GtkSpinButton *rows_spin;
  GtkSpinButton *cols_spin;
  GtkSpinButton *font_size_spin;
  GtkLabel *font_label;
  struct _Settings *new_settings;
  struct _KBComboBoxText *fixed_font_combo;
  struct _KBComboBoxText *main_font_combo;
  GtkColorButton *foreground_colour_button;
  GtkColorButton *background_colour_button;
  struct _KBComboBoxText *interpreter_combo;
  struct _KBComboBoxText *error_level_combo;
  GtkCheckButton *ignore_game_colours_check;
  };

struct _SettingsDialogClass
  {
  GtkDialogClass parent_class;
  void (* settingsdialog) (SettingsDialog *it);
  };

GType settingsdialog_get_type (void);

SettingsDialog *settingsdialog_new (const struct _Settings *settings);
const Settings *settingsdialog_get_new_settings (SettingsDialog *self);

G_END_DECLS




