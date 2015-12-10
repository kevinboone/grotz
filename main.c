#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <stdlib.h>
#include <signal.h>
#include <gtk/gtk.h>
#include <sys/stat.h>
#include "Settings.h" 
#include "MainWindow.h" 


gboolean version = FALSE; 
static MainWindow *main_window;

static GOptionEntry entries[] = 
{
  { "version", 0, 0, G_OPTION_ARG_NONE, &version, "Show version", NULL},
  { NULL }
};


void main_log_handler (const char *domain, GLogLevelFlags log_level,
    const char *message, gpointer user_data)
  {
#ifdef DEBUG
  printf ("grotz: %s\n", message);
#else
  if (log_level 
    & (G_LOG_LEVEL_CRITICAL | G_LOG_LEVEL_ERROR | G_LOG_LEVEL_WARNING))
    printf ("grotz: %s\n", message);

#endif
  }


#ifndef WIN32
// Called when a child process finishes
void handle_child_finished (int arg)
{
  mainwindow_child_finished (main_window);
  signal (SIGCHLD, handle_child_finished);
}
#endif

int main (int argc, char **argv)
{
  gtk_init (&argc, &argv);

  GError *error = NULL;
  GOptionContext *context;
  context = g_option_context_new ("- grotz");
  g_option_context_add_main_entries (context, entries, NULL);
  g_option_context_add_group (context, gtk_get_option_group (TRUE));

  if (!g_option_context_parse (context, &argc, &argv, &error))
  {
    g_critical ("Command line syntax error: %s\n", error->message);
    g_error_free (error);
    exit (1);
  }

  if (version)
  {
    printf (APPNAME " version %s\n", VERSION);
    printf ("(c)2011 Kevin Boone\n");
    exit (0);
  }

  g_log_set_default_handler (main_log_handler, NULL);

  g_debug ("Starting\n");

  // Work out where to look for the config file
  GString *config_dir = g_string_new ("");
  GString *config_file = g_string_new ("");

#ifdef WIN32
  g_string_printf (config_dir, "%s\\%s", getenv("APPDATA"), APPNAME);
  g_string_printf (config_file, "%s\\%s.ini", config_dir->str, APPNAME);
#else
  g_string_printf (config_dir, "%s/.%s", getenv("HOME"), APPNAME);
  g_string_printf (config_file, "%s/%s.ini", config_dir->str, APPNAME);
#endif

  g_message ("Using config file %s", config_file->str);

#ifdef WIN32
  mkdir (config_dir->str);
#else
  mkdir (config_dir->str, 0777);
#endif
    
  Settings *settings = settings_new (config_file->str);
  settings_load (settings);
  main_window = MAINWINDOW (mainwindow_new (settings, 
    config_dir->str));
 
  gtk_widget_show (GTK_WIDGET (main_window));

#ifndef WIN32
  signal (SIGCHLD, handle_child_finished);
#endif

  if (argc >= 2)
    mainwindow_open_file (main_window, argv[1]);

  gtk_main();

  // It's never really been clear to me why we don't have to 
  // destroy the main window here. But it gives a spiteful message
  // if we try
  //gtk_widget_destroy (GTK_WIDGET (main_window));

  g_string_free (config_file, TRUE);
  g_string_free (config_dir, TRUE);

  g_debug ("Done");
  return 0;
}

