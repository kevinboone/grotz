#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "Interpreter.h"
#include "ZTerminal.h"
#include "ZMachine.h"
#include "StoryTerminal.h"
#include "frotz.h"
#include "charutils.h"
#include "StoryReader.h"
#include "Picture.h"
#include "Sound.h"
#include "fileutils.h"
#include "MediaPlayer.h"

G_DEFINE_TYPE (ZMachine, zmachine, INTERPRETER_TYPE);

#define ZM_MAX_COLOURS 512
#define ZM_FIRST_CUSTOM_COLOUR 20 

extern void end_of_sound (zword routine);

typedef struct _ZMachinePriv
{
  char *story_file;
  int user_interpreter_number;
  gboolean user_tandy_bit;
  int speed;
  RGB8COLOUR colours[ZM_MAX_COLOURS];
  int user_random_seed;
  char *current_save_dir;
  int graphics_width;
  int graphics_height;
} ZMachinePriv;

ZMachine *global_zmachine = NULL;

extern int frotz_main (void);
extern void resize_screen (void); // from frotz_screen.c
extern void restart_header (void); // from frotz_screen.c

StoryTerminal *zmachine_create_terminal (Interpreter *self);
void zmachine_run (Interpreter *_self);
void zmachine_child_finished (Interpreter *_self);
static void zmachine_terminal_size_allocate_event (GtkWidget *w, 
    GdkRectangle *a, gpointer data);


/*======================================================================
zmachine_rgb8_to_rgb5
======================================================================*/
RGB8COLOUR zmachine_rgb8_to_rgb5 (int colour)
{
  int r = RGB8_GETRED(colour)>>3;
  int g = RGB8_GETGREEN(colour)>>3;
  int b = RGB8_GETBLUE(colour)>>3;
  return r|(g<<5)|(b<<10);
}


/*======================================================================
zmachine_rgb5_to_rgb8
======================================================================*/
RGB8COLOUR zmachine_rgb5_to_rgb8 (int five)
{
  int r = five&0x001F;
  int g = (five&0x03E0)>>5;
  int b = (five&0x7C00)>>10;
  int rgb8 = MAKE_RGB8COLOUR
    ((r<<3)|(r>>2),(g<<3)|(g>>2),(b<<3)|(b>>2));
  return rgb8;
}


/*======================================================================
zmachine_init_colour_table
======================================================================*/
void zmachine_init_colour_table (ZMachine *self)
{
  int i;
  for (i = 0; i < ZM_MAX_COLOURS; i++)
    {
    // Note -- we use 0 to mean 'unallocated', but it also means
    //  black. So we have to be careful how we search for a free
    //  colour slot
    self->priv->colours[i] = 0;
    }

  // These are the standard z-machine colours
  self->priv->colours[0]  = zmachine_rgb5_to_rgb8(0x0000); // black
  self->priv->colours[1]  = zmachine_rgb5_to_rgb8(0x001D); // red
  self->priv->colours[2]  = zmachine_rgb5_to_rgb8(0x0340); // green
  self->priv->colours[3]  = zmachine_rgb5_to_rgb8(0x03BD); // yellow
  self->priv->colours[4]  = zmachine_rgb5_to_rgb8(0x59A0); // blue
  self->priv->colours[5]  = zmachine_rgb5_to_rgb8(0x7C1F); // magenta
  self->priv->colours[6]  = zmachine_rgb5_to_rgb8(0x77A0); // cyan
  self->priv->colours[7]  = zmachine_rgb5_to_rgb8(0x7FFF); // white
  self->priv->colours[8]  = zmachine_rgb5_to_rgb8(0x5AD6); // light grey
  self->priv->colours[9]  = zmachine_rgb5_to_rgb8(0x4631); // medium grey
  self->priv->colours[10] = zmachine_rgb5_to_rgb8(0x2D6B); // dark grey

  // Leave 200-odd slots free for colours that are peeked out of
  //  images

  //for (i = 0; i < 10; i++)
  //  printf ("color %d is %06x\n", i, self->priv->colours[i]);

}


/*======================================================================
  zmachine_set_user_interpreter_number 
=====================================================================*/
void zmachine_set_user_interpreter_number (ZMachine *self, int number)
  { 
  self->priv->user_interpreter_number = number;
  }


/*======================================================================
  zmachine_set_user_random_seed
=====================================================================*/
void zmachine_set_user_random_seed (ZMachine *self, int number)
  { 
  self->priv->user_random_seed = number;
  }


/*======================================================================
  zmachine_set_user_tandy_bit
=====================================================================*/
void zmachine_set_user_tandy_bit (ZMachine *self, gboolean f)
  { 
  self->priv->user_tandy_bit = f;
  }


/*======================================================================
  zmachine_set_speed
=====================================================================*/
void zmachine_set_speed (ZMachine *self, int number)
  { 
  self->priv->speed = number;
  }


