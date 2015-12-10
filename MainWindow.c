#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <stdarg.h>
#include <ctype.h>
#include <time.h>
#include <string.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <errno.h>
#include <gtk/gtk.h>
#include <gdk/gdkkeysyms.h>
#include <string.h>
#include "MainWindow.h"
#include "Settings.h"
#include "SettingsDialog.h"
#include "StoryReader.h"
#include "StoryTerminal.h"
#include "Interpreter.h"
#include "colourutils.h"
#include "dialogs.h"

G_DEFINE_TYPE (MainWindow, mainwindow, GTK_TYPE_WINDOW);

static char *mainwindow_about_message = "\n" APPNAME 
    ": a graphical z-code interpreter for GTK.\nVersion " VERSION "\n\n"
    "(c)2011-2012 Kevin Boone and others\n";


void mainwindow_setup_ui (MainWindow *self);
void mainwindow_set_size_according_to_terminal (MainWindow *self);

// Ugly frig -- this int only makes sense to frotz, which this class is
// not supposed to know about. But the thought of implementing a half-dozen
// new virtual methods just to set this trivial variable was more than
// I could face
extern int err_report_mode;

/*======================================================================
  mainwindow_request_quit
======================================================================*/
static gboolean mainwindow_request_quit (MainWindow *self)
{

  if (self->story_reader)
    {
    // We _can't_ shut down elegantly -- this method is probably
    //  being called from deep within the interpreter somewhere,
    //  and cleaning up the UI would just cause it to crash
    // TODO -- prompt
    exit (0);
    }
  else
    gtk_main_quit();
  return FALSE; 
}


/*======================================================================
  mainwindow_delete_event_callback
======================================================================*/
static gboolean mainwindow_delete_event_callback (GtkWidget *w, 
  GdkEvent *e, gpointer user_data)
{
  return mainwindow_request_quit (MAINWINDOW(user_data));
} 


/*======================================================================
  mainwindow_quit_event_callback
======================================================================*/
static gboolean mainwindow_quit_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  return mainwindow_request_quit (MAINWINDOW(user_data));
} 


/*======================================================================
  mainwindow_kill_line_event_callback
======================================================================*/
static gboolean mainwindow_kill_line_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->terminal)
  {
    storyterminal_kill_line (self->terminal);
  }
  return FALSE;
}


/*======================================================================
  mainwindow_next_word_event_callback
======================================================================*/
static gboolean mainwindow_next_word_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->terminal)
  {
    storyterminal_go_to_next_word (self->terminal);
  }
  return FALSE;
}


/*======================================================================
  mainwindow_prev_word_event_callback
======================================================================*/
static gboolean mainwindow_prev_word_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->terminal)
  {
    storyterminal_go_to_prev_word (self->terminal);
  }
  return FALSE;
}


/*======================================================================
  mainwindow_kill_line_from_cursor_event_callback
======================================================================*/
static gboolean mainwindow_kill_line_from_cursor_event_callback 
  (GtkMenuItem *w, gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->terminal)
  {
    storyterminal_kill_line_from_cursor (self->terminal);
  }
  return FALSE;
}


/*======================================================================
  mainwindow_paste_quit_event_callback
======================================================================*/
static gboolean mainwindow_paste_quit_event_callback 
  (GtkMenuItem *w, gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->terminal)
  {
    storyterminal_paste_quit (self->terminal);
  }
  return FALSE;
}

/*======================================================================
  mainwindow_terminal_size_allocate_event
=====================================================================*/
static void mainwindow_terminal_size_allocate_event (GtkWidget *w, 
    GdkRectangle *a, gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  int fx, fy, cx, cy;
  storyterminal_get_char_cell_size_in_pixels (self->terminal, &fx, &fy);
  storyterminal_get_widget_size (self->terminal, &cx, &cy);
  int rows = cy / fy;
  int cols = cx / fx;
  g_debug 
    ("Main window notified that terminal size is now r=%d c=%d cx=%d cy=%d", 
     rows, cols, cx, cy);

  if (cols != self->settings->user_screen_width || 
       rows != self->settings->user_screen_height)
    {
    self->settings->user_screen_width = cols;
    self->settings->user_screen_height = rows;
    settings_save (self->settings);
    }
}


