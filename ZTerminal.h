#pragma once

#include <gtk/gtk.h>
#include "StoryTerminal.h" 

G_BEGIN_DECLS

#define ZTERMINAL_TYPE            (zterminal_get_type ())
#define ZTERMINAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ZTERMINAL_TYPE, ZTerminal))
#define ZTERMINAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ZTERMINAL_TYPE, ZTerminalClass))
#define IS_ZTERMINAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ZTERMINAL_TYPE))
#define IS_ZTERMINAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ZTERMINAL_TYPE))
#define ZTERMINAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ZTERMINAL_TYPE, ZTerminalClass))

typedef struct _ZTerminal ZTerminal;
typedef struct _ZTerminalClass  ZTerminalClass;

struct _ZTerminal 
  {
  StoryTerminal base;
  gboolean dispose_has_run;
  struct _ZTerminalPriv *priv;
  };

struct _ZTerminalClass
  {
  StoryTerminalClass parent_class;
  void (* zterminal) (ZTerminal *it);
  };

GType zterminal_get_type (void);

ZTerminal *zterminal_new (void);

gunichar2 zterminal_read_line (ZTerminal *self, int max, 
     gunichar2 *line, int timeout, 
     int width, gboolean continued, int *mouse_x, int *mouse_y);
gunichar2 zterminal_read_key (ZTerminal *self, int timeout, 
     gboolean show_cursor, int *mouse_x, int *mouse_y);
void zterminal_margins_to_bg (ZTerminal *self);

G_END_DECLS