/*======================================================================
  zmachine_set_graphics_size
=====================================================================*/
void zmachine_set_graphics_size (ZMachine *self, int width, int height)
  { 
  self->priv->graphics_width = width;
  self->priv->graphics_height = height;
  }

/*======================================================================
  zmachine_global_terminal
  Convenience function to get the current terminal from the
  global zmachine pointer
=====================================================================*/
StoryTerminal *zmachine_global_terminal (void)
  {
  return interpreter_get_terminal (INTERPRETER (global_zmachine)); 
  }


/*======================================================================
  zmachine_global_story_reader
  Convenience function to get the current storyreader from the
  global zmachine pointer
=====================================================================*/
const StoryReader *zmachine_global_story_reader (void)
  {
  return interpreter_get_story_reader (INTERPRETER (global_zmachine)); 
  }


/*======================================================================
  zmachine_init
=====================================================================*/
static void zmachine_init (ZMachine *this)
{
  g_debug ("Initialising zmachine");
  this->dispose_has_run = FALSE;
  this->priv = (ZMachinePriv *) malloc (sizeof (ZMachinePriv));
  memset (this->priv, 0, sizeof (ZMachinePriv));
}


/*======================================================================
  zmachine_dispose
======================================================================*/

static void zmachine_dispose (GObject *_this)
{
  ZMachine *this = (ZMachine *) _this;
  if (this->dispose_has_run) return;
  g_debug ("Disposing zmachine\n");
  this->dispose_has_run = TRUE;
  if (this->priv->story_file)
  {
    free (this->priv->story_file);
    this->priv->story_file = NULL;
  }
  if (this->priv->current_save_dir)
  {
    free (this->priv->current_save_dir);
    this->priv->current_save_dir = NULL;
  }
  if (this->priv)
  {
    free (this->priv);
    this->priv = NULL;
  }
  G_OBJECT_CLASS (zmachine_parent_class)->dispose (_this);
}


/*======================================================================
  zmachine_class_init
======================================================================*/

static void zmachine_class_init
  (ZMachineClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = zmachine_dispose;
  ((InterpreterClass*)klass)->interpreter_create_terminal
    = zmachine_create_terminal;
  ((InterpreterClass*)klass)->interpreter_run
    = zmachine_run;
  ((InterpreterClass*)klass)->interpreter_child_finished
    = zmachine_child_finished;
  
}


/*======================================================================
  zmachine_new
======================================================================*/

ZMachine *zmachine_new (void)
{
  ZMachine *self = g_object_new 
   (ZMACHINE_TYPE, NULL);
  global_zmachine = self;
  return self;
}


/*======================================================================
  zmachine_new
======================================================================*/
void zmachine_set_story_file (ZMachine *self, const char *story_file)
{
  g_debug ("ZMachine setting story file to %s", story_file);
  self->priv->story_file = strdup (story_file);
}


/*======================================================================
  zmachine_create_terminal
======================================================================*/
StoryTerminal *zmachine_create_terminal (Interpreter *self)
{
  ZTerminal *zt = zterminal_new ();
  storyterminal_set_main_window (STORYTERMINAL (zt), 
    interpreter_get_main_window(self));
  g_signal_connect (G_OBJECT(zt), "size-allocate",
     G_CALLBACK (zmachine_terminal_size_allocate_event), self);       
  return STORYTERMINAL (zt);
}


/*======================================================================
  interpreter_terminal_size_allocate_event
=====================================================================*/
static void zmachine_terminal_size_allocate_event (GtkWidget *w, 
    GdkRectangle *a, gpointer user_data)
{
  g_debug 
      ("ZMachine terminal size change notification");

  int fx, fy, cx, cy;
  StoryTerminal *terminal = zmachine_global_terminal (); 
  storyterminal_get_char_cell_size_in_pixels (terminal, &fx, &fy);
  storyterminal_get_widget_size (terminal, &cx, &cy);

  fx = storyterminal_get_char_width (terminal, '0');

  if (h_font_width != fx || h_font_height != fy 
     || h_screen_rows != (char) (cy / fy)
     || h_screen_cols != (char) (cx / fx)
     || h_screen_width != cx
     || h_screen_height != cy)
    {
    h_font_width = fx; 
    h_font_height = fy;
    h_screen_rows = (char) (cy / fy); 
    h_screen_height = cy; 
    h_screen_cols = (char ) (cx / fx);
    h_screen_width = cx; 
    if (h_version == V6)
      h_flags |= REFRESH_FLAG;
    resize_screen();
    restart_header();
    }
  else
    {
    g_debug 
      ("ZMachine size change ignored, because no parameters have changed");
    }
}


