#pragma once

#include <gtk/gtk.h>
#include "colourutils.h" 

G_BEGIN_DECLS

#define STORYTERMINAL_TYPE            (storyterminal_get_type ())
#define STORYTERMINAL(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), STORYTERMINAL_TYPE, StoryTerminal))
#define STORYTERMINAL_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), STORYTERMINAL_TYPE, StoryTerminalClass))
#define IS_STORYTERMINAL(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), STORYTERMINAL_TYPE))
#define IS_STORYTERMINAL_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), STORYTERMINAL_TYPE))
#define STORYTERMINAL_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), STORYTERMINAL_TYPE, StoryTerminalClass))

struct _MainWindow;
typedef struct _StoryTerminal StoryTerminal;
typedef struct _StoryTerminalClass  StoryTerminalClass;

// 8x8 pixel glyphs
#define ST_CUSTOM_GLYPH_SIZE 128

typedef enum 
  {
  ST_INPUT_NONE = 0,
  ST_INPUT_KEY = 1,
  ST_INPUT_SINGLE_CLICK = 2,
  ST_INPUT_DOUBLE_CLICK = 3,
  ST_INPUT_TIMEOUT = 4,
  ST_INPUT_KILL_LINE_FROM_CURSOR = 5, 
  ST_INPUT_KILL_LINE = 6, 
  ST_INPUT_NEXT_WORD = 7, 
  ST_INPUT_PREV_WORD = 8 
  } STInputType;

#define STSTYLE_NORMAL   0x0000
#define STSTYLE_REVERSE  0x0001
#define STSTYLE_BOLD     0x0002 
#define STSTYLE_ITALIC   0x0004 
#define STSTYLE_FIXED    0x0008 

typedef int STStyle;

typedef enum
{
  STFONT_NORMAL = 0,
  STFONT_FIXED = 1,
  STFONT_CUSTOM = 2
} STFontCode;


typedef struct _STInput
  {
  STInputType type;
  int key; // GDK keycode
  int mouse_x;
  int mouse_y;
  } STInput;


struct _StoryTerminal 
  {
  GtkDrawingArea base;
  gboolean dispose_has_run;
  struct _StoryTerminalPriv *priv;
  };

struct _StoryTerminalClass
  {
  GtkDrawingAreaClass parent_class;
  void (* storyterminal) (StoryTerminal *it);
  void (* storyterminal_get_custom_glyph) (struct _StoryTerminal *self, 
    int c, char *glyph_data);
  };

GType storyterminal_get_type (void);

StoryTerminal *storyterminal_new (void);

void storyterminal_set_size (StoryTerminal *self, int height, int width);

void storyterminal_set_font_name_main (StoryTerminal *self,
    const char *font_name);

void storyterminal_set_font_name_fixed (StoryTerminal *self,
    const char *font_name);

void storyterminal_set_font_size
    (StoryTerminal *self, int font_size);

void storyterminal_wait_for_input (StoryTerminal *self, STInput *input,
    gboolean accept_mouse, gboolean show_cursor, int timeout);

void storyterminal_write_input_line (StoryTerminal *self,
      gunichar2 *line, int input_pos, int max, int width, gboolean caret);

void storyterminal_erase_area (StoryTerminal *self,
      int top, int left, int bottom, int right, gboolean immediate); 

void storyterminal_erase_gfx_area (StoryTerminal *self,
      int x1, int x2, int w, int y, gboolean immediate); 

void storyterminal_set_default_bg_colour (StoryTerminal *self,
        RGB8COLOUR bg_colour);

void storyterminal_set_default_fg_colour (StoryTerminal *self,
        RGB8COLOUR fg_colour);

void storyterminal_write_char (StoryTerminal *self, gunichar2 c, 
  gboolean immediate);

void storyterminal_set_auto_scroll (StoryTerminal *self, gboolean f);

void storyterminal_scroll_up (StoryTerminal *self, gboolean immediate);

