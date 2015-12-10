#include <string.h>
#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include "Interpreter.h"
#include "StoryTerminal.h"
#include "StoryReader.h"
#include "charutils.h"
#include "MainWindow.h"

G_DEFINE_TYPE (Interpreter, interpreter, G_TYPE_OBJECT);

typedef struct _InterpreterPriv
{
  StoryTerminal *terminal;
  const StoryReader *story_reader;
  GString *transcript;
  char *temp_dir;
  InterpreterStateChangeCallback state_change_callback;
  void *state_change_callback_data;
  MainWindow *main_window;
} InterpreterPriv;


/*======================================================================
  interpreter_init
=====================================================================*/

static void interpreter_init (Interpreter *this)
{
  g_debug ("Initialising interpreter\n");
  this->dispose_has_run = FALSE;
  this->priv = (InterpreterPriv *) malloc (sizeof (InterpreterPriv));
  memset (this->priv, 0, sizeof (InterpreterPriv));
  this->priv->transcript = g_string_new("");
}


/*======================================================================
  interpreter_dispose
======================================================================*/
static void interpreter_dispose (GObject *_self)
{
  Interpreter *self = (Interpreter *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing interpreter\n");
  if (self->priv->transcript)
  {
    g_string_free (self->priv->transcript, TRUE);
    self->priv->transcript = NULL;
  }
  if (self->priv->temp_dir)
  {
    free (self->priv->temp_dir);
    self->priv->temp_dir = NULL;
  }
  self->dispose_has_run = TRUE;
  if (self->priv->terminal)
  {
    gtk_widget_destroy (GTK_WIDGET (self->priv->terminal));
    self->priv->terminal = NULL;
  }
  if (self->priv)
  {
    free (self->priv);
    self->priv = NULL;
  }
  G_OBJECT_CLASS (interpreter_parent_class)->dispose (_self);
}


/*======================================================================
  interpreter_class_init
======================================================================*/
static void interpreter_class_init
  (InterpreterClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = interpreter_dispose;
}


/*======================================================================
  interpreter_new
======================================================================*/
Interpreter *interpreter_new (void)
{
  Interpreter *this = g_object_new 
   (INTERPRETER_TYPE, NULL);
  return this;
}


/*======================================================================
  interpreter_create_terminal
  creates a suitable terminal
======================================================================*/
StoryTerminal *interpreter_create_terminal (Interpreter *self)
{
  StoryTerminal *st = INTERPRETER_GET_CLASS (self)->interpreter_create_terminal 
    (self); 
  return st;
}

/*======================================================================
  interpreter_get_terminal
  Returns a suitable terminal object, creating it if necessary
======================================================================*/
StoryTerminal *interpreter_get_terminal (Interpreter *self)
{
  if (!self->priv->terminal) 
    {
    self->priv->terminal = INTERPRETER_GET_CLASS (self)->
      interpreter_create_terminal (self); 
    }
  return self->priv->terminal;
}


/*======================================================================
  interpreter_set_story_reader 
======================================================================*/
void interpreter_set_story_reader (Interpreter *self, const StoryReader *sr)
  {
  self->priv->story_reader = sr;
  }


/*======================================================================
  interpreter_get_story_reader 
======================================================================*/
const StoryReader *interpreter_get_story_reader (const Interpreter *self)
  {
  return self->priv->story_reader;
  }


/*======================================================================
  interpreter_run
======================================================================*/
void interpreter_run (Interpreter *self)
  {
  INTERPRETER_GET_CLASS (self)->interpreter_run (self);
  }


/*======================================================================
  interpreter_child_finished
======================================================================*/
void interpreter_child_finished (Interpreter *self)
  {
  INTERPRETER_GET_CLASS (self)->interpreter_child_finished (self);
  }


/*======================================================================
  interpreter_append_to_transcript
======================================================================*/
void interpreter_append_to_transcript (Interpreter *self, const char *s)
  {
  if (!self->priv->transcript) return;
  self->priv->transcript = g_string_append (self->priv->transcript, s);
  }

/*======================================================================
  interpreter_append_utf16_to_transcript
======================================================================*/
void interpreter_append_utf16_to_transcript (Interpreter *self, 
    gunichar2 c)
  {
  if (!self->priv->transcript) return;
  charutils_append_utf16_char_to_utf8_string (c, self->priv->transcript);
  }


/*======================================================================
  interpreter_get_transcript
======================================================================*/
const GString *interpreter_get_transcript (const Interpreter *self)
  {
  return self->priv->transcript;
  }


/*======================================================================
  interpreter_get_temp_dir
======================================================================*/
const char *interpreter_get_temp_dir (const Interpreter *self)
  {
  return self->priv->temp_dir;
  }


/*======================================================================
  interpreter_set_temp_dir
======================================================================*/
void interpreter_set_temp_dir (Interpreter *self, 
     const char *temp_dir)
  {
  self->priv->temp_dir = strdup (temp_dir);
  }


/*======================================================================
  interpreter_set_state_chance_callback
======================================================================*/
void interpreter_set_state_change_callback 
      (Interpreter *self, InterpreterStateChangeCallback iscc, void *user_data)
  {
  self->priv->state_change_callback = iscc;
  self->priv->state_change_callback_data = user_data;
  }


/*======================================================================
  interpreter_call_state_change
  protected method, for use by subclasses. Cause the interpreter to
  exercise the callback that was registered to notify owner of
  state changes
======================================================================*/
void interpreter_call_state_change (Interpreter *self, 
    InterpreterStateChange isc)
  {
  if (self->priv->state_change_callback)
    self->priv->state_change_callback 
      (isc, self->priv->state_change_callback_data);
  }


/*======================================================================
  interpreter_set_main_window
=====================================================================*/
void interpreter_set_main_window (Interpreter *self, 
    MainWindow *main_window)
  {
  self->priv->main_window = main_window;
  }


/*======================================================================
  interpreter_get_main_window
=====================================================================*/
MainWindow *interpreter_get_main_window (const Interpreter *self)
  {
  return (self->priv->main_window);
  }