/*======================================================================
  os_read_line
======================================================================*/
zword os_read_line (int max, zword *line, int timeout, int width, int continued)
  {
  interpreter_call_state_change (INTERPRETER (global_zmachine),
    ISC_WAIT_FOR_INPUT);
    
  StoryTerminal *_terminal = interpreter_get_terminal 
    (INTERPRETER (global_zmachine)); 
  ZTerminal *terminal = ZTERMINAL (_terminal);
  int gfx_x, gfx_y;
  storyterminal_get_gfx_cursor_pos (STORYTERMINAL(terminal), &gfx_x, &gfx_y);
  g_debug ("os_read_line -- gfx cursor is at x=%d, y=%d. width=%d, max=%d", 
    gfx_x, gfx_y, width, max);

  int mx, my;
  int terminator = zterminal_read_line (terminal, max, 
     line, timeout, width, continued, &mx, &my);
  // TODO terminator;

  if (terminator == ZC_DOUBLE_CLICK || terminator == ZC_SINGLE_CLICK)
    {
    g_debug ("Input terminated by mouse click at %d %d\n", 
       my, my);
    mouse_x = mx;
    mouse_y = my;
    }

  interpreter_call_state_change (INTERPRETER (global_zmachine),
    ISC_INPUT_COMPLETED);

  return terminator;
  }


/*======================================================================
  os_read_key
======================================================================*/
zword os_read_key (int timeout, bool show_cursor)
  {
  StoryTerminal *_terminal = interpreter_get_terminal 
    (INTERPRETER (global_zmachine)); 
  ZTerminal *terminal = ZTERMINAL (_terminal);

  int mx, my;
  zword c = zterminal_read_key (terminal, timeout, show_cursor, 
     &mx, &my);
  // DO terminator;

  if (c == ZC_DOUBLE_CLICK || c == ZC_SINGLE_CLICK)
    {
    g_debug ("Input terminated by mouse click at %d %d\n", 
       my, my);
    mouse_x = mx;
    mouse_y = my;
    }
  return c;
  }


/*======================================================================
do_display_char
convenience wrapper around zterminal_write_char
======================================================================*/
void do_display_char (zword c)
{
  StoryTerminal *terminal = interpreter_get_terminal 
    (INTERPRETER (global_zmachine)); 
  storyterminal_write_char (terminal, (gunichar2) c, FALSE);
}


/*======================================================================
os_display_char
Note that c is a UTF-16 character -- allowable values extend to 0xFFFF
======================================================================*/
void os_display_char (zword c)
{
  //g_debug ("os_display_char %c", c); 
  if (c == ZC_GAP) 
  {
    do_display_char(' '); do_display_char(' ');
  } else if (c == ZC_INDENT) 
  {
    do_display_char(' '); do_display_char(' '); do_display_char(' ');
  }
  else
    do_display_char(c); // TODO -- check printable
  return;
}


/*======================================================================
  zmachine_run
======================================================================*/
void zmachine_run (Interpreter *_self)
  {
  ZMachine *self = ZMACHINE (_self);
  story_name = self->priv->story_file;
  g_debug ("Starting frotz interpreter, file is %s", story_name);
  frotz_main ();
  g_debug ("frotz interpreter finished");
  }


/*======================================================================
  zmachine_child_finished
======================================================================*/
void zmachine_child_finished (Interpreter *_self)
  {
  //ZMachine *self = ZMACHINE (_self);
  g_debug ("child processed finished");
  MediaPlayer *mp = mediaplayer_get_instance();
  mediaplayer_handle_eos (mp);
  }


/*======================================================================
zmachine_lookup_colour
Return the colour index corresponding to the specified rgb8 value.
Create a new entry in the colour table if the rgb8 value has not
previously been encountered
======================================================================*/
int zmachine_lookup_colour (ZMachine *self, RGB8COLOUR rgb8)
{
  int i;
  for (i = 0; i < ZM_MAX_COLOURS; i++)
    {
    if (self->priv->colours[i] == rgb8) 
      {
      if (i <= DARKGREY_COLOUR)
        return i + 2;
      else
        return i;
      }
    }

  g_debug ("In lookup_colour, colour %6x not found", rgb8);

  for (i = ZM_FIRST_CUSTOM_COLOUR /* skip std colours */; 
        i < ZM_MAX_COLOURS; i++)
    {
    if (self->priv->colours[i] == 0)
      {
      g_debug ("Assigning colour %6x to slot %d", rgb8, i);
      self->priv->colours[i] = rgb8;
      return i;
      }
    }

  return 0; // Run out of colour space -- should rarely happen
}



