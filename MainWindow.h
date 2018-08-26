#pragma once

#include <gtk/gtk.h>
#include "colourutils.h" 

G_BEGIN_DECLS


#define MAINWINDOW_TYPE            (mainwindow_get_type ())
#define MAINWINDOW(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), MAINWINDOW_TYPE, MainWindow))
#define MAINWINDOW_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), MAINWINDOW_TYPE, MainWindowClass))
#define IS_MAINWINDOW(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), MAINWINDOW_TYPE))
#define IS_MAINWINDOW_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), MAINWINDOW_TYPE))
#define MAINWINDOW_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), MAINWINDOW_TYPE, MainWindowClass))


typedef struct _MainWindow MainWindow;
typedef struct _MainWindowClass  MainWindowClass;
struct _Interpreter;

struct _MainWindow 
  {
  GtkWindow base;
  gboolean dispose_has_run;
  struct _Settings *settings;
  GtkMenuItem *open_menu_item;
  GtkMenuItem *about_story_item;
  GtkMenuItem *transcript_item;
  GtkMenuItem *edit_menu;
  char *temp_dir;
  char *current_story_dir;
  struct _StoryReader *story_reader;
  GtkContainer *terminal_container;
  struct _Interpreter *interpreter;
  struct _StoryTerminal *terminal;
  GtkMenuBar *menu_bar;
  };

struct _MainWindowClass
  {
  GtkWindowClass parent_class;
  void (* mainwindow) (MainWindow *it);
  };

GType mainwindow_get_type (void);

MainWindow *mainwindow_new (struct _Settings *settings, const char *temp_dir);
void mainwindow_open_file (MainWindow *self, const char *filename);
void mainwindow_child_finished (MainWindow *self); // Called from SIGCHLD
void mainwindow_set_gdk_background_colour (MainWindow *self, RGB8COLOUR bg);
gboolean mainwindow_get_ignore_game_colours (const MainWindow *self);

G_END_DECLS