void storyterminal_cr (StoryTerminal *self, gboolean immediate);

void storyterminal_lf (StoryTerminal *self, gboolean immediate);

void storyterminal_backspace (StoryTerminal *self, gboolean destructive,
       gboolean immediate);

RGB8COLOUR storyterminal_get_default_fg_colour (const StoryTerminal *self);
RGB8COLOUR storyterminal_get_default_bg_colour (const StoryTerminal *self);

void storyterminal_set_text_style (StoryTerminal *self, STStyle text_style);
STStyle storyterminal_get_text_style (const StoryTerminal *self);

void storyterminal_set_bg_colour (StoryTerminal *self, 
    RGB8COLOUR colour);

void storyterminal_set_fg_colour (StoryTerminal *self, 
    RGB8COLOUR colour);

void storyterminal_set_font_code (StoryTerminal *self, 
    STFontCode font_code);
STFontCode storyterminal_get_font_code (const StoryTerminal *self);

RGB8COLOUR storyterminal_peek_colour (const StoryTerminal *self);

void storyterminal_scroll_area (StoryTerminal *self, int top, int left, 
    int bottom, int right, int units, gboolean immediate);

void storyterminal_get_char_cell_size_in_pixels (const StoryTerminal *self,
    int *cw, int *ch);

void storyterminal_draw_pixbuf (StoryTerminal *self, GdkPixbuf *pb, 
    int row, int col);

void storyterminal_draw_pixbuf_at_gfx (StoryTerminal *self, GdkPixbuf *pb, 
    int x, int y);

void storyterminal_set_cursor (StoryTerminal *self, int row, int col);

void storyterminal_reset (StoryTerminal *self);

void storyterminal_get_gfx_pos (const StoryTerminal *self, int *x, int *y);

void storyterminal_get_cursor_pos (const StoryTerminal *self, int *r, int *c);

void storyterminal_get_gfx_cursor_pos (const StoryTerminal *self, int *x, int *y);

void storyterminal_get_widget_size (const StoryTerminal *self, int *width, 
    int *height);

void storyterminal_set_gfx_cursor (StoryTerminal *self, int x, int y);

int storyterminal_get_char_width (const StoryTerminal *terminal, gunichar2 c);

void storyterminal_scroll_gfx_area (StoryTerminal *self, 
    int x, int y, int w, int h, int units, gboolean immediate);

void storyterminal_draw_pixbuf_at_gfx (StoryTerminal *self, GdkPixbuf *pb, 
    int x, int y);

int storyterminal_get_string_width (const StoryTerminal *self, const char *s);

RGB8COLOUR storyterminal_peek_colour_under_gfx_cursor 
   (const StoryTerminal *self);

gunichar2 storyterminal_get_last_letter_output (const StoryTerminal *self);

void storyterminal_add_to_input_buffer_utf16 (StoryTerminal *self,
    const gunichar2 *s);

void storyterminal_add_to_input_buffer_utf8 (StoryTerminal *self,
    const char *s);

void storyterminal_kill_line_from_cursor (StoryTerminal *self);

void storyterminal_kill_line (StoryTerminal *self);
 
void storyterminal_paste_quit (StoryTerminal *self);

void storyterminal_go_to_next_word (StoryTerminal *self);

void storyterminal_go_to_prev_word (StoryTerminal *self);

void storyterminal_get_unit_size (const StoryTerminal *self, int *r, 
    int *c);

RGB8COLOUR storyterminal_get_bg_colour (const StoryTerminal *self);
RGB8COLOUR storyterminal_get_fg_colour (const StoryTerminal *self);

void storyterminal_set_main_window (StoryTerminal *self, 
    struct _MainWindow *main_window);

struct _MainWindow *storyterminal_get_main_window (const StoryTerminal *self);

gboolean storyterminal_get_ignore_game_colours (const StoryTerminal *self);

G_END_DECLS