/*======================================================================
os_init_screen
======================================================================*/
void os_init_screen(void)
{
  if (h_version == V3 && global_zmachine->priv->user_tandy_bit)
      h_config |= CONFIG_TANDY;

  if (global_zmachine->priv->user_interpreter_number > 0)
    h_interpreter_number = global_zmachine->priv->user_interpreter_number;
  else 
  {
    h_interpreter_number = h_version == 6 ? INTERP_MSDOS : INTERP_AMIGA;
  }
  h_interpreter_version = 'F';

  g_debug ("ZMachine using interpreter number %d", h_interpreter_number);

 if ((h_version >= V4) && (global_zmachine->priv->speed != 0))
    h_config |= CONFIG_TIMEDINPUT;

  if (h_version == V3) 
  {
    h_config |= CONFIG_SPLITSCREEN;
    h_config |= CONFIG_PROPORTIONAL;
    h_flags &= ~OLD_SOUND_FLAG;
  }

  if (h_version >= V4) 
  {
    h_config |= CONFIG_BOLDFACE;
    h_config |= CONFIG_EMPHASIS;
    h_config |= CONFIG_FIXED;
    h_config |= CONFIG_TIMEDINPUT;
  }

  if (h_version >= V5) 
  {
    h_config |= CONFIG_COLOUR;
    //h_flags &= ~SOUND_FLAG;
  }

  h_screen_height = h_screen_rows;
  h_screen_width = h_screen_cols;
  //int screen_cells = h_screen_rows * h_screen_cols;

  int fx, fy, cx, cy;
  StoryTerminal *terminal = zmachine_global_terminal (); 
  storyterminal_get_char_cell_size_in_pixels (terminal, &fx, &fy);
  storyterminal_get_widget_size (terminal, &cx, &cy);

  fx = storyterminal_get_char_width (terminal, '0');

  h_font_width = fx; 
  h_font_height = fy;
  h_screen_rows = (char) (cy / fy); 
  h_screen_height = cy; 
  h_screen_cols = (char ) (cx / fx);
  h_screen_width = cx; 

g_debug ("h_screen_width=%d", h_screen_width);
g_debug ("h_screen_height=%d", h_screen_height);
g_debug ("h_screen_rows=%d", h_screen_rows);
g_debug ("h_screen_cols=%d", h_screen_cols);
g_debug ("h_font_width=%d", h_font_width);
g_debug ("h_font_height=%d", h_font_height);

  h_default_foreground = 1;
  h_default_background = 1;

  zmachine_init_colour_table (global_zmachine);

  if (h_version >= V5)
    {
    zword mask = 0;
    if (h_version == V6)
      mask |= TRANSPARENT_FLAG;

    hx_flags &= mask;

    StoryTerminal *terminal = zmachine_global_terminal();
    RGB8COLOUR fg = storyterminal_get_default_fg_colour (terminal);
    RGB8COLOUR bg = storyterminal_get_default_bg_colour (terminal);

    hx_fore_colour = zmachine_rgb8_to_rgb5 (fg); 
    hx_back_colour = zmachine_rgb8_to_rgb5 (bg); 
   
    if (h_version == V6)
      {
      int index_fg = zmachine_lookup_colour (global_zmachine, fg);
      int index_bg = zmachine_lookup_colour (global_zmachine, bg);
      h_default_foreground = index_fg;
      h_default_background = index_bg;
      }
    }

  h_config |= CONFIG_SOUND;

  // Start off in fixed width mode. With luck, modern games
  //  with do a set_font to select style 1, which can be
  //  variable width
  os_set_text_style (FIXED_WIDTH_STYLE);
  }


/*======================================================================
os_set_text_style
Note -- it just so happens that VirtualScreen text styles are the
same as ZMachine text styles. If the style flags are changed at
either end, then we will need some mapping in this function
======================================================================*/
void os_set_text_style (int x)
{
  g_debug ("os_set_text_style %d", x);

  StoryTerminal *terminal = zmachine_global_terminal();
  storyterminal_set_text_style (terminal, x);
}


/*======================================================================
os_to_true_colour
======================================================================*/
zword os_to_true_colour (int a) 
  {
  g_debug ("os_to_true_colour %d", a);
  if (a < 0 || a > 255) return 0;
  return zmachine_rgb8_to_rgb5 (global_zmachine->priv->colours[a]);
  }


/*======================================================================
os_from_true_colour
======================================================================*/
int os_from_true_colour (zword a)
  {
  g_debug ("os_from_true_colour %d", a);
  RGB8COLOUR rgb8 = zmachine_rgb5_to_rgb8 (a);
  int index = zmachine_lookup_colour (global_zmachine, rgb8);
  return index;
  }


/*======================================================================
os_buffer_screen
======================================================================*/
int os_buffer_screen (int a)
  {
  g_debug ("os_buffer_screen %d\n", a);
  return 0;
  }


/*======================================================================
os_scrollback_char
======================================================================*/
void os_scrollback_char (zword a)
  {
  //g_debug ("os_scrollback_char %d", (int)a);
  interpreter_append_utf16_to_transcript (INTERPRETER (global_zmachine), a);
  }


/*======================================================================
os_check_unicode
======================================================================*/
int os_check_unicode (int font, zword c)
  {
  g_debug ("os_check_unicode %d %d", font, (int)c);
  if (font == GRAPHICS_FONT)
     return ((c >= 32) && (c < 128)); 
  else
     return 3; // All in, all out. Very optimistic :)
  }


/*======================================================================
os_scrollback_erase
======================================================================*/
void os_scrollback_erase (int a)
  {
  g_debug ("os_scrollback_erase %d", a);
  }


