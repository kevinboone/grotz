/*
StoryTerminal is an implementation of a graphics terminal, broadly suitable
for IF applications.

Things to watch out for:

1. This widget never requests size. It makes its own preferred size available
to its own by means of storyterminal_get_widget_size. This size is based
on the font size and number of rows and columns. The widget may be allocated
more or less space than this.

2. If the widget is allocated more than its preferred size, then that
will leave a border right and bottom, whose colour may not be the 
same as the background colour currently set for output operations. The
owner of this widget is expected to set the gtk background colour to
match itself, or to match the output colours, whatever works best. 

*/

#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include "StoryTerminal.h"
#include "charutils.h"
#include "colourutils.h"
#include "MainWindow.h"

G_DEFINE_TYPE (StoryTerminal, storyterminal, GTK_TYPE_DRAWING_AREA);

// Time between screen updates
#define ST_FLUSH_MSEC 100

typedef struct _StoryTerminalPriv
{
  int rows;
  int cols;
  PangoFontDescription *pfd_fixed;
  PangoFontDescription *pfd_main;
  char *font_name_fixed;
  char *font_name_main;
  int char_width;
  int char_height;
  int font_size;
  // Input buffer is an array of STInput objects. NOT POINTERS!
  GArray *input_event_array; 
  GdkPixmap *graphics_buffer;
  GdkGC *gc;
  gboolean dirty;
  int cursor_row;
  int cursor_col;
  RGB8COLOUR bg_colour;
  RGB8COLOUR fg_colour;
  RGB8COLOUR default_bg_colour;
  RGB8COLOUR default_fg_colour;
  int gfx_x;
  int gfx_y;
  gboolean auto_scroll;
  STStyle text_style;
  STFontCode font_code;
  gunichar2 last_letter_output;
  MainWindow *main_window;
} StoryTerminalPriv;

void storyterminal_recalc_fonts (StoryTerminal *self);
void storyterminal_deallocate_graphics_buffer (StoryTerminal *self);
void storyterminal_flush_buffer (StoryTerminal *self);
void storyterminal_get_char_cell_size_in_pixels (const StoryTerminal *self,
    int *cw, int *ch);
void storyterminal_home (StoryTerminal *self);
void storyterminal_erase_gfx_area (StoryTerminal *self,
      int x, int y, int w, int h, gboolean immediate);
gboolean storyterminal_timeout (StoryTerminal *self);



/*======================================================================
  storyterminal_clear_input_buffer
=====================================================================*/
void storyterminal_clear_input_buffer (StoryTerminal *self)
  {
  int l = self->priv->input_event_array->len;
  if (l > 0)
    g_array_remove_range (self->priv->input_event_array, 0, l);
  }


/*======================================================================
  storyterminal_get_widget_size
  Gets the size a widget would like to be (and will assume it is in
  all drawing operations) based on the current number of rows and columns
  and font size
=====================================================================*/
void storyterminal_get_widget_size (const StoryTerminal *self, int *width, 
    int *height)
  {
  *width = self->priv->char_width * self->priv->cols;
  *height = self->priv->char_height * self->priv->rows;
  }


/*======================================================================
  storyterminal_get_unit_size
=====================================================================*/
void storyterminal_get_unit_size (const StoryTerminal *self, int *r, 
    int *c)
  {
  *r = self->priv->rows;
  *c = self->priv->cols;
  }


/*======================================================================
  storyterminal_set_gfx_from_cursor
======================================================================*/
void storyterminal_set_gfx_from_cursor (StoryTerminal *self)
  {
  int cw, ch;
  storyterminal_get_char_cell_size_in_pixels (self,
    &cw, &ch);
  self->priv->gfx_x = self->priv->cursor_col * cw;
  self->priv->gfx_y = self->priv->cursor_row * ch;
  }


/*======================================================================
  storyterminal_set_cursor
======================================================================*/
void storyterminal_set_cursor (StoryTerminal *self, int row, int col)
  {
  if (row < 0 || row >= self->priv->rows) return; 
  if (col < 0 || col >= self->priv->cols) return; 
  self->priv->cursor_row = row;
  self->priv->cursor_col = col;
  storyterminal_set_gfx_from_cursor (self);
  }

/*======================================================================
  storyterminal_home
======================================================================*/
void storyterminal_home (StoryTerminal *self)
{
  storyterminal_set_cursor (self, 0, 0);
}


/*======================================================================
  storyterminal_mark_dirty
=====================================================================*/
void storyterminal_mark_dirty (StoryTerminal *self)
  {
  self->priv->dirty = TRUE;
  }


/*======================================================================
  storyterminal_mark_clean
=====================================================================*/
void storyterminal_mark_clean (StoryTerminal *self)
  {
  self->priv->dirty = FALSE;
  }


/*======================================================================
  storyterminal_clear_graphics_buffer
=====================================================================*/
void storyterminal_clear_graphics_buffer (StoryTerminal *self)
  {
  storyterminal_mark_dirty (self);
  if (!self->priv->graphics_buffer) return;
  if (!GTK_WIDGET_REALIZED (GTK_WIDGET (self))) return;
  if (!GDK_IS_GC(self->priv->gc)) return;
  GdkColor gdkc;
  colourutils_rgb8_to_gdk (self->priv->bg_colour, &gdkc);
  gdk_gc_set_rgb_fg_color (self->priv->gc, &gdkc);
  gdk_gc_set_rgb_bg_color (self->priv->gc, &gdkc);

  int width, height;
  storyterminal_get_widget_size (self, &width, &height);

  gdk_draw_rectangle (self->priv->graphics_buffer, self->priv->gc, 
      TRUE, 0, 0, width, height); 

  storyterminal_flush_buffer (self); 
  }


/*======================================================================
  storyterminal_allocate_and_clear_graphics_buffer
=====================================================================*/
void storyterminal_allocate_and_clear_graphics_buffer 
    (StoryTerminal *self)
  {
  GtkWidget *w = GTK_WIDGET (self);
  if (self->priv->graphics_buffer == NULL)
    {
    int width;
    int height;
    storyterminal_get_widget_size (self, &width, &height);
    self->priv->graphics_buffer = gdk_pixmap_new (w->window, 
      width, height, -1);
    }
  if (self->priv->gc == NULL)
    self->priv->gc = gdk_gc_new (w->window);
  storyterminal_clear_graphics_buffer (self);
  }


/*======================================================================
  storyterminal_deallocate_graphics_buffer
=====================================================================*/
void storyterminal_deallocate_graphics_buffer 
    (StoryTerminal *self)
  {
  if (self->priv->graphics_buffer)
    {
    g_object_unref (self->priv->graphics_buffer);
    self->priv->graphics_buffer = NULL;
    }
  if (self->priv->gc)
    {
    g_object_unref (self->priv->gc);
    self->priv->gc = NULL;
    }
  }


