#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "Settings.h"
#include "SettingsDialog.h"
#include "kbcomboboxtext.h"
#include "colourutils.h"

G_DEFINE_TYPE (SettingsDialog, settingsdialog, GTK_TYPE_DIALOG);


/*======================================================================
  settingsdialog_response_event
======================================================================*/
static void settingsdialog_response_event (GtkDialog *d, 
  int id, gpointer user_data)
{
  SettingsDialog *self = (SettingsDialog *)d;
  if (id == GTK_RESPONSE_OK)
    {
    self->new_settings->user_screen_width = 
      gtk_spin_button_get_value_as_int (self->cols_spin);
    self->new_settings->user_screen_height = 
      gtk_spin_button_get_value_as_int (self->rows_spin);
    settings_set_font_name_main (self->new_settings, 
      kb_combo_box_text_get_active_text (self->main_font_combo));
    settings_set_font_name_fixed (self->new_settings, 
      kb_combo_box_text_get_active_text (self->fixed_font_combo));
    self->new_settings->font_size = 
      gtk_spin_button_get_value_as_int (self->font_size_spin);
    GdkColor gdkc;
    gtk_color_button_get_color (self->foreground_colour_button, &gdkc);
    self->new_settings->foreground_rgb8 = 
      colourutils_gdk_to_rgb8 (&gdkc);
    gtk_color_button_get_color (self->background_colour_button, &gdkc);
    self->new_settings->background_rgb8 = 
      colourutils_gdk_to_rgb8 (&gdkc);
    self->new_settings->user_interpreter_number =  
      gtk_combo_box_get_active (GTK_COMBO_BOX (self->interpreter_combo));
    self->new_settings->error_level =  
      gtk_combo_box_get_active (GTK_COMBO_BOX (self->error_level_combo));
    self->new_settings->ignore_game_colours = 
      gtk_toggle_button_get_active 
        (GTK_TOGGLE_BUTTON (self->ignore_game_colours_check));
    } 
}


/*======================================================================
  settingsdialog_init
=====================================================================*/