/*======================================================================
os_window_height
======================================================================*/
void  os_window_height (int win, int height)
  {
  g_debug ("os_window height %d %d", win, height);
  if (win == 1 && height > 0)
    {
/*
    StoryTerminal *terminal = zmachine_global_terminal();
    int screen_width, screen_height;
    storyterminal_get_widget_size (terminal, &screen_width, &screen_height);
    RGB8COLOUR old = storyterminal_get_bg_colour (terminal);
    //RGB8COLOUR fg = storyterminal_get_fg_colour (terminal);
    RGB8COLOUR fg = RGB8BLACK; 
    storyterminal_set_bg_colour (terminal, fg);
    storyterminal_erase_gfx_area (terminal,
      0, 0, screen_width,  height, FALSE); 
    storyterminal_set_bg_colour (terminal, old);
*/
    }
  }


/*======================================================================
os_tick
======================================================================*/
void  os_tick (void)
  {
  //g_debug ("os_tick");
  // Perhaps we should invoke the event loop here rather than
  //  in individual routines?

  }


/*======================================================================
os_menu
======================================================================*/
void  os_menu (int a, int b, const zword *c) //FRIG
  {
  g_debug ("os_menu");

  }


/*======================================================================
os_wrap_window
======================================================================*/
int os_wrap_window (int win)
  {
  //g_debug ("os_wrap_window %d", win);
  return 1;
  }


/*======================================================================
os_char_width
======================================================================*/
int os_char_width (zword z)
{
  StoryTerminal *terminal = zmachine_global_terminal (); 
  return storyterminal_get_char_width (terminal, (gunichar2)z); 
}


/*======================================================================
os_set_colour
======================================================================*/
void os_set_colour (int fg_index, int bg_index)
{
  g_debug ("os_set_colour %d %d", fg_index, bg_index);

  StoryTerminal *terminal = zmachine_global_terminal ();

  RGB8COLOUR fg, bg;

  // Standard z-machine colours start at 2. 1 means use
  //  default. 0 means, what? Don't know. Our custom
  //  colours start at 20 or so

  // Strictly it is an error for the app to ask for a transparent
  //  foreground. But we will leave it at black
  if (fg_index < BLACK_COLOUR)
    fg = RGB8DEFAULT; // Use default
  else if (fg_index <= DARKGREY_COLOUR)
    fg = global_zmachine->priv->colours[fg_index - BLACK_COLOUR]; 
  else
    fg = global_zmachine->priv->colours[fg_index]; 

  if (bg_index == TRANSPARENT_COLOUR)
    bg = RGB8TRANSPARENT;
  else if (bg_index < BLACK_COLOUR)
    bg = RGB8DEFAULT; // Use default
  else if (bg_index <= DARKGREY_COLOUR)
    bg = global_zmachine->priv->colours[bg_index - BLACK_COLOUR]; 
  else
    {
    bg = global_zmachine->priv->colours[bg_index]; 
    }


  storyterminal_set_fg_colour (terminal, fg);
  storyterminal_set_bg_colour (terminal, bg);
}


/*======================================================================
os_set_font
======================================================================*/
void os_set_font (int f)
{
  g_debug ("os_set_font %d", f);
  // Does anything actually ever set anything but '1'?
  // Note that setting 1 -- which all games seem to do -- should not
  // _necessarily_ mean we need to select a variable-point font
  // Some games will set the text style and _then_ call set_font
  // set_font should, presumably, not override the font style choice
  StoryTerminal *terminal = zmachine_global_terminal ();
  if (f == 4)
    storyterminal_set_font_code (terminal, STFONT_FIXED);
  else if (f == 3)
    storyterminal_set_font_code (terminal, STFONT_CUSTOM);
  else
    storyterminal_set_font_code (terminal, STFONT_NORMAL);
}


/*======================================================================
os_font_data
======================================================================*/
int os_font_data (int font, int *height, int *width)
{
  g_debug ("os_font_data");

  int fx, fy;
  StoryTerminal *terminal = zmachine_global_terminal (); 
  storyterminal_get_char_cell_size_in_pixels (terminal, &fx, &fy);
  fx = storyterminal_get_char_width (terminal, '0');

  if (font == TEXT_FONT) 
  {
    *height = fy; *width = fx; return 1;
    return 1;
  }
  if (font == FIXED_WIDTH_FONT) 
  {
    *height = fy; *width = fx; return 1;
    return 1;
  }

  if (font == GRAPHICS_FONT) 
  {
    *height = fy; *width = fx; return 1;
    return 1;
  }
  return 0;
}

/*======================================================================
os_peek_colour
======================================================================*/
int os_peek_colour (void) 
{
  StoryTerminal *terminal = zmachine_global_terminal ();

  RGB8COLOUR rgb8 = storyterminal_peek_colour_under_gfx_cursor (terminal);

  int colour_index = zmachine_lookup_colour (global_zmachine, rgb8); 

  g_debug ("os_peek_color RGB=#%6X index=%d", rgb8, colour_index);

  return colour_index; 
}


