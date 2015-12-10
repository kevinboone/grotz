#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "fileutils.h"
#ifdef WIN32
#include <windows.h>
#endif

/*======================================================================
fileutils_get_extension
======================================================================*/
GString *fileutils_get_extension (const char *filename)
  {
  if (filename == NULL) return g_string_new ("");
  char *p = strrchr (filename, '.');
  if (!p) return g_string_new ("");
  return g_string_new (p+1);
  }


/*======================================================================
fileutils_strip_extension
======================================================================*/
GString *fileutils_strip_extension (const char *filename)
  {
  GString *s = g_string_new (filename);
  char *p = strrchr (s->str, '.');
  if (!p) return s;
  s = g_string_truncate (s, p - s->str);
  return s;
  }


/*======================================================================
fileutils_get_dirname
This function always succeeds, but the return might be an empty
string
======================================================================*/
GString *fileutils_get_dirname (const char *filename)
  {
  const char *p;
#ifdef WIN32
  p = strrchr (filename, '\\');
#else
  p = strrchr (filename, '/');
#endif
  if (!p) return g_string_new (""); 

  GString *dir = g_string_new (filename);
  dir = g_string_truncate (dir, p - filename + 1);
  return dir;
  }


/*======================================================================
fileutils_get_filename
This function always succeeds, but the return might be an empty
string, or just a copy of the input
======================================================================*/
GString *fileutils_get_filename (const char *path)
  {
  const char *p;
#ifdef WIN32
  p = strrchr (path, '\\');
#else
  p = strrchr (path, '/');
#endif
  if (!p) return g_string_new (path); // No dir in path 

  GString *file = g_string_new (p + 1);
  return file;
  }


/*======================================================================
fileutils_concat_path
======================================================================*/
GString *fileutils_concat_path (const char *dir, const char *filename)
  {
  GString *s = g_string_new (dir);
#ifdef WIN32
  if (dir[strlen(dir) - 1] != '\\')
    s = g_string_append (s, "\\");
#else
  if (dir[strlen(dir) - 1] != '/')
    s = g_string_append (s, "/");
#endif
  g_string_append (s, filename);
  return s;
  }


/*======================================================================
fileutils_get_full_argv0
======================================================================*/
GString *fileutils_get_full_argv0 (void)
  {
#ifdef WIN32
  char buff[1024];
  GetModuleFileName (NULL, buff, sizeof (buff));
  return g_string_new (buff);
#else
  char szTmp[32];
  char *buff = (char *) malloc (1024); // Should never be this long!
  memset (buff, 0, 1024);
  snprintf(szTmp, sizeof (szTmp), "/proc/%d/exe", getpid());
  readlink (szTmp, buff, 1023);
  GString *p = g_string_new (buff);
  free (buff);
  return p;
#endif
  }