/*======================================================================
  mainwindow_paste_event_callback
======================================================================*/
static gboolean mainwindow_paste_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->terminal)
    {
    GtkClipboard *clipboard = gtk_clipboard_get (GDK_SELECTION_CLIPBOARD);
    char *s = gtk_clipboard_wait_for_text (clipboard);
    if (s)
      {
      storyterminal_add_to_input_buffer_utf8 (self->terminal, s);
      free (s);
      }
    }
  return FALSE;
} 


/*======================================================================
  mainwindow_prefs_event_callback
======================================================================*/
static gboolean mainwindow_prefs_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  SettingsDialog *d = SETTINGSDIALOG 
    (settingsdialog_new (self->settings));

  int ret = gtk_dialog_run (GTK_DIALOG (d));
  if (ret == GTK_RESPONSE_ACCEPT || ret == GTK_RESPONSE_OK || ret
    == GTK_RESPONSE_APPLY)
  {
    const Settings *new_settings = settingsdialog_get_new_settings (d);

    err_report_mode = new_settings->error_level;

    if (self->settings->user_screen_width != 
            new_settings->user_screen_width
       || self->settings->user_screen_height != 
            new_settings->user_screen_height)
    {
      if (self->terminal)
        {
        storyterminal_set_size (self->terminal, 
          new_settings->user_screen_height, new_settings->user_screen_width);
        mainwindow_set_size_according_to_terminal (self);
        }
    }
  
    if (strcmp (self->settings->font_name_fixed, new_settings->font_name_fixed))
    {
      if (self->terminal)
        {
        storyterminal_set_font_name_fixed (self->terminal, 
          new_settings->font_name_fixed);
        }
    }

    if (strcmp (self->settings->font_name_main, new_settings->font_name_main))
    {
      if (self->terminal)
        {
        storyterminal_set_font_name_main (self->terminal, 
          new_settings->font_name_main);
        }
    }
  
    if (self->settings->font_size != new_settings->font_size)
    {
      if (self->terminal)
        {
        storyterminal_set_font_size (self->terminal, 
          new_settings->font_size);
        mainwindow_set_size_according_to_terminal (self);
        }
    }

    if (self->settings->foreground_rgb8 != new_settings->foreground_rgb8)
    {
      if (self->terminal)
        {
        storyterminal_set_default_fg_colour (self->terminal, 
          new_settings->foreground_rgb8);
        }
    }
 
    if (self->settings->background_rgb8 != new_settings->background_rgb8)
    {
      if (self->terminal)
        {
        storyterminal_set_default_bg_colour (self->terminal, 
          new_settings->background_rgb8);
        GdkColor gdkc;
        colourutils_rgb8_to_gdk (self->settings->background_rgb8, 
           &gdkc);
        gtk_widget_modify_bg (GTK_WIDGET(self->terminal), 
          GTK_STATE_NORMAL, &gdkc);
        }
      GdkColor gdkc;
      colourutils_rgb8_to_gdk (new_settings->background_rgb8, &gdkc);
      gtk_widget_modify_bg (GTK_WIDGET(self), GTK_STATE_NORMAL, &gdkc);
    }
 
    if (self->settings->foreground_rgb8 != new_settings->foreground_rgb8)
    {
      if (self->terminal)
        {
        storyterminal_set_default_fg_colour (self->terminal, 
          new_settings->foreground_rgb8);
        }
    }

    if (self->settings->ignore_game_colours != 
      new_settings->ignore_game_colours)
    {
    // We can't really change this dynamically
    }

    settings_copy (self->settings, new_settings);
    settings_save (self->settings);

  }
  gtk_widget_destroy (GTK_WIDGET (d));
  return TRUE;
} 