static void settingsdialog_init (SettingsDialog *self)
{
  g_debug ("Initialising settingsdialog\n");
  self->dispose_has_run = FALSE;
  self->newsettings = NULL;
  gtk_dialog_add_buttons (GTK_DIALOG (self), 
       GTK_STOCK_OK, GTK_RESPONSE_OK, GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL, 
       NULL);

  GtkTable *t = GTK_TABLE (gtk_table_new (11, 2, TRUE));
  gtk_table_set_row_spacings (t, 5); 
  gtk_table_set_col_spacings (t, 5); 

  self->rows_spin = GTK_SPIN_BUTTON 
    (gtk_spin_button_new_with_range (20.0, 100.0, 1.0));
  GtkLabel *l = GTK_LABEL (gtk_label_new_with_mnemonic ("_Rows"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->rows_spin));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 0, 1);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->rows_spin), 1, 2, 0, 1);
  self->cols_spin = GTK_SPIN_BUTTON 
    (gtk_spin_button_new_with_range (70.0, 200.0, 1.0));
  l = GTK_LABEL (gtk_label_new_with_mnemonic ("C_ols"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->cols_spin));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 1, 2);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->cols_spin), 1, 2, 1, 2);

  l = GTK_LABEL (gtk_label_new_with_mnemonic ("_Fixed font"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  self->fixed_font_combo = KB_COMBO_BOX_TEXT 
     (kb_combo_box_text_new ());
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->fixed_font_combo));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 2, 3);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->fixed_font_combo), 
    1, 2, 2, 3);

  l = GTK_LABEL (gtk_label_new_with_mnemonic ("_Main font"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  self->main_font_combo = KB_COMBO_BOX_TEXT 
     (kb_combo_box_text_new ());
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->main_font_combo));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 3, 4);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->main_font_combo), 
    1, 2, 3, 4);

  self->font_size_spin = GTK_SPIN_BUTTON 
    (gtk_spin_button_new_with_range (6.0, 16.0, 1.0));
  l = GTK_LABEL (gtk_label_new_with_mnemonic ("Fo_nt size"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->font_size_spin));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 4, 5);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->font_size_spin), 1, 2, 4, 5);

  self->foreground_colour_button = GTK_COLOR_BUTTON 
    (gtk_color_button_new ());
  l = GTK_LABEL (gtk_label_new_with_mnemonic ("For_eground colour"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET 
    (self->foreground_colour_button));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 5, 6);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->foreground_colour_button), 
    1, 2, 5, 6);
  
  self->background_colour_button = GTK_COLOR_BUTTON 
    (gtk_color_button_new ());
  l = GTK_LABEL (gtk_label_new_with_mnemonic ("_Background colour"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET 
    (self->background_colour_button));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 6, 7);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->background_colour_button), 
    1, 2, 6, 7);
  
  self->ignore_game_colours_check = GTK_CHECK_BUTTON 
     (gtk_check_button_new_with_mnemonic ("I_gnore game colours"));
  gtk_table_attach_defaults (t, GTK_WIDGET (self->ignore_game_colours_check), 
    1, 2, 7, 8);

  l = GTK_LABEL (gtk_label_new_with_mnemonic ("_Interpreter emualtion"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  self->interpreter_combo = KB_COMBO_BOX_TEXT 
     (kb_combo_box_text_new ());
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->interpreter_combo));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 8, 9);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->interpreter_combo), 
    1, 2, 8, 9);

  l = GTK_LABEL (gtk_label_new_with_mnemonic ("_Error level"));
  gtk_misc_set_alignment (GTK_MISC (l), 0.95, 0.5);
  self->error_level_combo = KB_COMBO_BOX_TEXT 
     (kb_combo_box_text_new ());
  gtk_label_set_mnemonic_widget (l, GTK_WIDGET (self->error_level_combo));
  gtk_table_attach_defaults (t, GTK_WIDGET (l), 0, 1, 9, 10);
  gtk_table_attach_defaults (t, GTK_WIDGET (self->error_level_combo), 
    1, 2, 9, 10);

  gtk_box_pack_start (GTK_BOX (GTK_DIALOG (self)->vbox), 
    GTK_WIDGET (t), TRUE, TRUE, 0);

  gtk_dialog_set_default_response (GTK_DIALOG (self), GTK_RESPONSE_OK);

  g_signal_connect (G_OBJECT(self), "response",
     G_CALLBACK (settingsdialog_response_event), self);       

  gtk_container_set_border_width (GTK_CONTAINER(GTK_DIALOG(self)->vbox), 10);
  gtk_window_set_title (GTK_WINDOW (self), "grotz settings");

  gtk_widget_show_all (GTK_WIDGET (t));
}


/*======================================================================
  settingsdialog_dispose
======================================================================*/
static void settingsdialog_dispose (GObject *_this)
{
  SettingsDialog *this = (SettingsDialog *) _this;
  if (this->dispose_has_run) return;
  g_debug ("Disposing settingsdialog\n");
  this->dispose_has_run = TRUE;
  if (this->new_settings)
  {
    g_object_unref (this->new_settings);
    this->new_settings = NULL;
  }
  G_OBJECT_CLASS (settingsdialog_parent_class)->dispose (_this);
}


/*======================================================================
  settingsdialog_class_init
======================================================================*/
static void settingsdialog_class_init
  (SettingsDialogClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = settingsdialog_dispose;
}


