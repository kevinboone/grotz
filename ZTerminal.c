#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include <stdlib.h>
#include <gdk/gdkkeysyms.h>
#include "ZTerminal.h"
#include "frotz.h"
#include "charutils.h"
#include "MainWindow.h"

G_DEFINE_TYPE (ZTerminal, zterminal, STORYTERMINAL_TYPE);

typedef struct _ZTerminalPriv
{
  GList *history;
} ZTerminalPriv;

void zterminal_get_custom_glyph (StoryTerminal *self, 
    int c, char *glyph_data);

#include "gfx_font_data.c"

int completion (const gunichar2 *buffer, gunichar2 *result);

/*======================================================================
  zterminal_init
=====================================================================*/
static void zterminal_init (ZTerminal *self)
{
  g_debug ("Initialising zterminal\n");
  self->dispose_has_run = FALSE;
  self->priv = (ZTerminalPriv *) malloc (sizeof (ZTerminalPriv));
  memset (self->priv, 0, sizeof (ZTerminalPriv));
  storyterminal_set_auto_scroll (STORYTERMINAL (self), FALSE);
}


/*======================================================================
  zterminal_clear_history
======================================================================*/
void zterminal_clear_history (ZTerminal *self)
  {
  if (!self->priv->history) return;
  int i, l = g_list_length (self->priv->history);
  for (i = 0; i < l; i++)
  {
    GArray *s = g_list_nth_data (self->priv->history, i);
    g_array_free (s, TRUE);
  }
  g_list_free (self->priv->history); 
  self->priv->history = NULL;
  }

/*======================================================================
  zterminal_dispose
======================================================================*/