/*======================================================================
os_scroll_area
======================================================================*/
void os_scroll_area (int top, int left, int bottom, int right, int units)
{
  g_debug ("os_scroll_area %d %d %d %d %d\n", top, left, bottom, right, units);

  StoryTerminal *terminal = zmachine_global_terminal ();
   
  // ZM coordinates are 1-based; ST are 0-based
  storyterminal_scroll_gfx_area
    (terminal, left - 1, top - 1, right - left, bottom - top, units, FALSE); 
}


/*======================================================================
os_picture_data
======================================================================*/
bool os_picture_data (int num, int *height, int *width)
{
  g_debug ("os_picture_data");
  *height = 0; *width = 0;

  const StoryReader *story_reader = zmachine_global_story_reader();
  const StoryTerminal *terminal = zmachine_global_terminal();

  int num_pictures = storyreader_get_num_pictures (story_reader);
  if (num_pictures == 0) return FALSE;

  if (num == 0)
    {
    // Special case -- return num images
    *height = num_pictures; 
    *width = 1;
    return TRUE;
    }
 
  const Picture *p = storyreader_get_picture_by_znum (story_reader, num);
  if (p)
    {
    int screen_width, screen_height;

    storyterminal_get_widget_size (terminal, &screen_width, &screen_height);

    double x_scale = (double) screen_width / 
      (double) global_zmachine->priv->graphics_width;
    double y_scale = (double) screen_height / 
      (double) global_zmachine->priv->graphics_height;

    g_debug ("os_picture_data -- image is %d x %d", p->width, p->height);
  
    *width = p->width * x_scale;
    *height = p->height * y_scale;
    g_debug ("os_picture_data -- pic is %d rows x %d cols", *height, *width);

    return TRUE;
    }

  return FALSE;
}


/*======================================================================
os_erase_area
======================================================================*/
void os_erase_area (int top, int left, int bottom, int right, int win)
{
  g_debug ("os_erase_area %d %d %d %d %d\n", top, left, bottom, right, win);

  StoryTerminal *terminal = zmachine_global_terminal();

//  storyterminal_erase_area (terminal,
//    top - 1, left - 1, bottom - 1, right - 1, FALSE); 

  storyterminal_erase_gfx_area (terminal,
    left - 1, top - 1, right - left,  bottom - top, FALSE); 

  int screen_width, screen_height;
  storyterminal_get_widget_size (terminal, &screen_width, &screen_height);
  
  if (top ==1 && left == 1 && bottom == screen_height && right == screen_width)
    {
    // Clear all screen
    zterminal_margins_to_bg (ZTERMINAL (terminal)); 
    }

}


/*======================================================================
os_draw_picture
======================================================================*/
void os_draw_picture (int num, int row, int col)
{
  g_debug ("os_draw_picture %d at %d,%d", num, row, col);

  const StoryReader *story_reader = zmachine_global_story_reader();
  StoryTerminal *terminal = zmachine_global_terminal();

  const Picture *p = storyreader_get_picture_by_znum (story_reader, num);
  if (p)
    {
    int ww, wh;
    storyterminal_get_widget_size (terminal, &ww, 
      &wh);

    // What to do with scaling? There's no indication in the blorb file or
    // the individual images what size screen they were intended for.
    // Assume 320x200 since that fits the Infocom games. But, really...
    GdkPixbuf *pb = picture_make_pixbuf (p, 1024, 1024); 
    if (pb)
      {
      int img_w = gdk_pixbuf_get_width (pb);
      int img_h = gdk_pixbuf_get_height (pb);
      double x_scale = (double)ww / 
        (double)global_zmachine->priv->graphics_width;
      double y_scale = (double)wh / 
        (double)global_zmachine->priv->graphics_height;
      int new_w = img_w * x_scale;
      int new_h = img_h * y_scale;

      GdkPixbuf *pbs = gdk_pixbuf_scale_simple (pb, 
        new_w, new_h, 
        GDK_INTERP_BILINEAR);    

      storyterminal_draw_pixbuf_at_gfx (terminal, pbs, col - 1, row - 1);
      g_object_unref (pbs); 
      g_object_unref (pb);
      }
    }
}
 

/*======================================================================
os_restart_game
======================================================================*/
void os_restart_game (int stage) 
  {
  // Do we need to do anything here?
  g_debug ("os_restart_game");
  }


/*======================================================================
os_fatal
Bad, bad, bad in a GUI app. Need to remove all calls to this
if possible
======================================================================*/
void os_fatal (const char *s)
{
  StoryTerminal *terminal = zmachine_global_terminal();
  GtkWindow *w = GTK_WINDOW(gtk_widget_get_toplevel (GTK_WIDGET (terminal))); 
  GtkDialog *d = GTK_DIALOG (gtk_message_dialog_new (w,
      GTK_DIALOG_DESTROY_WITH_PARENT, GTK_MESSAGE_ERROR, GTK_BUTTONS_CLOSE,
      "ZMachine error: %s", s));
  gtk_dialog_run (d);
  gtk_widget_destroy (GTK_WIDGET (d));
  
  exit(1);
}