/*======================================================================
  storyterminal_realize_event
=====================================================================*/
static void storyterminal_realize_event (GtkWidget *w, gpointer data)
{
  storyterminal_allocate_and_clear_graphics_buffer (STORYTERMINAL(w));
}


/*======================================================================
  storyterminal_button_press_event
  _COPIES_ the supplied input object and puts it on the buffer.
  The caller can (and should) free its own copy, if it was dynamically
  allocated
======================================================================*/
void storyterminal_add_to_input_buffer (StoryTerminal *self, 
    const STInput *_input)
  {
  STInput input;
  memcpy (&input, _input, sizeof (STInput));
  g_array_append_val (self->priv->input_event_array, input);
  }

/*======================================================================
  storyterminal_button_press_event
======================================================================*/
gboolean storyterminal_button_press_event (GtkWidget *w, GdkEventButton *ev, 
  gpointer user_data)
{
  gtk_widget_grab_focus (w);
  StoryTerminal *self = (StoryTerminal *)user_data;

  STInput input;
  input.mouse_x = ev->x;
  input.mouse_y = ev->y;
  input.type = 
    (ev->type == GDK_2BUTTON_PRESS ? 
       ST_INPUT_DOUBLE_CLICK : ST_INPUT_SINGLE_CLICK); 

  storyterminal_add_to_input_buffer (self, &input);

  return TRUE;
}



/*======================================================================
  storyterminal_key_event
======================================================================*/
gboolean storyterminal_key_event (GtkWidget *w, GdkEventKey *ev, 
  gpointer user_data)
{
  StoryTerminal *self = (StoryTerminal *)user_data;

  STInput input;
  input.type = ST_INPUT_KEY;
  input.key = ev->keyval; 

  storyterminal_add_to_input_buffer (self, &input);

  return TRUE;
}


/*======================================================================
  storyterminal_flush_buffer
=====================================================================*/
void storyterminal_flush_buffer (StoryTerminal *self)
{
  if (!self->priv->graphics_buffer) return;

  GtkWidget *w = GTK_WIDGET (self);
  gdk_draw_drawable (w->window, 
    self->priv->gc,
    self->priv->graphics_buffer,
    0, 0, 0, 0, -1, -1);
  storyterminal_mark_clean (self);
}


/*======================================================================
  storyterminal_reset
=====================================================================*/
void storyterminal_reset (StoryTerminal *self)
{
  storyterminal_home (self);
  storyterminal_clear_graphics_buffer (self);
  storyterminal_clear_input_buffer (self);
  self->priv->default_bg_colour = RGB8WHITE;
  self->priv->default_fg_colour = RGB8BLACK;
  self->priv->fg_colour = self->priv->default_fg_colour; 
  self->priv->bg_colour = self->priv->default_bg_colour; 
  self->priv->text_style = STSTYLE_NORMAL; 
  self->priv->font_code = STFONT_NORMAL; 
  self->priv->last_letter_output = 0;
}

/*======================================================================
  storyterminal_expose_event
=====================================================================*/
static gboolean storyterminal_expose_event (GtkWidget *w, GdkEventExpose *ev,
    gpointer data)
{
  StoryTerminal *self = (StoryTerminal *)w;
  storyterminal_flush_buffer (self);
  return TRUE;
}


/*======================================================================
  storyterminal_size_allocate_event
=====================================================================*/
static void storyterminal_size_allocate_event (GtkWidget *w, 
    GdkRectangle *a, gpointer data)
{
  g_debug ("StoryTerminal resize to %d,%d", a->width, a->height);
  StoryTerminal *self = STORYTERMINAL (w);
  int cols = a->width / self->priv->char_width; 
  int rows = a->height / self->priv->char_height; 
  storyterminal_set_size (self, rows, cols);
  g_debug ("StoryTerminal set unit size to r=%d,r=%d", rows, cols);
}


/*======================================================================
  storyterminal_init
=====================================================================*/

static void storyterminal_init (StoryTerminal *self)
{
  g_debug ("Initialising storyterminal\n");
  self->dispose_has_run = FALSE;
  self->priv = (StoryTerminalPriv *) malloc (sizeof (StoryTerminalPriv));
  memset (self->priv, 0, sizeof (StoryTerminalPriv));
  self->priv->pfd_main = NULL; 
  self->priv->pfd_fixed = NULL; 
  self->priv->font_name_fixed = strdup ("Monospace");
  self->priv->font_name_main = strdup ("Serif");
  self->priv->font_size = 10;
  self->priv->default_bg_colour = RGB8WHITE;
  self->priv->default_fg_colour = RGB8BLACK;
  self->priv->fg_colour = self->priv->default_fg_colour; 
  self->priv->bg_colour = self->priv->default_bg_colour; 
  self->priv->auto_scroll = TRUE;
  self->priv->text_style = STSTYLE_NORMAL; 
  self->priv->font_code = STFONT_NORMAL; 
  self->priv->input_event_array = g_array_new (FALSE, TRUE, sizeof (STInput));
  gtk_widget_add_events (GTK_WIDGET (self), GDK_BUTTON_PRESS_MASK);
  gtk_widget_add_events (GTK_WIDGET (self), GDK_KEY_PRESS_MASK);
  gtk_widget_add_events (GTK_WIDGET (self), GDK_FOCUS_CHANGE_MASK);
  GTK_WIDGET_SET_FLAGS (GTK_WIDGET (self), GTK_CAN_FOCUS); 
  gtk_widget_grab_focus (GTK_WIDGET (self));
  g_signal_connect (G_OBJECT (self), "expose_event",
     G_CALLBACK (storyterminal_expose_event), self);
  g_signal_connect (G_OBJECT (self), "size_allocate",
     G_CALLBACK (storyterminal_size_allocate_event), self);
  g_signal_connect (G_OBJECT (self), "realize",
     G_CALLBACK (storyterminal_realize_event), self);
  storyterminal_recalc_fonts (self);
  g_signal_connect (G_OBJECT(self), "key-press-event",
     G_CALLBACK (storyterminal_key_event), self);       
  g_signal_connect (G_OBJECT(self), "button-press-event",
     G_CALLBACK (storyterminal_button_press_event), self);       
  storyterminal_reset (self);
  g_timeout_add (ST_FLUSH_MSEC, (GSourceFunc) storyterminal_timeout, self);
}


/*======================================================================
  storyterminal_dispose
======================================================================*/