/*======================================================================
  mainwindow_open_event_callback
======================================================================*/
static gboolean mainwindow_open_event_callback (GtkMenuItem *w, 
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  GtkFileChooserDialog *d = GTK_FILE_CHOOSER_DIALOG 
    (gtk_file_chooser_dialog_new ("Open File",
    GTK_WINDOW (self),
    GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL));

  GtkFileFilter *fz = gtk_file_filter_new ();
  gtk_file_filter_set_name (fz, "Z-code files (*.z*)");
  gtk_file_filter_add_pattern (fz, "*.z*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (d), fz);
  GtkFileFilter *fall = gtk_file_filter_new ();
  gtk_file_filter_set_name (fall, "All files");
  gtk_file_filter_add_pattern (fall, "*");
  gtk_file_chooser_add_filter (GTK_FILE_CHOOSER (d), fall);

  if (self->current_story_dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (d), 
      self->current_story_dir);

  if (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_ACCEPT)
  {
    char *filename;
    filename = gtk_file_chooser_get_filename (GTK_FILE_CHOOSER (d));
    if (self->current_story_dir)
    {
      free (self->current_story_dir);
      self->current_story_dir = NULL;
    }
    self->current_story_dir = gtk_file_chooser_get_current_folder 
      (GTK_FILE_CHOOSER (d));

    gtk_widget_destroy (GTK_WIDGET(d));
    mainwindow_open_file (self, filename);

    free (filename);
  }
 else
  {
  gtk_widget_destroy (GTK_WIDGET(d));
  }

  return FALSE; 
} 

/*======================================================================
  mainwindow_about_event
======================================================================*/
void mainwindow_about_event_callback (GtkMenuItem *mi,
  gpointer data)
{
  MainWindow *w = (MainWindow *)data;
  GtkDialog *ed = GTK_DIALOG (gtk_message_dialog_new (GTK_WINDOW (w), 
        GTK_DIALOG_MODAL, GTK_MESSAGE_OTHER, GTK_BUTTONS_OK, 
        mainwindow_about_message));
  gtk_window_set_title (GTK_WINDOW (ed), APPNAME); 
  gtk_dialog_run (ed);
  gtk_widget_destroy (GTK_WIDGET (ed));
}


/*======================================================================
  mainwindow_about_story_event
======================================================================*/
void mainwindow_about_story_event_callback (GtkMenuItem *mi,
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->story_reader)
  {
    storyreader_show_dialog (self->story_reader, GTK_WINDOW (self));
  }
}


/*======================================================================
  mainwindow_transcript_event
======================================================================*/
void mainwindow_transcript_event_callback (GtkMenuItem *mi,
  gpointer user_data)
{
  MainWindow *self = (MainWindow *)user_data;
  if (self->interpreter)
  {
  const GString *s = interpreter_get_transcript (self->interpreter);
  dialogs_show_text (GTK_WINDOW (self), s->str);
  }
}


