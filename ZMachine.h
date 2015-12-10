#pragma once

#include <gtk/gtk.h>
#include "Interpreter.h" 

G_BEGIN_DECLS

#define ZMACHINE_TYPE            (zmachine_get_type ())
#define ZMACHINE(obj)            (G_TYPE_CHECK_INSTANCE_CAST ((obj), ZMACHINE_TYPE, ZMachine))
#define ZMACHINE_CLASS(klass)    (G_TYPE_CHECK_CLASS_CAST ((klass), ZMACHINE_TYPE, ZMachineClass))
#define IS_ZMACHINE(obj)         (G_TYPE_CHECK_INSTANCE_TYPE ((obj), ZMACHINE_TYPE))
#define IS_ZMACHINE_CLASS(klass) (G_TYPE_CHECK_CLASS_TYPE ((klass), ZMACHINE_TYPE))
#define ZMACHINE_GET_CLASS(obj)  (G_TYPE_INSTANCE_GET_CLASS ((obj), ZMACHINE_TYPE, ZMachineClass))

typedef struct _ZMachine ZMachine;
typedef struct _ZMachineClass  ZMachineClass;

struct _ZMachine 
  {
  Interpreter base;
  gboolean dispose_has_run;
  struct _ZMachinePriv *priv;
  };

struct _ZMachineClass
  {
  InterpreterClass parent_class;
  void (* zmachine) (ZMachine *it);
  };

GType zmachine_get_type (void);

ZMachine *zmachine_new (void);

void zmachine_set_story_file (ZMachine *self, const char *story_file);

void zmachine_set_user_interpreter_number (ZMachine *self, int number);
void zmachine_set_user_tandy_bit (ZMachine *self, gboolean f);
void zmachine_set_speed (ZMachine *self, int number);
void zmachine_set_user_random_seed (ZMachine *self, int number);
void zmachine_set_graphics_size (ZMachine *self, int width, int height);


G_END_DECLS