static void storyterminal_dispose (GObject *_self)
{
  StoryTerminal *self = (StoryTerminal *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing storyterminal\n");
  self->dispose_has_run = TRUE;
  storyterminal_clear_input_buffer (self);
  if (self->priv->pfd_main) 
  {
    pango_font_description_free (self->priv->pfd_main);
    self->priv->pfd_main = NULL;
  }
  if (self->priv->pfd_fixed) 
  {
    pango_font_description_free (self->priv->pfd_fixed);
    self->priv->pfd_fixed = NULL;
  }
  storyterminal_deallocate_graphics_buffer (self);
  if (self->priv)
  {
    free (self->priv);
    self->priv = NULL;
  }
  G_OBJECT_CLASS (storyterminal_parent_class)->dispose (_self);
}


/*======================================================================
  storyterminal_class_init
======================================================================*/

static void storyterminal_class_init
  (StoryTerminalClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = storyterminal_dispose;
}


/*======================================================================
  storyterminal_new
======================================================================*/

StoryTerminal *storyterminal_new ()
{
  StoryTerminal *this = g_object_new 
   (STORYTERMINAL_TYPE, NULL);
  return this;
}


/*======================================================================
  storyterminal_get_char_cell_size_in_pixels
======================================================================*/
void storyterminal_get_char_cell_size_in_pixels (const StoryTerminal *self,
    int *cw, int *ch)
  {
  *cw = self->priv->char_width;
  *ch = self->priv->char_height;
  }


/*======================================================================
  storyterminal_size_widget_to_fit
  Requests space to fit the number of rows and columns, at the specified
  font size
======================================================================*/
/*
void storyterminal_size_widget_to_fit (StoryTerminal *self)
  {
  int char_height;
  int char_width;
  storyterminal_get_char_cell_size_in_pixels 
    (self, &char_width, &char_height);

  //int w = char_width * self->priv->cols;
  //int h = char_height * self->priv->rows;
  //gtk_widget_set_size_request (GTK_WIDGET (self), 
   // w, h);

  gtk_widget_queue_draw (GTK_WIDGET (self));
  }
*/

/*======================================================================
  storyterminal_set_size
  Sets the size in screen cells
======================================================================*/
void storyterminal_set_size (StoryTerminal *self, int height, int width)
  {
  g_debug ("StoryTerminal set_size %d rows, %d cols\n", height, width);
  if (height == self->priv->rows && width == self->priv->cols)
    {
    g_debug ("StoryTerminal set_size ignored because size is not changed");
    return;
    }

  //int old_rows = -1;
  //int old_cols = -1;
  int old_cursor_row = -1;
  int old_cursor_col = -1;
  int old_gfx_x = -1;
  int old_gfx_y = -1;
  gboolean copy = FALSE;
  GdkPixmap *old_graphics_buffer;

  if (self->priv->rows != 0)
    {
    copy = TRUE;
    //old_rows = self->priv->rows;
    //old_cols = self->priv->cols;
    old_cursor_row = self->priv->cursor_row;
    old_cursor_col = self->priv->cursor_col;
    old_gfx_x = self->priv->gfx_x;
    old_gfx_y = self->priv->gfx_y;
    old_graphics_buffer = self->priv->graphics_buffer;
    g_object_ref (old_graphics_buffer);
    }

  storyterminal_deallocate_graphics_buffer (self);
  if (GTK_WIDGET_REALIZED (GTK_WIDGET (self)))
    {
    storyterminal_allocate_and_clear_graphics_buffer (self);
    }

  self->priv->rows = height;
  self->priv->cols = width;
  //storyterminal_size_widget_to_fit (self);  

  // If there is screen data from before the resize, try to copy it
  //  to the new screen. Of course, it won't necessarily be a good fit 
  if (copy)
    {
    // TODO copy old screen contents
    gdk_draw_drawable (self->priv->graphics_buffer, 
      self->priv->gc,
      old_graphics_buffer,
      0, 0, 0, 0, -1, -1);
    g_object_unref (old_graphics_buffer);
    storyterminal_set_cursor (self, old_cursor_row, old_cursor_col);
    self->priv->gfx_x = old_gfx_x;
    self->priv->gfx_y = old_gfx_y;
    self->priv->cursor_row = old_cursor_row;
    self->priv->cursor_col = old_cursor_col;
    }
  storyterminal_mark_dirty (self);
  }


/*======================================================================
  storyterminal_recalc_fonts
======================================================================*/
void storyterminal_recalc_fonts (StoryTerminal *self)
  {
  if (self->priv->pfd_main) pango_font_description_free 
    (self->priv->pfd_main);
  if (self->priv->pfd_fixed) pango_font_description_free 
    (self->priv->pfd_fixed);
  GString *name_fixed = g_string_new ("");
  GString *name_main = g_string_new ("");
  g_string_printf (name_fixed, "%s Regular %d", 
    self->priv->font_name_fixed, self->priv->font_size);
  g_string_printf (name_main, "%s Regular %d", 
    self->priv->font_name_main, self->priv->font_size);
  self->priv->pfd_fixed = pango_font_description_from_string (name_fixed->str);
  self->priv->pfd_main = pango_font_description_from_string (name_main->str);
  PangoContext *pc = gtk_widget_get_pango_context (GTK_WIDGET (self)) ;
  PangoLayout *layout = pango_layout_new (pc);
  // work out maximum text cell spices for bold italic, since this
  //  will be less likely to lead to clipping
  pango_layout_set_markup (layout, "<i><b>MjX</b></i>", -1);
  pango_layout_set_font_description (layout, self->priv->pfd_fixed);
  int char_height;
  int char_width;
  pango_layout_get_pixel_size (layout, &char_width, &char_height);
  char_width /= 3;
  char_height += 0;
  g_object_unref (layout);
  self->priv->char_width = char_width;
  self->priv->char_height = char_height;
  }


/*======================================================================
  storyterminal_set_font_name_main
  There should be no need to notify the interpreter if a font
  name has changed as this should not (ideally) mean a change 
  in screen size
======================================================================*/
void storyterminal_set_font_name_main (StoryTerminal *self,
    const char *font_name)
  {
  if (self->priv->font_name_main)
    free (self->priv->font_name_main);
  self->priv->font_name_main = strdup (font_name);
  storyterminal_recalc_fonts (self); 
  }


/*======================================================================
  storyterminal_set_font_name_fixed
======================================================================*/
void storyterminal_set_font_name_fixed (StoryTerminal *self,
    const char *font_name)
  {
  if (self->priv->font_name_fixed)
    free (self->priv->font_name_fixed);
  self->priv->font_name_fixed = strdup (font_name);
  storyterminal_recalc_fonts (self); 
  }


/*======================================================================
  storyterminal_set_font_size
======================================================================*/
void storyterminal_set_font_size
    (StoryTerminal *self, int font_size)
  {
  self->priv->font_size = font_size;
  storyterminal_recalc_fonts (self);

  GtkWidget *w = GTK_WIDGET (self);
  int cx = w->allocation.width;
  int cy = w->allocation.height;

  if (cx == 1)
    {
    int char_height;
    int char_width;
    storyterminal_get_char_cell_size_in_pixels 
      (self, &char_width, &char_height);

    cx = char_width * self->priv->cols;
    cy = char_height * self->priv->rows;
    }
  }


/*======================================================================
  storyterminal_wait_for_input
  TODO: show cursor
======================================================================*/
void storyterminal_wait_for_input (StoryTerminal *self, STInput *input,
    gboolean accept_mouse, gboolean show_cursor, int timeout)
  {

  int x1 = self->priv->gfx_x + 0; 
  int y1 = self->priv->gfx_y; 
  
  if (show_cursor)
    {
    // Draw the caret
    gdk_draw_rectangle (self->priv->graphics_buffer, 
                self->priv->gc,
                TRUE,
                x1, y1, 1, self->priv->char_height);

    GtkWidget *w = GTK_WIDGET(self);
    gdk_draw_rectangle (w->window, 
                self->priv->gc,
                TRUE,
                x1, y1, 1, self->priv->char_height);
    }

  memset (input, 0, sizeof (STInput));
  long elapsed = 0; // msec

  while (self->priv->input_event_array->len == 0)
    {
    // TODO check if mouse input is OK
    usleep (50000); // 50 msec idle
    elapsed += 50; 
    while (gtk_events_pending()) gtk_main_iteration();
    if (timeout > 0 && elapsed > timeout)
      {
      memset (input, 0, sizeof (STInput));
      input->type = ST_INPUT_TIMEOUT;
      return;
      }
    }

  if (show_cursor)
    {
    // Erase the caret
    storyterminal_erase_gfx_area 
      (self, x1, y1, 1, self->priv->char_height, TRUE);
    }

  memcpy (input, self->priv->input_event_array->data, sizeof (STInput));
  g_array_remove_index (self->priv->input_event_array, 0);
  }


/*======================================================================
  storyterminal_erase_gfx_area
======================================================================*/
void storyterminal_erase_gfx_area (StoryTerminal *self,
      int x, int y, int w, int h, gboolean immediate) 
  {
  GdkColor gdkc;
  colourutils_rgb8_to_gdk (self->priv->bg_colour, &gdkc);
  gdk_gc_set_rgb_fg_color (self->priv->gc, &gdkc);
  gdk_gc_set_rgb_bg_color (self->priv->gc, &gdkc);

  // Add 1 to w and h to account for gdk filled rectangle
  // anomaly
  gdk_draw_rectangle (self->priv->graphics_buffer, self->priv->gc, 
      TRUE, x, y, w + 1, h + 1);
 
  if (immediate)
    {
    gdk_draw_rectangle (GTK_WIDGET(self)->window,  self->priv->gc, 
        TRUE, x, y, w + 1, h + 1);
    }
  else
    {
    storyterminal_mark_dirty (self);
    }

  }

/*======================================================================
  storyterminal_erase_area
======================================================================*/
void storyterminal_erase_area (StoryTerminal *self,
      int top, int left, int bottom, int right, gboolean immediate) 
  {
  if (top < 0) top = 0;
  if (left < 0) left = 0;
  if (bottom >= self->priv->rows) bottom = self->priv->rows - 1;
  if (right >= self->priv->cols) right = self->priv->cols - 1;

  int cw, ch;
  storyterminal_get_char_cell_size_in_pixels (self, &cw, &ch);
  int x1 = left * cw;
  int x2 = (right + 1) * cw; 
  int y1 = top * ch;
  int y2 = (bottom  + 1) * ch;
      
  storyterminal_erase_gfx_area (self, x1, y1, x2 - x1, y2 - y1, immediate);
  }


/*======================================================================
  storyterminal_write_input_line
  Line argument must be null-terminated
  Does not move the graphics cursor
======================================================================*/
void storyterminal_write_input_line (StoryTerminal *self,
      gunichar2 *line, int input_pos, int max, int width, gboolean caret)
  {
  GtkWidget *w = GTK_WIDGET (self);

  GString *s_before = charutils_utf16_string_to_utf8 
        ((gunichar2*)line, input_pos);

  GString *s_after = charutils_utf16_string_to_utf8 
        ((gunichar2*)line + input_pos, -1);

  int x1 = self->priv->gfx_x + 0; 
  int y1 = self->priv->gfx_y; 
  int cx = max * self->priv->char_width;
  if (cx > width) cx = width;
  int cy = self->priv->char_height;

  PangoContext *pc = gtk_widget_get_pango_context (w);
  PangoLayout *layout = pango_layout_new (pc);

  // We need to draw the input in the currently-selected font
  if (self->priv->font_code == STFONT_FIXED || 
      (self->priv->text_style & STSTYLE_FIXED))
    pango_layout_set_font_description (layout, self->priv->pfd_fixed);
  else
    pango_layout_set_font_description (layout, self->priv->pfd_main);

  pango_layout_set_text (layout, s_before->str, strlen (s_before->str));
  int before_width, before_height;
  pango_layout_get_pixel_size (layout, &before_width, &before_height);

  // Why do we need to erase this extra 20 pixels width?
  if (cx < before_width) cx = before_width;
  storyterminal_erase_gfx_area (self, x1, y1, cx + 20, cy, TRUE);

  GdkColor gdkc;
  colourutils_rgb8_to_gdk (self->priv->fg_colour, &gdkc);
  gdk_gc_set_rgb_fg_color (self->priv->gc, &gdkc);


  // Draw the text before the caret
  gdk_draw_layout (self->priv->graphics_buffer,
                self->priv->gc,
                x1, y1, 
                layout);

  gdk_draw_layout (w->window,
                self->priv->gc,
                x1, y1, 
                layout);
  
  if (caret)
    {
    // Draw the caret
    gdk_draw_rectangle (self->priv->graphics_buffer, 
                self->priv->gc,
                TRUE,
                x1 + before_width, y1, 1, self->priv->char_height);

    gdk_draw_rectangle (w->window, 
                self->priv->gc,
                TRUE,
                x1 + before_width, y1, 1, self->priv->char_height);
    }

  // Draw the text after the caret
  pango_layout_set_text (layout, s_after->str, strlen (s_after->str));

  gdk_draw_layout (self->priv->graphics_buffer,
                self->priv->gc,
                x1 + before_width + 1, y1, 
                layout);

  gdk_draw_layout (w->window,
                self->priv->gc,
                x1 + before_width + 1, y1, 
                layout);

  g_object_unref (layout);
  g_string_free (s_before, TRUE);
  g_string_free (s_after, TRUE);

  }


/*======================================================================
  storyterminal_set_default_bg_colour
======================================================================*/
void storyterminal_set_default_bg_colour (StoryTerminal *self,
        RGB8COLOUR bg_colour)
  {
  self->priv->default_bg_colour = bg_colour;
  }

 
/*======================================================================
  storyterminal_set_default_fg_colour
======================================================================*/
void storyterminal_set_default_fg_colour (StoryTerminal *self,
        RGB8COLOUR bg_colour)
  {
  self->priv->default_fg_colour = bg_colour;
  }


/*======================================================================
      storyterminal_markup_text_for_attributes
======================================================================*/
static GString *storyterminal_markup_text_for_attributes 
     (StoryTerminal *self, const char *s, RGB8COLOUR colour,
      STStyle style, STFontCode font_code)
  {
  GString *ss = g_string_new (s);

  if (font_code == STFONT_FIXED || (style & STSTYLE_FIXED))
    {
    // This tag could probably be precomputed to save reworking
    // it for (potentially) every tt character
    ss = g_string_prepend (ss, "\">");
    ss = g_string_prepend (ss, self->priv->font_name_fixed);
    ss = g_string_prepend (ss, "<span  face=\"");
    ss = g_string_append (ss, "</span>");
    }

  if (style & STSTYLE_ITALIC)
    {
    ss = g_string_prepend (ss, "<i>");
    ss = g_string_append (ss, "</i>");
    }

  if (style & STSTYLE_BOLD)
    {
    ss = g_string_prepend (ss, "<b>");
    ss = g_string_append (ss, "</b>");
    }

  return ss;
  }


/*======================================================================
      storyterminal_markup_text_for_current_attributes
      On entry, s contains a _single_ character encoded as UTF8.
      The markup will be wrapped around that character
      Actually, multiple-character sequences will still work, but
      the use of a char * for input does not imply that there
      should be more than one character, or often will be
======================================================================*/
static GString *storyterminal_markup_text_for_current_attributes 
     (StoryTerminal *self, const char *s)
  {
  return storyterminal_markup_text_for_attributes 
    (self, s, self->priv->fg_colour, self->priv->text_style, 
      self->priv->font_code); 
  }


/*======================================================================
      storyterminal_nasty_font_hack
======================================================================*/
void storyterminal_nasty_font_hack (StoryTerminal *self, 
    int x, int y, gunichar2 c, gboolean immediate, int *move_x)
  {
  int cw = self->priv->char_width;
  int ch = self->priv->char_height;

  char bg_red = RGB8_GETRED (self->priv->bg_colour);
  char bg_green = RGB8_GETGREEN (self->priv->bg_colour);
  char bg_blue = RGB8_GETBLUE (self->priv->bg_colour);
  char fg_red = RGB8_GETRED (self->priv->fg_colour);
  char fg_green = RGB8_GETGREEN (self->priv->fg_colour);
  char fg_blue = RGB8_GETBLUE (self->priv->fg_colour);

  char chargrid[128 * 3];

  char dotgrid [128];
  
  STORYTERMINAL_GET_CLASS (self)->storyterminal_get_custom_glyph 
    (self, c, dotgrid);

  int i;
  for (i = 0; i < 128; i++)
    {
    if (dotgrid[i])
      {
      chargrid[i*3 + 0] = fg_red;
      chargrid[i*3 + 1] = fg_green;
      chargrid[i*3 + 2] = fg_blue;
      }
    else
      {
      chargrid[i*3 + 0] = bg_red;
      chargrid[i*3 + 1] = bg_green;
      chargrid[i*3 + 2] = bg_blue;
      }
    } 

  GdkPixbuf *pb = gdk_pixbuf_new_from_data ((guchar*)chargrid, 
    GDK_COLORSPACE_RGB, FALSE, 8, 8, 8, 8 * 3, NULL, NULL); 

  GdkPixbuf *pbs = gdk_pixbuf_scale_simple (pb, 
        cw, ch, 
        GDK_INTERP_BILINEAR);    

  storyterminal_draw_pixbuf_at_gfx (self, pbs, x, y);
  g_object_unref (pbs); 
  g_object_unref (pb);

  *move_x = *move_x + cw;
  }


/*======================================================================
      storyterminal_write_char_at_gfx
======================================================================*/
void storyterminal_write_char_at_gfx (StoryTerminal *self, 
    int x, int y, gunichar2 c, gboolean immediate, int *move_x)
  {
  if (!self->priv->graphics_buffer) return;

  if (self->priv->font_code == STFONT_CUSTOM)
    {
    storyterminal_nasty_font_hack (self, 
      x, y, c, immediate, move_x);
    return;
    }

  GtkWidget *w = GTK_WIDGET (self);
  PangoContext *pc = gtk_widget_get_pango_context (w);
  PangoLayout *layout = pango_layout_new (pc);

  char text[10]; // more that 5 should do
  charutils_utf16_char_to_utf8 (c, text, sizeof(text));

  char *escaped = g_markup_escape_text (text, -1);
  GString *markup = storyterminal_markup_text_for_current_attributes 
    (self, escaped);
  pango_layout_set_markup (layout, markup->str, strlen (markup->str));
  g_string_free (markup, TRUE);
  free (escaped);

  pango_layout_set_font_description (layout, self->priv->pfd_main);
  int width, height;
  pango_layout_get_pixel_size (layout, &width, &height);
  *move_x = width;
  
  // Note -- we must erase the cell before drawing. Z-machine assumes
  // that writes are destructive
  // Note note -- unless the Z app has called for transparent text

  GdkColor gdk_bg;
  GdkColor gdk_fg;

  if (self->priv->text_style & STSTYLE_REVERSE)
    {
    colourutils_rgb8_to_gdk (self->priv->bg_colour, &gdk_fg);
    colourutils_rgb8_to_gdk (self->priv->fg_colour, &gdk_bg);
    }
  else
    {
    colourutils_rgb8_to_gdk (self->priv->bg_colour, &gdk_bg);
    colourutils_rgb8_to_gdk (self->priv->fg_colour, &gdk_fg);
    }

  if (self->priv->bg_colour != RGB8TRANSPARENT)
    {
    gdk_gc_set_rgb_fg_color (self->priv->gc, &gdk_bg);
    gdk_gc_set_rgb_bg_color (self->priv->gc, &gdk_bg);
    gdk_draw_rectangle (self->priv->graphics_buffer, self->priv->gc, 
        TRUE, x, y, width, height);
    if (immediate)
      {
      gdk_draw_rectangle (self->priv->graphics_buffer, self->priv->gc, 
        TRUE, x, y, width, height);
      }
    }

  gdk_gc_set_rgb_bg_color (self->priv->gc, &gdk_bg);
  gdk_gc_set_rgb_fg_color (self->priv->gc, &gdk_fg);

  gdk_draw_layout (self->priv->graphics_buffer,
                self->priv->gc,
                x, y, 
                layout);

  if (immediate)
    {
    gdk_draw_layout (w->window,
                self->priv->gc,
                x, y, 
                layout);
    }
  
  if (!immediate)
    storyterminal_mark_dirty (self);

  g_object_unref (layout);
  }  


/*======================================================================
  storyterminal_erase_cell_at_units
======================================================================*/
void storyterminal_erase_cell_at_units 
        (StoryTerminal *self, int row, int col, gboolean immediate)
  {
  //GtkWidget *w = GTK_WIDGET (self);
  int char_height;
  int char_width;
  storyterminal_get_char_cell_size_in_pixels 
    (self, &char_width, &char_height);

  int x1 = col * char_width;
  int y1 = row * char_height;
  
  // Note that destructive backspacing can only possibly work properly
  //  if the font if fixed-pitch. This is just a limitation of writing
  //  with arbitrary fonts on a cell grid :/

  storyterminal_erase_gfx_area (self, x1, y1, 
    char_width, char_height, immediate); 
  }

/*======================================================================
  storyterminal_backspace
======================================================================*/
void storyterminal_backspace (StoryTerminal *self, gboolean destructive,
       gboolean immediate)
  {
  // Note that shuffling text is not implemented and, probably, not
  // required for ZMachine. Other terminals, maybe this needs to be
  // overridden
  
  if (self->priv->cursor_col == 0 && self->priv->cursor_row == 0)
    return; // Already at top left

  if (self->priv->cursor_col == 0)
    {
    self->priv->cursor_row--;
    self->priv->cursor_col = self->priv->cols - 1;
    }
  else
    {
    self->priv->cursor_col--;
    }

  if (destructive)
    {
    storyterminal_erase_cell_at_units 
        (self, self->priv->cursor_row, self->priv->cursor_col, immediate);
    }

  storyterminal_set_gfx_from_cursor (self); 
  }


/*======================================================================
  storyterminal_cr
======================================================================*/
void storyterminal_cr (StoryTerminal *self, 
       gboolean immediate)
  {
  self->priv->cursor_col = 0;
  storyterminal_set_gfx_from_cursor (self);
  }


/*======================================================================
  storyterminal_lf
======================================================================*/
void storyterminal_lf (StoryTerminal *self,
       gboolean immediate)
  {
  self->priv->cursor_row++;
  if (self->priv->cursor_row >= self->priv->rows)
    {
    if (self->priv->auto_scroll)
      storyterminal_scroll_up (self, immediate);
    self->priv->cursor_row = self->priv->rows - 1;
    }
  storyterminal_set_gfx_from_cursor (self);
  }


/*======================================================================
  storyterminal_scroll_gfx_area
  Note that the coordinates here are _inclusive_ (unlike those
  used by frotz).
  Note also that 'units' here is pixel units, not screen units
======================================================================*/
void storyterminal_scroll_gfx_area (StoryTerminal *self, 
    int x, int y, int w, int h, int units, gboolean immediate)
  {
  GtkWidget *widget = GTK_WIDGET (self);
  //int cw, ch;
  //storyterminal_get_char_cell_size_in_pixels (self, &cw, &ch);
  int x1 = x; 
  //int x2 = x + w; 
  int y1 = y; 
  //int y2 = y + h;
  int sw = w;
  int sh = h;

  gdk_draw_drawable (self->priv->graphics_buffer, 
          self->priv->gc,
          self->priv->graphics_buffer, x1, y1 + units, x1, y1, sw, sh);


  if (immediate)
        {
        gdk_draw_drawable (widget->window,
          self->priv->gc,
          self->priv->graphics_buffer, x1, y1 + units, x1, y1, sw, sh);
        }
  else
        storyterminal_mark_dirty (self);

  // Not sure about the +1 here :
  storyterminal_erase_gfx_area (self, x1, y1 + h - units + 1, w, 
    units, immediate);
  }


/*======================================================================
  storyterminal_scroll_area
  Note that the coordinates here are _inclusive_ (unlike those
  used by frotz).
======================================================================*/
void storyterminal_scroll_area (StoryTerminal *self, int top, int left, 
    int bottom, int right, int units, gboolean immediate)
  {
  if (!self->priv->graphics_buffer) return;
  if (top > bottom) return;
  if (left > right) return;
  if (top < 0) top = 0;
  if (left < 0) left = 0;
  if (bottom >= self->priv->rows) bottom = self->priv->rows - 1;
  if (right >= self->priv->cols) right = self->priv->cols - 1;
  if (top > bottom) return;
  if (left > right) return;

  int i;
  for (i = 0; i < units; i++)
    {
      GtkWidget *w = GTK_WIDGET (self);
      int cw, ch;
      storyterminal_get_char_cell_size_in_pixels (self, &cw, &ch);
      int x1 = left * cw;
      int x2 = (right + 1) * cw; 
      int y1 = top * ch;
      int y2 = bottom * ch;
      int sw = x2 - x1;
      int sh = y2 - y1;
      
      gdk_draw_drawable (self->priv->graphics_buffer, 
          self->priv->gc,
          self->priv->graphics_buffer, x1, y1 + ch, x1, y1, sw, sh);
      if (immediate)
        {
        gdk_draw_drawable (w->window,
          self->priv->gc,
          self->priv->graphics_buffer, x1, y1 + ch, x1, y1, sw, sh);
        }
      else
        storyterminal_mark_dirty (self);
      
      storyterminal_erase_area (self, bottom - units + 1, left, 
        bottom, right, immediate); 
    }
  }



/*======================================================================
  storyterminal_scroll_up
  Scroll up whole screen
======================================================================*/
void storyterminal_scroll_up (StoryTerminal *self,
       gboolean immediate)
  {
  storyterminal_scroll_area (self, 0, 0, 
    self->priv->rows, self->priv->cols, 1, immediate);
  }


/*======================================================================
  storyterminal_advance_unit_cursor
  moves the current unit-based cursor, without moving the 
  graphics cursor. Some attention need to be given to what happens
  if the unit cursor wraps onto a new line, while the gfx cursor
  still fits onto the old line. 
======================================================================*/
void storyterminal_advance_unit_cursor (StoryTerminal *self, 
    gboolean immediate)
{
  self->priv->cursor_col++;
  if (self->priv->cursor_col >= self->priv->cols)
  {
    self->priv->cursor_col = 0;
    self->priv->cursor_row++;
    if (self->priv->cursor_row >= self->priv->rows)
    {
      self->priv->cursor_row = self->priv->rows - 1;
      if (self->priv->auto_scroll)
        storyterminal_scroll_up (self, immediate); 
    }
  }
}


/*======================================================================
  storyterminal_write_char
======================================================================*/
void storyterminal_write_char (StoryTerminal *self, gunichar2 c, 
    gboolean immediate)
  { 
  //printf ("write char %d at %d %d\n", (int)c, 
  // self->cursor_row, self->cursor_col);
  switch (c)
  {
  case 8:
    storyterminal_backspace (self, TRUE, immediate);
    break;

  case 13:
    storyterminal_cr (self, immediate); // TODO -- perhaps decouple CR from LF?
    storyterminal_lf (self, immediate);
    break;

  // TODO -- do we need to handle other control chars? Escape? LF?

  default:
    {
    int move_x = 0;
   
    storyterminal_write_char_at_gfx (self, 
                self->priv->gfx_x, self->priv->gfx_y, c, FALSE, &move_x);
    
    // We need to move the text cursor to its current unit position,
    //  even though the text itself may be somewhere else if 
    //  variable pitch font is in use. This is because the game
    //  might ask for a 'more...' prompt, and we need to be able
    //  to put the cursor back in a sensible place after erasing
    //  the prompt
    //storyterminal_advance_unit_cursor (self, immediate);
    self->priv->gfx_x += move_x;
    // What about screen wrapping? Do we need to handle it here?

    storyterminal_mark_dirty (self);

    if (g_unichar_isalpha (c))
      self->priv->last_letter_output = c;
    }
  }
  }


/*======================================================================
  storyterminal_set_auto_scroll
======================================================================*/
void storyterminal_set_auto_scroll (StoryTerminal *self, gboolean f)
  {
  self->priv->auto_scroll = f;
  }


/*======================================================================
  storyterminal_refresh_timeout
=====================================================================*/
gboolean storyterminal_timeout (StoryTerminal *self)
{
  if (GTK_WIDGET (self)->window == NULL) return FALSE;
  if (self->dispose_has_run) return FALSE;

  if (self->priv->dirty)
    gtk_widget_queue_draw (GTK_WIDGET (self));

  return TRUE;
}


/*======================================================================
  storyterminal_get_default_fg_colour
=====================================================================*/
RGB8COLOUR storyterminal_get_default_fg_colour (const StoryTerminal *self)
{
  return self->priv->default_fg_colour;
}


/*======================================================================
  storyterminal_get_default_bg_colour
=====================================================================*/
RGB8COLOUR storyterminal_get_default_bg_colour (const StoryTerminal *self)
{
  return self->priv->default_bg_colour;
}


/*======================================================================
  storyterminal_get_default_bg_colour
=====================================================================*/
void storyterminal_set_text_style (StoryTerminal *self, 
   STStyle text_style)
{
  self->priv->text_style = text_style;
}


/*======================================================================
  storyterminal_set_fg_colour
=====================================================================*/
void storyterminal_set_fg_colour (StoryTerminal *self, 
    RGB8COLOUR colour)
  {
    if (colour == RGB8DEFAULT)
      self->priv->fg_colour = self->priv->default_fg_colour;
    else if (!storyterminal_get_ignore_game_colours (self))
      self->priv->fg_colour = colour;
  }


/*======================================================================
  storyterminal_get_fg_colour
=====================================================================*/
RGB8COLOUR storyterminal_get_fg_colour (const StoryTerminal *self)
  {
  return self->priv->fg_colour;
  } 


/*======================================================================
  storyterminal_get_ignore_game_colours
=====================================================================*/
gboolean storyterminal_get_ignore_game_colours (const StoryTerminal *self)
  {
  return mainwindow_get_ignore_game_colours (self->priv->main_window);
  }

/*======================================================================
  storyterminal_set_bg_colour
=====================================================================*/
RGB8COLOUR storyterminal_get_bg_colour (const StoryTerminal *self)
  {
  return self->priv->bg_colour;
  } 


/*======================================================================
  storyterminal_set_bg_colour
=====================================================================*/
void storyterminal_set_bg_colour (StoryTerminal *self, 
    RGB8COLOUR colour)
  {
    if (colour == RGB8DEFAULT)
      self->priv->bg_colour = self->priv->default_bg_colour;
    else if (!storyterminal_get_ignore_game_colours (self))
      self->priv->bg_colour = colour;
  }


/*======================================================================
  storyterminal_set_font_code
=====================================================================*/
void storyterminal_set_font_code (StoryTerminal *self, 
    STFontCode font_code)
{
  self->priv->font_code = font_code;
}


/*======================================================================
  storyterminal_peek_colour
=====================================================================*/
RGB8COLOUR storyterminal_peek_colour_at_gfx (const StoryTerminal *self,
   int x, int y)
{
  GdkPixbuf *pb = gdk_pixbuf_get_from_drawable
    (NULL, self->priv->graphics_buffer, gdk_colormap_get_system(),  
       x, y, 0, 0, 1, 1);

  guchar *p = gdk_pixbuf_get_pixels (pb);

  RGB8COLOUR rgb8 = MAKE_RGB8COLOUR (p[0], p[1], p[2]);
  
  g_object_unref (pb);
 
  return rgb8;
}


/*======================================================================
  storyterminal_peek_colour
=====================================================================*/
RGB8COLOUR storyterminal_peek_colour_under_gfx_cursor 
   (const StoryTerminal *self)
{
  return storyterminal_peek_colour_at_gfx (self, self->priv->gfx_x, 
    self->priv->gfx_y);
}


/*======================================================================
  storyterminal_peek_colour
=====================================================================*/
RGB8COLOUR storyterminal_peek_colour (const StoryTerminal *self)
{
  // It is unclear, as is often the case, where the 'cursor' is
  //  with a variable-pitch display. However, this function will
  //  usually be called directly after a set_cursor, so it might not
  //  make any difference. 
  int char_width, char_height;
  storyterminal_get_char_cell_size_in_pixels 
    (self, &char_width, &char_height);
  int x = self->priv->cursor_col * char_width;
  int y = self->priv->cursor_row * char_height;
  return storyterminal_peek_colour_at_gfx (self, x, y);
}


/*======================================================================
  storyterminal_draw_pixbuf_at_gfx
  at specified unit cursor position
=====================================================================*/
void storyterminal_draw_pixbuf_at_gfx (StoryTerminal *self, GdkPixbuf *pb, 
    int x, int y)
  {
  gdk_draw_pixbuf (self->priv->graphics_buffer, self->priv->gc,
    pb, 0, 0, x, y, -1, -1, 
    GDK_RGB_DITHER_NONE, 0, 0);

  storyterminal_mark_dirty (self);
  }


/*======================================================================
  storyterminal_draw_pixbuf
  at specified unit cursor position
=====================================================================*/
void storyterminal_draw_pixbuf (StoryTerminal *self, GdkPixbuf *pb, 
    int row, int col)
  {
  int char_width, char_height;
  storyterminal_get_char_cell_size_in_pixels (self, &char_width,
    &char_height);
  storyterminal_draw_pixbuf_at_gfx (self, pb, col * char_width, 
    row * (char_height));
  }


/*======================================================================
  storyterminal_get_gfx_pos
=====================================================================*/
void storyterminal_get_gfx_pos (const StoryTerminal *self, int *x, int *y)
  {
  *x = self->priv->gfx_x;
  *y = self->priv->gfx_y;
  }


/*======================================================================
  storyterminal_get_cursor_pos
=====================================================================*/
void storyterminal_get_cursor_pos (const StoryTerminal *self, int *r, int *c)
  {
  *c = self->priv->cursor_col;
  *r = self->priv->cursor_row;
  }


/*======================================================================
  storyterminal_set_gfx_cursor
=====================================================================*/
void storyterminal_set_gfx_cursor (StoryTerminal *self, int x, int y)
  {
  self->priv->gfx_x = x;
  self->priv->gfx_y = y;
  }


/*======================================================================
  storyterminal_get_char_width
  Note that get_string_width is not a sequence of calls to
  get_char_width, because it's quicker to have Pango work out the
  size of a complete string in one go, if the complete string
  is available. Of course, terminal output is often character-by-
  character, so the complete string is often not available
=====================================================================*/
int storyterminal_get_char_width (const StoryTerminal *self, gunichar2 c) 
  {
  PangoContext *pc = gtk_widget_get_pango_context (GTK_WIDGET (self)) ;
  PangoLayout *layout = pango_layout_new (pc);
  char s[10];
  charutils_utf16_char_to_utf8 (c, s, sizeof (s));
  if (self->priv->font_code == STFONT_FIXED || 
      (self->priv->text_style & STSTYLE_FIXED))
    pango_layout_set_font_description (layout, self->priv->pfd_fixed);
  else
    pango_layout_set_font_description (layout, self->priv->pfd_main);
  pango_layout_set_text (layout, s, -1);
  int char_height;
  int char_width;
  pango_layout_get_pixel_size (layout, &char_width, &char_height);
  g_object_unref (layout);

  if (self->priv->text_style & STSTYLE_BOLD) char_width += 1;
  if (self->priv->text_style & STSTYLE_ITALIC) char_width += 1;

  return char_width;
  }


/*======================================================================
  storyterminal_get_string_width
=====================================================================*/
int storyterminal_get_string_width (const StoryTerminal *self, const char *s) 
  {
  PangoContext *pc = gtk_widget_get_pango_context (GTK_WIDGET (self)) ;
  PangoLayout *layout = pango_layout_new (pc);
  pango_layout_set_text (layout, s, -1);

  //TODO: ideally we should work this out with text styles applied
  if (self->priv->font_code == STFONT_FIXED || 
      (self->priv->text_style & STSTYLE_FIXED))
    pango_layout_set_font_description (layout, self->priv->pfd_fixed);
  else
    pango_layout_set_font_description (layout, self->priv->pfd_main);

  int char_height;
  int char_width;
  pango_layout_get_pixel_size (layout, &char_width, &char_height);
  g_object_unref (layout);
  return char_width;
  }


/*======================================================================
  storyterminal_get_text_style
=====================================================================*/
STStyle storyterminal_get_text_style (const StoryTerminal *self)
  {
  return self->priv->text_style;
  }


/*======================================================================
  storyterminal_get_font_code
=====================================================================*/
STFontCode storyterminal_get_font_code (const StoryTerminal *self)
  {
  return self->priv->font_code;
  }


/*======================================================================
  storyterminal_get_font_code
=====================================================================*/
void storyterminal_get_gfx_cursor_pos (const StoryTerminal *self, 
    int *x, int *y)
  {
  *x = self->priv->gfx_x;
  *y = self->priv->gfx_y;
  }

/*======================================================================
  storyterminal_get_last_char_output
=====================================================================*/
gunichar2 storyterminal_get_last_letter_output (const StoryTerminal *self)
  {
  return self->priv->last_letter_output;
  }


/*======================================================================
  storyterminal_add_to_input_buffer_utf16
=====================================================================*/
void storyterminal_add_to_input_buffer_utf16 (StoryTerminal *self,
    const gunichar2 *s)
  {
  STInput input;
  const gunichar2 *ss = s;
  while (*ss)
    {
    gunichar2 c = *ss;
    input.type = ST_INPUT_KEY;
    input.key = c; 
    if (c == 13)
      input.key = GDK_Return;
    storyterminal_add_to_input_buffer (self, &input);
    ss++;
    }
 
  }


/*======================================================================
  storyterminal_add_to_input_buffer_utf8
=====================================================================*/
void storyterminal_add_to_input_buffer_utf8 (StoryTerminal *self,
    const char *s)
  {
  gunichar2 *ss = g_utf8_to_utf16 (s, -1, NULL, NULL, NULL);
  storyterminal_add_to_input_buffer_utf16 (self, ss);
  free (ss);
  }


/*======================================================================
  storyterminal_kill_line_from_cursor
=====================================================================*/
void storyterminal_kill_line_from_cursor (StoryTerminal *self)
  {
  STInput input;
  input.type = ST_INPUT_KILL_LINE_FROM_CURSOR;
  storyterminal_add_to_input_buffer (self, &input);
  }


/*======================================================================
  storyterminal_kill_line
=====================================================================*/
void storyterminal_kill_line (StoryTerminal *self)
  {
  STInput input;
  input.type = ST_INPUT_KILL_LINE;
  storyterminal_add_to_input_buffer (self, &input);
  }


/*======================================================================
  storyterminal_paste_quit
=====================================================================*/
void storyterminal_paste_quit (StoryTerminal *self)
  {
  storyterminal_kill_line (self);
  storyterminal_add_to_input_buffer_utf8 (self, "quit\r");
  }


/*======================================================================
  storyterminal_go_to_next_word
=====================================================================*/
void storyterminal_go_to_next_word (StoryTerminal *self)
  {
  STInput input;
  input.type = ST_INPUT_NEXT_WORD;
  storyterminal_add_to_input_buffer (self, &input);
  }


/*======================================================================
  storyterminal_go_to_prev_word
=====================================================================*/
void storyterminal_go_to_prev_word (StoryTerminal *self)
  {
  STInput input;
  input.type = ST_INPUT_PREV_WORD;
  storyterminal_add_to_input_buffer (self, &input);
  }


/*======================================================================
  storyterminal_set_main_window
=====================================================================*/
void storyterminal_set_main_window (StoryTerminal *self, 
    MainWindow *main_window)
  {
  self->priv->main_window = main_window;
  }


/*======================================================================
  storyterminal_get_main_window
=====================================================================*/
MainWindow *storyterminal_get_main_window (const StoryTerminal *self)
  {
  return (self->priv->main_window);
  }