/*======================================================================
  mainwindow_create_menu
======================================================================*/
static GtkMenuBar* mainwindow_create_menu (MainWindow *self,
  GtkAccelGroup *accel_group)
{
  GtkMenuBar *menuBar = GTK_MENU_BAR (gtk_menu_bar_new ());

  // File menu
  GtkMenuItem *fileMenu = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_File"));
  GtkMenu *fileMenuMenu = GTK_MENU (gtk_menu_new ());
  self->open_menu_item = GTK_MENU_ITEM 
    (gtk_image_menu_item_new_from_stock (GTK_STOCK_OPEN, accel_group));
  g_signal_connect (G_OBJECT(self->open_menu_item), "activate",
     G_CALLBACK (mainwindow_open_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (fileMenuMenu), 
    GTK_WIDGET (self->open_menu_item));
  GtkMenuItem *sep = GTK_MENU_ITEM 
    (gtk_separator_menu_item_new ());
  gtk_menu_shell_append (GTK_MENU_SHELL (fileMenuMenu), 
    GTK_WIDGET (sep));
  GtkMenuItem *prefsMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Preferences..."));
  g_signal_connect (G_OBJECT(prefsMenuItem), "activate",
     G_CALLBACK (mainwindow_prefs_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (fileMenuMenu), 
    GTK_WIDGET (prefsMenuItem));
  sep = GTK_MENU_ITEM 
    (gtk_separator_menu_item_new ());
  gtk_menu_shell_append (GTK_MENU_SHELL (fileMenuMenu), 
    GTK_WIDGET (sep));
  GtkMenuItem *quitMenuItem = GTK_MENU_ITEM 
    (gtk_image_menu_item_new_from_stock (GTK_STOCK_QUIT, accel_group));
  g_signal_connect (G_OBJECT(quitMenuItem), "activate",
     G_CALLBACK (mainwindow_quit_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (fileMenuMenu), 
    GTK_WIDGET (quitMenuItem));
  gtk_menu_item_set_submenu (fileMenu, GTK_WIDGET (fileMenuMenu));
  gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), GTK_WIDGET (fileMenu));

  // Edit menu
  self->edit_menu = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Edit"));
  GtkMenu *editMenuMenu = GTK_MENU (gtk_menu_new ());

  GtkMenuItem *pasteMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Paste"));
  g_signal_connect (G_OBJECT(pasteMenuItem), "activate",
     G_CALLBACK (mainwindow_paste_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (editMenuMenu), 
    GTK_WIDGET (pasteMenuItem));

  GtkMenuItem *killLineMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Erase line"));
  g_signal_connect (G_OBJECT(killLineMenuItem), "activate",
     G_CALLBACK (mainwindow_kill_line_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (editMenuMenu), 
    GTK_WIDGET (killLineMenuItem));

  GtkMenuItem *nextWordMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Next word"));
  g_signal_connect (G_OBJECT(nextWordMenuItem), "activate",
     G_CALLBACK (mainwindow_next_word_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (editMenuMenu), 
    GTK_WIDGET (nextWordMenuItem));

  GtkMenuItem *prevWordMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("P_revious word"));
  g_signal_connect (G_OBJECT(prevWordMenuItem), "activate",
     G_CALLBACK (mainwindow_prev_word_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (editMenuMenu), 
    GTK_WIDGET (prevWordMenuItem));

  GtkMenuItem *killLineFromCursorMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("Er_ase line from cursor"));
  g_signal_connect (G_OBJECT(killLineFromCursorMenuItem), "activate",
     G_CALLBACK (mainwindow_kill_line_from_cursor_event_callback), 
       self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (editMenuMenu), 
    GTK_WIDGET (killLineFromCursorMenuItem));
  GtkMenuItem *pasteQuitMenuItem = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Quit"));
  g_signal_connect (G_OBJECT(pasteQuitMenuItem), "activate",
     G_CALLBACK (mainwindow_paste_quit_event_callback), 
       self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (editMenuMenu), 
    GTK_WIDGET (pasteQuitMenuItem));
  gtk_menu_item_set_submenu (self->edit_menu, GTK_WIDGET (editMenuMenu));
  gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), 
    GTK_WIDGET (self->edit_menu));

  gtk_widget_add_accelerator(GTK_WIDGET(pasteMenuItem), "activate", 
      accel_group, GDK_v, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
  gtk_widget_add_accelerator(GTK_WIDGET(killLineMenuItem), "activate", 
      accel_group, GDK_c, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
  gtk_widget_add_accelerator(GTK_WIDGET(killLineFromCursorMenuItem), 
      "activate", accel_group, GDK_k, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
  gtk_widget_add_accelerator(GTK_WIDGET(pasteQuitMenuItem), 
      "activate", accel_group, GDK_d, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
  gtk_widget_add_accelerator(GTK_WIDGET(nextWordMenuItem), 
      "activate", accel_group, GDK_Right, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 
  gtk_widget_add_accelerator(GTK_WIDGET(prevWordMenuItem), 
      "activate", accel_group, GDK_Left, GDK_CONTROL_MASK, GTK_ACCEL_VISIBLE); 


  // View menu
  GtkMenuItem *viewMenu = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_View"));
  GtkMenu *viewMenuMenu = GTK_MENU (gtk_menu_new ());

  self->transcript_item = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Transcript"));
  g_signal_connect (G_OBJECT(self->transcript_item), "activate",
     G_CALLBACK (mainwindow_transcript_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (viewMenuMenu), 
    GTK_WIDGET (self->transcript_item));
  gtk_widget_set_sensitive (GTK_WIDGET (self->transcript_item), FALSE);

  gtk_menu_item_set_submenu (viewMenu, GTK_WIDGET (viewMenuMenu));
  gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), GTK_WIDGET (viewMenu));

  // Help menu
  GtkMenuItem *helpMenu = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("_Help"));
  GtkMenu *helpMenuMenu = GTK_MENU (gtk_menu_new ());

  GtkMenuItem *aboutMenuItem = GTK_MENU_ITEM 
    (gtk_image_menu_item_new_from_stock (GTK_STOCK_ABOUT, accel_group));
  g_signal_connect (G_OBJECT(aboutMenuItem), "activate",
     G_CALLBACK (mainwindow_about_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (helpMenuMenu), 
    GTK_WIDGET (aboutMenuItem));

  self->about_story_item = GTK_MENU_ITEM 
    (gtk_menu_item_new_with_mnemonic ("About this _story..."));
  g_signal_connect (G_OBJECT(self->about_story_item), "activate",
     G_CALLBACK (mainwindow_about_story_event_callback), self);       
  gtk_menu_shell_append (GTK_MENU_SHELL (helpMenuMenu), 
    GTK_WIDGET (self->about_story_item));
  gtk_widget_set_sensitive (GTK_WIDGET (self->about_story_item), FALSE);

  gtk_menu_item_set_submenu (helpMenu, GTK_WIDGET (helpMenuMenu));
  gtk_menu_shell_append (GTK_MENU_SHELL (menuBar), GTK_WIDGET (helpMenu));

  gtk_widget_show_all (GTK_WIDGET (menuBar));
  return menuBar;
}


/*======================================================================
  mainwindow_setup_ui
  Sets up menus and titles according to play state
======================================================================*/
void mainwindow_setup_ui (MainWindow *self)
{
  GString *title = g_string_new ("grotz");
  if (self->story_reader)
  {
    gtk_widget_set_sensitive (GTK_WIDGET (self->transcript_item), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->open_menu_item), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->about_story_item), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->edit_menu), TRUE);
    title = g_string_append (title, ": ");
    title = g_string_append (title, storyreader_get_title (self->story_reader));
  }
  else
  {
    gtk_widget_set_sensitive (GTK_WIDGET (self->transcript_item), FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->open_menu_item), TRUE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->about_story_item),FALSE);
    gtk_widget_set_sensitive (GTK_WIDGET (self->edit_menu), FALSE);
  }
  gtk_window_set_title (GTK_WINDOW (self), title->str);
  g_string_free (title, TRUE);
}