/*======================================================================
os_random_seed
======================================================================*/
int os_random_seed (void)
{
  int r = global_zmachine->priv->user_random_seed;
  if (r == -1)
    /* Use the epoch as seed value */
    return (time(0) & 0x7fff);
  else return r; 
}


/*======================================================================
os_string_width
======================================================================*/
int os_string_width (const zword *s)
{
  // Ideally we ought to account for style changes, etc., in 
  //  calculating the width
  int width = 0;
  zword c;

  StoryTerminal *terminal = zmachine_global_terminal();
  STStyle old_style = storyterminal_get_text_style (terminal);
  STFontCode old_font_code = storyterminal_get_font_code (terminal);

  while ((c = *s++) != 0)
  {
    if (c == ZC_NEW_FONT)
    {
      os_set_font(*s++);
    }
    else if (c == ZC_NEW_STYLE)
    {
      os_set_text_style(*s++);
    }
    else 
    {
      width += os_char_width(c);
    }
  }

  storyterminal_set_text_style (terminal, old_style);
  storyterminal_set_font_code (terminal, old_font_code);

  return width;
}


/*======================================================================
os_set_cursor
======================================================================*/
void os_set_cursor (int row, int col)
{
  g_debug ("os_set_cursor %d %d", row, col);

  StoryTerminal *terminal = zmachine_global_terminal();

  storyterminal_set_gfx_cursor (terminal, col - 1, row - 1);
}


/*======================================================================
os_display_string
======================================================================*/
void os_display_string (const zword *s)
{
#ifdef DEBUG
  GString *ss = charutils_utf16_string_to_utf8 (s, -1);
  g_debug ("os_display_string '%s'", ss->str); 
  g_string_free (ss, TRUE);
#endif

  zword c;
  while ((c = *s++) != 0)
  {
    if (c == ZC_NEW_FONT)
    {
      os_set_font(*s++);
    }
    else if (c == ZC_NEW_STYLE)
    {
      os_set_text_style(*s++);
    }
    else 
     {
     os_display_char (c);
     }
  }
  // Not sure about this
  static int tick = 0;
  if (tick++ % 200 == 0)
    while (gtk_events_pending()) gtk_main_iteration();
}

/*======================================================================
os_more_prompt
======================================================================*/
void os_more_prompt (void)
  {
  g_debug ("os_more_prompt");

  int x, y, new_x, new_y;
  StoryTerminal *terminal = zmachine_global_terminal();
  storyterminal_get_gfx_cursor_pos (terminal, &x, &y);

  gunichar2 *s = g_utf8_to_utf16 ("(more...)", -1, NULL, NULL, NULL); 
  os_display_string (s);
  free (s);
  storyterminal_get_gfx_cursor_pos (terminal, &new_x, &new_y);

  os_read_key (-1, FALSE); // Don't show a caret -- it's ugly

  /*
  s = g_utf8_to_utf16 ("\x08\x08\x08\x08\x08\x08\x08\x08\x08", 
    -1, NULL, NULL, NULL); 
  os_display_string (s);
  free (s);
  */

  int more_length = new_x - x;
  if (more_length > 0)
    {
    int char_width, char_height;
    storyterminal_get_char_cell_size_in_pixels 
      (terminal, &char_width, &char_height);
    storyterminal_erase_gfx_area (terminal,
      x, y, more_length, char_height, TRUE); 
    }

  storyterminal_set_gfx_cursor (terminal, x, y);
  }



/*======================================================================
os_read_mouse
======================================================================*/
zword os_read_mouse (void)
{
  g_debug ("os_read_mouse");
  /* NOT IMPLEMENTED */
  return 0;
}