/*======================================================================
  settingsdialog_new
======================================================================*/
SettingsDialog *settingsdialog_new (const Settings *old_settings)
{
  SettingsDialog *self = g_object_new 
   (SETTINGSDIALOG_TYPE, NULL);

  gtk_spin_button_set_value (self->rows_spin, 
    (double)old_settings->user_screen_height);
  gtk_spin_button_set_value (self->cols_spin, 
    (double)old_settings->user_screen_width);
  gtk_spin_button_set_value (self->font_size_spin, 
    (double)old_settings->font_size);

  self->new_settings = settings_clone (old_settings);

  PangoFontFamily **families;
  int i, n_families;
  PangoFontMap *fontmap = pango_cairo_font_map_get_default();
  pango_font_map_list_families (fontmap, &families, &n_families);
  int active_font_main = 0;
  int active_font_fixed = 0;

  GList *font_list = NULL;

  for (i = 0; i < n_families; i++)
    {
    PangoFontFamily *family = families[i];
    const char *family_name = pango_font_family_get_name (family);
    font_list = g_list_append (font_list, strdup (family_name));
    }

  // TODO: really this comparison should be unicode
  font_list = g_list_sort (font_list, (GCompareFunc)strcmp);
  // TODO: should we free this new list?
    
  for (i = 0; i < n_families; i++)
    {
    const char *family_name = g_list_nth_data (font_list, i);
    kb_combo_box_text_append_text (self->main_font_combo, family_name);
    kb_combo_box_text_append_text (self->fixed_font_combo, family_name);
    if (strcmp (family_name, old_settings->font_name_main) == 0)
      active_font_main = i;
    if (strcmp (family_name, old_settings->font_name_fixed) == 0)
      active_font_fixed = i;
    }

  for (i = 0; i < n_families; i++)
    {
    char *s = g_list_nth_data (font_list, i);
    free (s);
    }

  g_list_free (font_list);
    
  free (families);

  kb_combo_box_text_append_text (self->interpreter_combo, "0: default");
  kb_combo_box_text_append_text (self->interpreter_combo, "1: DEC 20");
  kb_combo_box_text_append_text (self->interpreter_combo, "2: Apple IIe");
  kb_combo_box_text_append_text (self->interpreter_combo, "3: Macintosh");
  kb_combo_box_text_append_text (self->interpreter_combo, "4: Amiga");
  kb_combo_box_text_append_text (self->interpreter_combo, "5: Atari ST");
  kb_combo_box_text_append_text (self->interpreter_combo, "6: MS-DOS");
  kb_combo_box_text_append_text (self->interpreter_combo, "7: CBM-128");
  kb_combo_box_text_append_text (self->interpreter_combo, "8: CBM-64");
  kb_combo_box_text_append_text (self->interpreter_combo, "9: Apple IIc");
  kb_combo_box_text_append_text (self->interpreter_combo, "10: Apple IIGS");
  kb_combo_box_text_append_text (self->interpreter_combo, "11: Tandy");

  kb_combo_box_text_append_text (self->error_level_combo, "0: never report");
  kb_combo_box_text_append_text (self->error_level_combo, "1: report once");
  kb_combo_box_text_append_text (self->error_level_combo, "2: always report");
  kb_combo_box_text_append_text (self->error_level_combo, "3: fatal");

  gtk_combo_box_set_active (GTK_COMBO_BOX (self->fixed_font_combo), 
    active_font_fixed);
  gtk_combo_box_set_active (GTK_COMBO_BOX (self->main_font_combo), 
    active_font_main);

  GdkColor gdkc;
  colourutils_rgb8_to_gdk (old_settings->foreground_rgb8, &gdkc);
  gtk_color_button_set_color (self->foreground_colour_button, &gdkc);
  colourutils_rgb8_to_gdk (old_settings->background_rgb8, &gdkc);
  gtk_color_button_set_color (self->background_colour_button, &gdkc);
  
  gtk_combo_box_set_active (GTK_COMBO_BOX (self->interpreter_combo),
    old_settings->user_interpreter_number);

  gtk_combo_box_set_active (GTK_COMBO_BOX (self->error_level_combo),
    old_settings->error_level);

  gtk_toggle_button_set_active  
    (GTK_TOGGLE_BUTTON (self->ignore_game_colours_check),
    old_settings->ignore_game_colours);

  return self;
}


/*======================================================================
  settingsdialog_get_new_settings
======================================================================*/
const Settings *settingsdialog_get_new_settings (SettingsDialog *self)
  {
  return self->new_settings;
  }