static void zterminal_dispose (GObject *_self)
{
  ZTerminal *self = (ZTerminal *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing zterminal\n");
  self->dispose_has_run = TRUE;
  zterminal_clear_history (self);
  if (self->priv)
  {
    free (self->priv);
    self->priv = NULL;
  }
  G_OBJECT_CLASS (zterminal_parent_class)->dispose (_self);
}


/*======================================================================
  zterminal_class_init
======================================================================*/
static void zterminal_class_init
  (ZTerminalClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = zterminal_dispose;
  ((StoryTerminalClass*)klass)->storyterminal_get_custom_glyph
    = zterminal_get_custom_glyph;
}


/*======================================================================
  zterminal_new
======================================================================*/
ZTerminal *zterminal_new (void)
{
  ZTerminal *this = g_object_new 
   (ZTERMINAL_TYPE, NULL);
  return this;
}


/*======================================================================
  zterminal_find_next_word
======================================================================*/
int zterminal_find_next_word (ZTerminal *self, GArray *input_buffer,
     int start_pos)
{
  if (input_buffer->len == 0) return - 1;
  if (start_pos >= input_buffer->len) return -1;

  gunichar2 *p = (gunichar2 *) input_buffer->data;
  p += start_pos;
  while (*p)
    {
    if (*p == ' ') return (p - (gunichar2 *)input_buffer->data + 1);
    p++;
    }

  // No space found -- put at end of input
  return input_buffer->len;
}


/*======================================================================
  zterminal_find_prev_word
======================================================================*/
int zterminal_find_prev_word (ZTerminal *self, GArray *input_buffer,
     int start_pos)
{
  if (input_buffer->len == 0) return - 1;
  if (start_pos == 0) return -1;

  int pos = start_pos - 1;
  gunichar2 *p = (gunichar2 *) input_buffer->data;
  if ((p[pos] == ' ') && (pos >= 1)) pos--;
  while (pos > 0)
    {
    if (p[pos] == ' ') return (pos + 1);
    pos --;
    }

  // No space found put at start of input
  return 0;
}




/*======================================================================
  zterminal_gdk_key_to_zkey (gunichar2 key)
======================================================================*/
zword zterminal_gdk_key_to_zkey (gunichar2 key)
  {
  if (key == GDK_Return)
    return ZC_RETURN;
  if (key == GDK_Down)
    return ZC_ARROW_DOWN;
  if (key == GDK_Up)
    return ZC_ARROW_UP;
  if (key == GDK_Left)
    return ZC_ARROW_LEFT;
  if (key == GDK_Right)
    return ZC_ARROW_RIGHT;
  if (key == GDK_Escape)
    return ZC_ESCAPE;
  if (key == GDK_F1)
    return ZC_FKEY_MIN+0;
  if (key == GDK_F2)
    return ZC_FKEY_MIN+1;
  if (key == GDK_F3)
    return ZC_FKEY_MIN+2;
  if (key == GDK_F4)
    return ZC_FKEY_MIN+3;
  if (key == GDK_F5)
    return ZC_FKEY_MIN+4;
  if (key == GDK_F6)
    return ZC_FKEY_MIN+5;
  if (key == GDK_F7)
    return ZC_FKEY_MIN+6;
  if (key == GDK_F8)
    return ZC_FKEY_MIN+7;
  if (key == GDK_F9)
    return ZC_FKEY_MIN+8;
  if (key == GDK_F10)
    return ZC_FKEY_MIN+9;
  if (key == GDK_F11)
    return ZC_FKEY_MIN+10;
  if (key == GDK_F12)
    return ZC_FKEY_MIN+11;
  if (key == GDK_BackSpace)
    return ZC_BACKSPACE;
  int unic = gdk_keyval_to_unicode (key);
  return unic;
  }


/*======================================================================
  zterminal_is_terminator
======================================================================*/
gboolean zterminal_is_terminator (zword zkey)
  {
  if (zkey == ZC_RETURN) return TRUE;
  if (zkey == ZC_SINGLE_CLICK) return TRUE;
  if (zkey == ZC_DOUBLE_CLICK) return TRUE;
  if (zkey >= ZC_FKEY_MIN && zkey <= ZC_FKEY_MAX) return TRUE;
  return FALSE; //TODO
  }

/*======================================================================
  zterminal_copy_array
  Copies the array and contents. In this class, none of the array
  contents are pointers, so the copy can be shallow
======================================================================*/
GArray *zterminal_copy_array (const GArray *s)
{
  GArray *a = g_array_new (TRUE, TRUE, sizeof (gunichar2));
  int i, l = s->len;
  for (i = 0; i < l; i++)
  {
    gunichar2 c = g_array_index (s, gunichar2, i); 
    g_array_append_val (a, c);
  }
  return a;
}


/*======================================================================
  zterminal_strcmp 
======================================================================*/
int zterminal_strcmp (const GArray *a, const GArray *b)
{
  if (a->len != b->len) return 1;
  return memcmp (a->data, b->data, a->len * sizeof (gunichar2));
}


/*======================================================================
  vslinereader_is_in_history
======================================================================*/
gboolean zterminal_is_in_history (ZTerminal *self, const GArray *s)
{
  int i, l = g_list_length (self->priv->history);
  for (i = 0; i < l; i++)
  {
    GArray *ss = g_list_nth_data (self->priv->history, i);
    if (zterminal_strcmp (ss, s) == 0) return TRUE;
  }
  return FALSE;
}


/*======================================================================
  vslinereader_find_in_history
======================================================================*/
int zterminal_find_in_history (ZTerminal *self, const GArray *s)
{
  int i, l = g_list_length (self->priv->history);
  for (i = 0; i < l; i++)
  {
    GArray *ss = g_list_nth_data (self->priv->history, i);
    if (zterminal_strcmp (ss, s) == 0) return i;
  }
  return -1;
}



/*======================================================================
  zterminal_add_to_history
======================================================================*/
void zterminal_add_to_history (ZTerminal *self, GArray *array)
  {
  int p = zterminal_find_in_history (self, array);
  if (p > 0)
     {
     GArray *a = g_list_nth_data (self->priv->history, p);
     self->priv->history = g_list_remove (self->priv->history, a);
     g_array_free (a, FALSE);
     }

  GArray *a = zterminal_copy_array (array);
  self->priv->history = g_list_append (self->priv->history, a);
  }


/*======================================================================
  zterminal_read_key
======================================================================*/
zword zterminal_read_key (ZTerminal *self, int timeout, gboolean show_cursor, 
     int *mouse_x, int *mouse_y)
  {
  STInput input;
  // We have to look here because the interpreter will only be able
  // to process valid keys, and st_wait_for_input will return on any
  // kind of key
  while (TRUE)
    {
    storyterminal_wait_for_input (STORYTERMINAL(self), &input, 
     show_cursor, show_cursor, timeout * 100); // ZM timeouts are in tenths

    if (input.type == ST_INPUT_TIMEOUT)
      {
      return ZC_TIME_OUT; 
      }
    if (input.type == ST_INPUT_KEY)
      {
      zword c = zterminal_gdk_key_to_zkey (input.key);
      if (c) return c;
      }
    if (input.type == ST_INPUT_SINGLE_CLICK)
      {
      *mouse_x = input.mouse_x + 1;
      *mouse_y = input.mouse_y + 1;
      return ZC_SINGLE_CLICK;
      }
    if (input.type == ST_INPUT_DOUBLE_CLICK)
      {
      *mouse_x = input.mouse_x + 1;
      *mouse_y = input.mouse_y + 1;
      return ZC_DOUBLE_CLICK;
      }
    }
  }


/*======================================================================
  zterminal_read_line
======================================================================*/
gunichar2 zterminal_read_line (ZTerminal *self, int max, 
     gunichar2 *line, int timeout, 
     int width, gboolean continued, int *mouse_x, int *mouse_y)
  {
  //storyterminal_set_gfx_from_cursor (STORYTERMINAL(self));

  STInput input;
  gunichar2 zc;
  int input_pos = 0;

  // history_pos is the index of the string that will be inserted
  // if we press the 'up' key
  // On entry this will be the last string entered, so pos might
  // be -1 if the history list is empty
  // On successive presses of 'up', history_pos will eventually
  // end up as -1
  int history_pos = g_list_length (self->priv->history) - 1;

  gunichar2 null[1];
  null[0] = 0;
  storyterminal_write_input_line (STORYTERMINAL (self),
        null, 0, max, width, TRUE);

  GArray *input_buffer = g_array_new (TRUE, TRUE, sizeof (gunichar2));

  gunichar2 *pp = line;
  int ppl = 0;
  while (*pp)
    {
    g_array_append_val (input_buffer, *pp);
    pp++;
    ppl++;
    } 

  // The Z app may 'pre-charge' the input buffer. It seems that in such
  //  cases, the app will print the buffer contents before calling this
  //  routine. That means that we need to back up the gfx cursor by the
  //  size of text in the buffer, so that when we dump the line, we are
  //  in the right place

  // As if that wasn't stupid enough, some games actually prechard the
  // input buffer with lower-case text while writing upper case on the 
  // screen. That doesn't necessarily cause a huge problem with fixed-pitch
  // fonts, but it's a nightmare with variable pitch, because the upper
  // and lower case text is different lengths. There seems to be no
  // straightforward fix for this other than to look at the case of the
  // last output letter, and 'adjust' the input buffer to match it if it
  // sees to be wrong. But, really, this is a very stupid situation to
  // get into in the first place

  input_pos = ppl;

  gboolean last_char_was_upper = 
    g_unichar_isupper ((gunichar) 
      storyterminal_get_last_letter_output (STORYTERMINAL(self)));
  if (input_pos > 0 && last_char_was_upper)
    {
    int i;
    g_debug ("Recasing input buffer to work around stupid zmachine bug");
    for (i = 0; i < input_buffer->len; i++)
      {
      gunichar2 *p = (gunichar2 *) input_buffer->data;
      gunichar2 c = p[i];
      p[i] = (gunichar2) g_unichar_toupper ((gunichar) c); 
      }
    }

  GString *preinput = charutils_utf16_string_to_utf8 
    ((gunichar2 *)input_buffer->data, input_buffer->len);
  g_debug ("On entry to zterminal_read_line, input buffer contains '%s'",
    preinput->str);

  int piwidth = storyterminal_get_string_width (STORYTERMINAL(self), 
    preinput->str);
  g_string_free (preinput, TRUE);
  int gfx_x, gfx_y;
  storyterminal_get_gfx_pos (STORYTERMINAL(self), &gfx_x, &gfx_y); 
  storyterminal_set_gfx_cursor (STORYTERMINAL(self), gfx_x - piwidth, gfx_y); 

  do 
    {
/*
gunichar2 comp_out[200];
int comp_result = completion ((gunichar2*)input_buffer->data, comp_out);
printf ("c=%d\n", comp_result);
*/

    //Note show_cusor param FALSE here because we are doing our
    // own caret drawing
    storyterminal_wait_for_input (STORYTERMINAL(self), &input, TRUE, 
      FALSE, timeout * 100); // Z-code timeouts are in tenths
    gboolean redraw = FALSE;
    if (input.type == ST_INPUT_TIMEOUT)
      {
      return ZC_TIME_OUT;
      }
    else if (input.type == ST_INPUT_KILL_LINE_FROM_CURSOR)
      {
      int l = input_buffer->len;
      if (l > 0 && input_pos < l)
        { 
        g_array_remove_range (input_buffer, input_pos, l - input_pos);
        redraw = TRUE;
        }
      }
    else if (input.type == ST_INPUT_KILL_LINE)
      {
      int l = input_buffer->len;
      if (l > 0)
        { 
        g_array_remove_range (input_buffer, 0, l);
        input_pos = 0;
        redraw = TRUE;
        }
      }
    else if (input.type == ST_INPUT_NEXT_WORD)
      {
      int p = zterminal_find_next_word (self, input_buffer, input_pos); 
      if (p >= 0)
        { 
        input_pos = p;
        redraw = TRUE;
        }
      }
    else if (input.type == ST_INPUT_PREV_WORD)
      {
      int p = zterminal_find_prev_word (self, input_buffer, input_pos); 
      if (p >= 0)
        { 
        input_pos = p;
        redraw = TRUE;
        }
      }
    else if (input.type == ST_INPUT_KEY)
      {
      //printf ("got char %d\n", zc);

      switch (input.key)
        {
        case GDK_BackSpace:
           if (input_pos > 0)
             {
             g_array_remove_index (input_buffer, 
               input_pos - 1);
             input_pos--;
             redraw = TRUE;
             }
           break;

        case GDK_Delete:
           if (input_pos < input_buffer->len)
             {
             g_array_remove_index (input_buffer, 
               input_pos);
             redraw = TRUE;
             }
           break;

        case GDK_Left:
          if (input_pos > 0) 
            {
            input_pos--;
            redraw = TRUE;
            }
          break;

        case GDK_Right:
          if (input_pos < input_buffer->len) 
            {
            input_pos++;
            redraw = TRUE;
            }
          break;

        case GDK_Home:
          if (input_pos > 0) 
            {
            input_pos = 0;
            redraw = TRUE;
            }
          break;

        case GDK_Up:
          if (g_list_length (self->priv->history) == 0) break;
          if (history_pos < 0) break;
          //if (history_pos >= g_list_length (self->priv->history)) break;
          const GArray *s = g_list_nth_data 
              (self->priv->history, history_pos);
          history_pos--;
          g_array_free (input_buffer, TRUE);
          input_buffer = zterminal_copy_array (s); 
          input_pos = input_buffer->len;
          redraw = TRUE;
          break;

        case GDK_Down:
          if (g_list_length (self->priv->history) == 0) break;
          if (history_pos >= (int)g_list_length (self->priv->history)) break; 
          if (history_pos < 0) history_pos = 0;
          history_pos+=1;
          if (history_pos >= g_list_length (self->priv->history))
            {
            GArray *null = g_array_new (TRUE, TRUE, sizeof (gunichar2));
            g_array_free (input_buffer, TRUE);
            input_buffer = zterminal_copy_array (null); 
            input_pos = 0;
            g_array_free (null, TRUE);
            redraw = TRUE;
            history_pos = g_list_length (self->priv->history) - 1;
            } 
          else
            {
            const GArray *s = g_list_nth_data (self->priv->history, 
              history_pos);
            g_array_free (input_buffer, TRUE);
            input_buffer = zterminal_copy_array (s); 
            input_pos = input_buffer->len;
            redraw = TRUE;
            }

          break;

        case GDK_End:
          if (input_pos < input_buffer->len) 
            {
            input_pos = input_buffer->len;
            redraw = TRUE;
            }
          break;

        default:
          zc = zterminal_gdk_key_to_zkey (input.key);
          if (!zterminal_is_terminator (zc) && zc != 0 && zc != 27)
            {
            if (input_buffer->len < max - 1)
              {
              g_array_insert_val (input_buffer, input_pos, zc);
              input_pos++;
              redraw = TRUE;
              }
            } 
        }
      }
    else if (input.type == ST_INPUT_SINGLE_CLICK)
      {
        *mouse_x = input.mouse_x + 1;
        *mouse_y = input.mouse_y + 1;
        zc = ZC_SINGLE_CLICK;
      }
    else if (input.type == ST_INPUT_DOUBLE_CLICK)
      {
        *mouse_x = input.mouse_x + 1;
        *mouse_y = input.mouse_y + 1;
        zc = ZC_DOUBLE_CLICK;
      }
    if (redraw)
      {
      storyterminal_write_input_line (STORYTERMINAL (self),
        (gunichar2 *)input_buffer->data, input_pos, max, width, TRUE);
      }
    } while (!zterminal_is_terminator (zc));

  // It's not clear what we should do to the input buffer if input
  // is terminated by a mouse click
  //if (zc == ZC_RETURN)
  if (TRUE)
    {
    // Add to history
    zterminal_add_to_history (self, input_buffer);

    // Copy the null terminator
    memcpy (line, input_buffer->data, (input_buffer->len + 1) * 2);

    storyterminal_write_input_line (STORYTERMINAL (self),
        line, 0, max, width, FALSE);

    // In order for function keys to work, on exit we have to
    //  leave the gfx cursor at the end of the line
    GString *postinput = charutils_utf16_string_to_utf8 
      ((gunichar2 *)input_buffer->data, input_buffer->len);
    int powidth = storyterminal_get_string_width (STORYTERMINAL(self), 
      postinput->str);
    g_string_free (postinput, TRUE);
    int gfx_x, gfx_y;
    storyterminal_get_gfx_pos (STORYTERMINAL(self), &gfx_x, &gfx_y); 
    storyterminal_set_gfx_cursor (STORYTERMINAL(self), gfx_x + powidth, gfx_y); 
    }
  else
    {
    // Erase the line
    storyterminal_write_input_line (STORYTERMINAL (self),
        null, 0, max, width, FALSE);
    }

  g_array_free (input_buffer, TRUE);

  
  return zc;
  }


/*======================================================================
  zterminal_read_line
======================================================================*/
void zterminal_get_custom_glyph (StoryTerminal *self, 
    int c, char *glyph_data)
  {
  memset (glyph_data, 0, ST_CUSTOM_GLYPH_SIZE);
  if (c >= 32 && c < 128)
    {
    int index = c - 32;
    // Data is arranged in groups of 4 chars. Each group
    // occupies 4 * 8 * 8 = 128 bytes
    int block = index / 4;
    int block_offset = block * 8 * 32;
    int position_in_block = c % 4;
    int offset_in_table = block_offset + position_in_block * 8;

    int i;
    for (i = 0; i < ST_CUSTOM_GLYPH_SIZE; i++)
       {
       int col = i % 8;
       int row = (i / 8);
       
       char dot = fontdata[col + offset_in_table + row * 32];
       if (dot == '#') glyph_data[i] = 1;
       }
    } 
  }

/*======================================================================
  storyterminal_margins_to_bg
=====================================================================*/
void zterminal_margins_to_bg (ZTerminal *self)
  {
  StoryTerminal *self_ = STORYTERMINAL (self);
  RGB8COLOUR bg_colour = storyterminal_get_bg_colour (self_);
  mainwindow_set_gdk_background_colour 
    (storyterminal_get_main_window (STORYTERMINAL(self)), bg_colour);
  }