/*======================================================================
os_read_file_name
======================================================================*/
int os_read_file_name (char *file_name, const char *default_name, int flag)
{
  ZMachine *self = global_zmachine;
  StoryTerminal *terminal = zmachine_global_terminal();
  GtkWindow *toplevel = 
     GTK_WINDOW(gtk_widget_get_toplevel (GTK_WIDGET (terminal))); 
  gboolean is_save = FALSE;
  char *caption = "Open file";

  switch (flag)
    {
    case FILE_RESTORE:
      caption = "Restore state";
      break;
     
    case FILE_SAVE:
      is_save = TRUE;
      caption = "Save state";
      break;
     
    case FILE_SCRIPT:
      caption = "Run script";
      break;
     
    case FILE_PLAYBACK:
      caption = "Play back transcript";
      break;

    case FILE_RECORD:
      caption = "Record transcript";
      is_save = TRUE;
      break;
     
    case FILE_LOAD_AUX:
      break;
     
    case FILE_SAVE_AUX:
      caption = "Save file";
      is_save = TRUE;
      break;
     
    default:;
    }

  GtkFileChooserDialog *d = GTK_FILE_CHOOSER_DIALOG 
    (gtk_file_chooser_dialog_new (caption,
    toplevel,
    is_save ?
      GTK_FILE_CHOOSER_ACTION_SAVE : 
      GTK_FILE_CHOOSER_ACTION_OPEN,
    GTK_STOCK_CANCEL, GTK_RESPONSE_CANCEL,
    GTK_STOCK_OPEN, GTK_RESPONSE_ACCEPT,
    NULL));

  if (self->priv->current_save_dir)
    gtk_file_chooser_set_current_folder (GTK_FILE_CHOOSER (d), 
      self->priv->current_save_dir);

  if (default_name && is_save)
    gtk_file_chooser_set_current_name (GTK_FILE_CHOOSER (d), 
      default_name);

  if (gtk_dialog_run (GTK_DIALOG (d)) == GTK_RESPONSE_ACCEPT)
  {
    char *selected_filename = gtk_file_chooser_get_filename 
      (GTK_FILE_CHOOSER (d));
    if (self->priv->current_save_dir)
    {
      free (self->priv->current_save_dir);
      self->priv->current_save_dir = NULL;
    }
    self->priv->current_save_dir = gtk_file_chooser_get_current_folder 
      (GTK_FILE_CHOOSER (d));

    strncpy (file_name, selected_filename, MAX_FILE_NAME - 1);

    gtk_widget_destroy (GTK_WIDGET(d));
    free (selected_filename);
    return TRUE;
  }

  gtk_widget_destroy (GTK_WIDGET(d));
  return FALSE; 

}


/*======================================================================
 os_path_open
We only need to support absolute paths
======================================================================*/
FILE *os_path_open(const char *name, const char *mode)
{
  FILE *fp;

  if ((fp = fopen(name, mode))) 
    {
    return fp;
    }
  return NULL;
}


/*======================================================================
os_beep
======================================================================*/
void os_beep (int volume)
{
//  g_message ("beep not supported");
}


/*======================================================================
os_prepare_sample
======================================================================*/
void os_prepare_sample (int n) 
  {
  g_debug ("os_prepare sample %d\n", n);
  }


/*======================================================================
os_finish_with_sample
======================================================================*/
void os_finish_with_sample (int n) 
  {
  g_debug ("os_finish_with_sample %d\n", n);
  }


/*======================================================================
zmachine_media_finished 
======================================================================*/
void zmachine_media_notification (MediaPlayerNotificationType type, 
    void *user_data1, int user_data2)
  {
  if (type == MEDIAPLAYER_NOTIFICATION_FINISHED)
    {
    g_debug ("MediaPlayer notified EOS");
    int eos = (int) user_data1;
    end_of_sound ((zword) eos);
    }
  }


/*======================================================================
os_start_sample
======================================================================*/
void os_start_sample (int n, int volume, int repeats, zword eos) 
  {
  g_debug ("os_start_sample %d, vol=%d, repeats=%d, eos=%d", n,
    volume, repeats, (int)eos);

  const StoryReader *story_reader = zmachine_global_story_reader();
  //StoryTerminal *terminal = zmachine_global_terminal();

  const Sound *sound = storyreader_get_sound_by_znum (story_reader, n);
  if (sound)
    {
    double true_volume = 1.0;
    if ((volume == 255) | (volume < 1))
      true_volume = 1.0;
    else
      true_volume = (double) volume / 8.0;

   char *ext = ".aif";
   if (sound->format == SOUND_FORMAT_OGG)
      ext = ".ogg";
   else if (sound->format == SOUND_FORMAT_MOD)
      ext = ".mod";

   char filename[50];
   snprintf (filename, sizeof (filename), "sound%d%s", n, ext);

   const char *temp_dir = 
     interpreter_get_temp_dir (INTERPRETER (global_zmachine));
   GString *path = fileutils_concat_path
     (temp_dir, filename);

    FILE *f = fopen (path->str, "wb");
    if (f)
      {
      fwrite (sound->data, sound->data_len, 1, f);  
      fclose (f);

      MediaPlayer *mp = mediaplayer_get_instance();

      GError *error = NULL;
      mediaplayer_play_file (mp, path->str, true_volume, repeats, 
        zmachine_media_notification, (void *)global_zmachine, eos, &error);
      if (error)
        {
        g_message (error->message);
        g_error_free (error);
        }

      }   
    else
      {
      g_message ("Can't open file '%s' to write sound data", path->str);
      }
    g_string_free (path, TRUE);
    }
  }


/*======================================================================
os_stop_sample
======================================================================*/
void os_stop_sample (int n) 
  {
  g_debug ("os_stop_sample %d\n", n);
  MediaPlayer *mp = mediaplayer_get_instance();
  mediaplayer_stop (mp);
  }


/*======================================================================
os_reset_screen
======================================================================*/
void os_reset_screen (void)
  {
  g_debug ("os_reset_screen");
  StoryTerminal *terminal = zmachine_global_terminal();
  storyterminal_reset (terminal);
  zmachine_init_colour_table (global_zmachine);
  }