/*======================================================================
  mainwindow_setup_layout
  Sets up packing, margins, etc, according to what is in self->settings
======================================================================*/
void mainwindow_setup_layout (MainWindow *self)
  {
  gtk_alignment_set_padding (GTK_ALIGNMENT(self->terminal_container), 
    self->settings->top_margin, self->settings->bottom_margin,
    self->settings->left_margin, self->settings->right_margin);
  self->terminal_container = GTK_CONTAINER (self->terminal_container);
  }


/*======================================================================
  mainwindow_interpreter_state_change_callback
======================================================================*/
void mainwindow_interpeter_state_change_callback 
    (InterpreterStateChange state, void *user_data)
  {
  g_debug ("interpreter state change notification: %d", (int) state);
  MainWindow *self = MAINWINDOW (user_data);
  switch (state)
    {
    case ISC_WAIT_FOR_INPUT:
      gtk_widget_set_sensitive (GTK_WIDGET (self->edit_menu), TRUE);
      break;
    case ISC_INPUT_COMPLETED:
      gtk_widget_set_sensitive (GTK_WIDGET (self->edit_menu), FALSE);
      break;
    }

  }


/*======================================================================
  mainwindow_set_size_according_to_terminal
  Find out the size the terminal would like to be, then size 
  this window to accomodate that size plus the margins
======================================================================*/
void mainwindow_set_size_according_to_terminal (MainWindow *self)
  {
  int stw, sth, r, c;
  storyterminal_get_widget_size (self->terminal, &stw, &sth);
  int h_margin = self->settings->left_margin + self->settings->right_margin;
  int v_margin = self->settings->top_margin + self->settings->bottom_margin;
  storyterminal_get_unit_size (self->terminal, &r, &c);
  g_debug ("Setting main window size to match terminal size of "
            "r=%d c=%d cx=%d cy=%d hmargin=%d vmargin=%d", 
            r, c, stw, sth, h_margin, v_margin);

  int mh = (GTK_WIDGET(self->menu_bar))->allocation.height;

  gtk_window_resize (GTK_WINDOW(self), stw + h_margin, 
    sth + v_margin + mh);
  }


