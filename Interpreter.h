#pragma once

#include <gtk/gtk.h>

G_BEGIN_DECLS

#define INTERPRETER_TYPE            (interpreter_get_type ())
#define INTERPRETER(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), INTERPRETER_TYPE, Interpreter))
#define INTERPRETER_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), INTERPRETER_TYPE, InterpreterClass))
#define IS_INTERPRETER(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), INTERPRETER_TYPE))
#define IS_INTERPRETER_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), INTERPRETER_TYPE))
#define INTERPRETER_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), INTERPRETER_TYPE, InterpreterClass))

struct _MainWindow;
typedef struct _Interpreter Interpreter;
typedef struct _InterpreterClass  InterpreterClass;

typedef enum 
  {
  ISC_WAIT_FOR_INPUT = 0,
  ISC_INPUT_COMPLETED = 1
  } InterpreterStateChange;

typedef void (*InterpreterStateChangeCallback) 
 (InterpreterStateChange state_change, void *user_data);

struct _StoryReader;

struct _Interpreter 
  {
  GObject base;
  gboolean dispose_has_run;
  struct _InterpreterPriv *priv;
  };

struct _InterpreterClass
  {
  GObjectClass parent_class;
  void (* interpreter) (Interpreter *it);
  struct _StoryTerminal *(*interpreter_create_terminal) 
    (struct _Interpreter *self);
  void (*interpreter_run)(struct _Interpreter *self);
  void (*interpreter_child_finished)(struct _Interpreter *self);
  };

GType interpreter_get_type (void);

Interpreter *interpreter_new (void);

struct _StoryTerminal *interpreter_get_terminal (Interpreter *self);

void interpreter_run (Interpreter *self);

void interpreter_child_finished (Interpreter * self);

void interpreter_set_story_reader (Interpreter *self, 
  const struct _StoryReader *sr);

const struct _StoryReader *interpreter_get_story_reader 
  (const Interpreter *self);

const char *interpreter_get_temp_dir (const Interpreter *self);
void interpreter_set_temp_dir (Interpreter *self, const char *temp_dir);

void interpreter_append_to_transcript (Interpreter *self, const char *s);

void interpreter_append_utf16_to_transcript (Interpreter *self, 
  gunichar2 c);

const GString *interpreter_get_transcript (const Interpreter *self);

void interpreter_set_state_change_callback 
      (Interpreter *self, InterpreterStateChangeCallback iscc, void *user_data);

struct _MainWindow *interpreter_get_main_window (const Interpreter *self);

void interpreter_set_main_window (Interpreter *self, 
    struct _MainWindow *main_window);



//Protected
struct _StoryTerminal *interpreter_create_terminal (Interpreter *self);

void interpreter_call_state_change (Interpreter *self, 
    InterpreterStateChange isc);

G_END_DECLS