/*======================================================================
  mainwindow_child_finished
======================================================================*/
void mainwindow_child_finished (MainWindow *self)
  {
  if (self->interpreter)
    {
    interpreter_child_finished (self->interpreter);
    }
  }


/*======================================================================
  mainwindow_get_ignore_game_colours
======================================================================*/
gboolean mainwindow_get_ignore_game_colours (const MainWindow *self)
  {
  return self->settings->ignore_game_colours;
  }

/*======================================================================
  mainwindow_set_gdk_background_colour
  Sets the gdk background colour of the main widget. This does not affect
  the StoryTerminal background in any way
======================================================================*/
void mainwindow_set_gdk_background_colour (MainWindow *self, RGB8COLOUR bg)
  {
  GdkColor gdkc;
  colourutils_rgb8_to_gdk (bg, &gdkc);
  gtk_widget_modify_bg (GTK_WIDGET(self), GTK_STATE_NORMAL, &gdkc); 
  gtk_widget_queue_draw (GTK_WIDGET(self));
  }


/*======================================================================
  mainwindow_open_file
  Note that this method opens the file and runs the interpreter
  It does not return until interpretation is complete. All
  UI operations are carried out by peeking the GTK event loop from
  within the interpreter
======================================================================*/
void mainwindow_open_file (MainWindow *self, const char *filename)
{
  GError *error = NULL;
  self->story_reader = storyreader_new (self);
  self->interpreter = storyreader_open (self->story_reader, 
    filename, self->temp_dir, self->settings, &error);

  if (!error)
  {
    self->terminal = interpreter_get_terminal (self->interpreter);

    storyterminal_set_font_name_fixed (self->terminal, 
          self->settings->font_name_fixed);
    storyterminal_set_font_name_main (self->terminal, 
          self->settings->font_name_main);
    storyterminal_set_font_size (self->terminal, 
          self->settings->font_size);

    storyterminal_set_size (self->terminal, 
      self->settings->user_screen_height,
      self->settings->user_screen_width);

    mainwindow_set_size_according_to_terminal (self);

    g_signal_connect (G_OBJECT (self->terminal), "size_allocate",
       G_CALLBACK (mainwindow_terminal_size_allocate_event), self);

    // TODO -- get default colours from settings
    //  Note that we have to match _this_ window to the terminal
    //  background colour, unless we want a border around the 
    //  terminal of different colour
    storyterminal_set_default_bg_colour 
      (self->terminal, self->settings->background_rgb8);
    GdkColor gdkc;
    colourutils_rgb8_to_gdk (self->settings->background_rgb8, 
       &gdkc);
    gtk_widget_modify_bg (GTK_WIDGET(self), GTK_STATE_NORMAL, &gdkc);
    gtk_widget_modify_bg (GTK_WIDGET(self->terminal), GTK_STATE_NORMAL, &gdkc);
    storyterminal_set_default_fg_colour 
      (self->terminal, self->settings->foreground_rgb8);

    GtkWidget *old_term = gtk_bin_get_child 
      (GTK_BIN(self->terminal_container));
    if (old_term) gtk_widget_destroy (old_term);

    gtk_container_add (GTK_CONTAINER (self->terminal_container), 
      GTK_WIDGET (self->terminal));
    gtk_widget_show_all (GTK_WIDGET (self->terminal_container));
    gtk_widget_grab_focus (GTK_WIDGET (self->terminal));

    mainwindow_setup_ui (self);

    interpreter_set_state_change_callback 
      (self->interpreter, mainwindow_interpeter_state_change_callback, self);
    interpreter_run (self->interpreter);

    g_object_unref (self->interpreter);
    storyreader_close (self->story_reader);
    gtk_container_add (GTK_CONTAINER (self->terminal_container), 
      GTK_WIDGET (gtk_label_new(mainwindow_about_message)));
    gtk_widget_show_all (GTK_WIDGET (self->terminal_container));
  }
  else
  {
    GtkDialog *d = GTK_DIALOG (gtk_message_dialog_new (GTK_WINDOW(self),
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "Can't open story '%s': %s",
      filename, error->message));
      gtk_dialog_run (d);
      gtk_widget_destroy (GTK_WIDGET (d));
      g_error_free (error);
      return;
  }

  g_object_unref (self->story_reader);
  self->story_reader = NULL;
  self->interpreter = NULL;
  self->terminal = NULL;
  mainwindow_setup_ui (self);
}


/*======================================================================
  mainwindow_init
======================================================================*/
static void mainwindow_init (MainWindow *self)
{
  g_debug ("Initialising main window");
  self->dispose_has_run = FALSE;
  //self->interpreter = NULL;

  // Init GUI elements
  gtk_widget_set_name (GTK_WIDGET(self), "mainwindow");
  GtkVBox *main_box = GTK_VBOX (gtk_vbox_new (FALSE, 0)); 
  //self->main_box = main_box;
  GtkAccelGroup *accel_group = gtk_accel_group_new();
  gtk_window_add_accel_group (GTK_WINDOW(self), accel_group);
  //self->accel_group = accel_group;
  gtk_widget_show (GTK_WIDGET (main_box));
  gtk_container_add (GTK_CONTAINER (self), GTK_WIDGET (main_box));
  gtk_window_set_title (GTK_WINDOW (self), APPNAME);
  g_signal_connect (G_OBJECT(self), "delete-event",
     G_CALLBACK (mainwindow_delete_event_callback), self);       

  self->menu_bar = mainwindow_create_menu (self, accel_group);

  gtk_box_pack_start (GTK_BOX (main_box), GTK_WIDGET (self->menu_bar),
    FALSE, FALSE, 0);

  GtkAlignment *vs_align = GTK_ALIGNMENT (gtk_alignment_new 
   (0.5, 0.5, 1.0, 1.0));
  self->terminal_container = GTK_CONTAINER (vs_align);

  gtk_widget_show (GTK_WIDGET (vs_align));
  gtk_box_pack_start (GTK_BOX (main_box), GTK_WIDGET (vs_align),
    TRUE, TRUE, 0);
  
  // We have to start with some size, or the UI will be unusable
  //  But we can't just do gtk_widget_set_size_request 
  //  (GTK_WIDGET(self), 500, 300); as this will prevent the screen
  //  being resized to fit the terminal window. So create a dummy 
  //  widget with some message in, which will later be removed

  gtk_container_add (self->terminal_container, gtk_label_new 
    (mainwindow_about_message));
  gtk_widget_show_all (GTK_WIDGET(self->terminal_container));

  mainwindow_setup_ui (self);

//  gtk_window_set_policy (GTK_WINDOW(self), TRUE, FALSE, TRUE);

#ifndef WIN32
   GString *icon = g_string_new (PIXMAPDIR);
   icon = g_string_append (icon, "/");
   icon = g_string_append (icon, "grotz.png");

   GError *error = NULL;
   GdkPixbuf *pb = gdk_pixbuf_new_from_file (icon->str, &error);
   if (pb)
     gtk_window_set_icon (GTK_WINDOW(self), pb);
   if (error) 
     g_error_free (error);
   
   g_string_free (icon, TRUE);
#endif

}


/*======================================================================
  mainwindow_dispose
======================================================================*/
static void mainwindow_dispose (GObject *_self)
{
  MainWindow *self = (MainWindow *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing main window");
  self->dispose_has_run = TRUE;
  if (self->settings)
  {
    g_object_unref (self->settings);
    self->settings = 0;
  }
  if (self->current_story_dir)
  {
    free (self->current_story_dir);
    self->current_story_dir = NULL;
  }
  if (self->temp_dir)
  {
    free (self->temp_dir);
    self->temp_dir = NULL;
  }
  G_OBJECT_CLASS (mainwindow_parent_class)->dispose (_self);
}
/*======================================================================
  mainwindow_class_init
======================================================================*/

static void mainwindow_class_init
  (MainWindowClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = mainwindow_dispose;
}


/*======================================================================
  mainwindow_new
======================================================================*/
MainWindow *mainwindow_new (Settings *settings, const char *temp_dir)
{
  MainWindow *self = g_object_new 
   (MAINWINDOW_TYPE, NULL);
  self->settings = g_object_ref (settings);
  self->temp_dir = strdup (temp_dir);
  mainwindow_setup_layout (self);
  err_report_mode = settings->error_level;
  return self;
}


