#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <fcntl.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>
#include "StoryReader.h"
#include "Interpreter.h"
#include "fileutils.h"
#include "ZMachine.h"
#include "blorbreader.h"
#include "MetaData.h"
#include "Picture.h"
#include "Sound.h"
#include "Settings.h"
#include "MainWindow.h"

G_DEFINE_TYPE (StoryReader, storyreader, G_TYPE_OBJECT);

GQuark storyreader_error_quark ()
{
  static GQuark error_quark=0;
  if (error_quark == 0)
  {
    error_quark = g_quark_from_string("storyreader-error-quark");
  }
  return error_quark;
}


typedef struct _StoryReaderPriv
{
  MetaData *metadata;
  GList *picture_list;
  GList *sound_list;
  char *story_filename;
  char *title;
  MainWindow *main_window;
} StoryReaderPriv;


/*======================================================================
  storyreader_init
=====================================================================*/
static void storyreader_init (StoryReader *this)
{
  g_debug ("Initialising storyreader\n");
  this->dispose_has_run = FALSE;
  this->priv = (StoryReaderPriv *) malloc (sizeof (StoryReaderPriv));
  memset (this->priv, 0, sizeof (StoryReaderPriv));
  this->priv->metadata = NULL;
}


/*======================================================================
  storyreader_dispose
======================================================================*/
static void storyreader_dispose (GObject *_self)
{
  StoryReader *self = (StoryReader *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing storyreader\n");
  self->dispose_has_run = TRUE;
  storyreader_close (self);
  if (self->priv)
  {
    if (self->priv->metadata)
    {
      g_object_unref (self->priv->metadata);
      self->priv->metadata = NULL;
    }
    free (self->priv);
    self->priv = NULL;
  }
  G_OBJECT_CLASS (storyreader_parent_class)->dispose (_self);
}


/*======================================================================
  storyreader_class_init
======================================================================*/
static void storyreader_class_init
  (StoryReaderClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = storyreader_dispose;
}


/*======================================================================
  storyreader_new
======================================================================*/
StoryReader *storyreader_new (MainWindow *main_window)
{
  StoryReader *this = g_object_new 
   (STORYREADER_TYPE, NULL);
  this->priv->main_window = main_window;
  return this;
}


/*======================================================================
  storyreader_close
======================================================================*/
void storyreader_close (StoryReader *self)
{
  g_debug ("closing storyreader\n");
  int i, l = g_list_length (self->priv->picture_list);
  for (i = 0; i < l; i++)
    {
    GObject *o = g_list_nth_data (self->priv->picture_list, i);
    g_object_unref (o);
    }
  self->priv->picture_list = NULL;
  l = g_list_length (self->priv->sound_list);
  for (i = 0; i < l; i++)
    {
    GObject *o = g_list_nth_data (self->priv->sound_list, i);
    g_object_unref (o);
    }
  self->priv->sound_list = NULL;
  if (self->priv->story_filename)
    {
    free (self->priv->story_filename);
    self->priv->story_filename = NULL;
    }
  if (self->priv->title)
    {
    free (self->priv->title);
    self->priv->title = NULL;
    }
}


/*======================================================================
  storyreader_read_resources_from_blorb
  Do not barf on errors -- don't stop a game just because
  of missing pictures, etc
======================================================================*/
void storyreader_read_resources_from_blorb (StoryReader *self, 
    const char *filename)
{
  g_debug ("Readering resources form blorb '%s'", filename);

  GError *error = NULL;
  self->priv->picture_list = blorbreader_get_pictures (filename, &error);
  if (error)
  {
    g_message (error->message);
    g_error_free (error);
  }
  error = NULL;

  g_debug ("Loaded %d picture(s)", g_list_length (self->priv->picture_list));
  
  self->priv->sound_list = blorbreader_get_sounds (filename, &error);
  if (error)
  {
    g_message (error->message);
    g_error_free (error);
  }
  error = NULL;

  g_debug ("Loaded %d sounds(s)", g_list_length (self->priv->sound_list));
  
  self->priv->metadata = metadata_new_from_file (filename, &error);
  if (error)
  {
    g_message (error->message);
    g_error_free (error);
  }

}


/*======================================================================
  storyreader_open
  We must return an Interpreter object, initialized so that it
  knows where its files, etc., are. 
======================================================================*/
Interpreter *storyreader_open (StoryReader *self, 
    const char *filename, const char *temp_dir, 
    const Settings *settings, GError **error)
  {
  g_message ("Opening file %s", filename);
 
  // We need to do some basic file checks before jumping into the
  //  interpreter, because although the interpreter will detect
  //  a defective file, it does it in such a way that there is no
  //  realistic way to recover control and the program has to be
  //  terminated. 

  struct stat sb;
  if (stat (filename, &sb))
  {
    g_set_error (error, storyreader_error_quark(), -1,
        "Not a valid file: %s", filename);
    return NULL;
  }
  
#ifdef WIN32
  int f = open (filename, O_RDONLY | O_BINARY);
#else
  int f = open (filename, O_RDONLY);
#endif 

  if (f <= 0)
  {
    g_set_error (error, storyreader_error_quark(), -1,
        "Can't open file for reading: %s", filename);
    return NULL;
  }

  close (f);

  // We have a good file of some type. But what type?

  GString *story_filename = NULL;

  GString *ext = fileutils_get_extension (filename);
  if (strcmp (ext->str, "zblorb") == 0 || strcmp (ext->str, "zlb") == 0) 
    {
    // Unpack story file from zblorb

    g_debug ("Unpacking blorb '%s'", filename);

    int data_len;
    unsigned char *data;
    blorbreader_get_first_chunk_of_type (filename, 
      "ZCOD", &data, &data_len, error);
    if (*error)
      {
      return NULL; // Error has been set
      }
  
   // (Beyond this point we must free data)
   g_debug ("Unpacked %d ZCOD bytes, z-code version is %d\n", 
     data_len, data[0]);

   int zcode_version = (int)data[0];

   GString *short_filename = fileutils_get_filename (filename);

   GString *short_fn_without_ext = fileutils_strip_extension 
     (short_filename->str);

   g_string_append_printf (short_fn_without_ext, ".z%d", zcode_version); 
   GString *temp_story_filename = fileutils_concat_path 
     (temp_dir, short_fn_without_ext->str);

   g_debug ("Writing z-code to new file %s\n", temp_story_filename->str);

   FILE *fo = fopen (temp_story_filename->str, "wb");
   if (fo)
     {
     fwrite (data, data_len, 1, fo);
     fclose (fo);
     }
   else
     {
     g_set_error (error, storyreader_error_quark(), -1,
        "Can't write valid file: %s", filename);
     if (data) free (data);
     g_string_free (short_filename, TRUE);
     g_string_free (short_fn_without_ext, TRUE);
     g_string_free (temp_story_filename, TRUE);
     return NULL;
     }

    free (data); // It has been written to file
    g_string_free (short_filename, TRUE);
    g_string_free (short_fn_without_ext, TRUE);
    story_filename = temp_story_filename;

    // Read resources from this file itself

    storyreader_read_resources_from_blorb (self, filename);
    }
  else
    {
    // It's not a blorb. See if there is an associated blorb.
    story_filename = g_string_new (filename);

    GString *blb_file = fileutils_strip_extension 
      (story_filename->str);
    blb_file = g_string_append (blb_file, ".blb");
    // Read resources from associated blorb
    storyreader_read_resources_from_blorb (self, blb_file->str);
    g_string_free (blb_file, TRUE); 
    }

  g_string_free (ext, TRUE); 

  self->priv->story_filename = strdup (story_filename->str);

  if (self->priv->metadata)
    {
    if (self->priv->metadata->title)
      self->priv->title = strdup (self->priv->metadata->title); 
    }

  if (!self->priv->title)
    {
    GString *short_name = fileutils_get_filename 
      (self->priv->story_filename);
    self->priv->title = strdup (short_name->str); 
    g_string_free (short_name, TRUE);
    }


  // TODO handle initialization of other VM types here
  ZMachine *zm = zmachine_new (); 
  interpreter_set_main_window (INTERPRETER (zm), self->priv->main_window);
  zmachine_set_story_file (zm, story_filename->str);
  interpreter_set_story_reader (INTERPRETER (zm), self);
  interpreter_set_temp_dir (INTERPRETER (zm), temp_dir);
  zmachine_set_user_interpreter_number 
    (zm, settings->user_interpreter_number);
  if (settings->user_tandy_bit)
    zmachine_set_user_tandy_bit (zm, TRUE);
  zmachine_set_speed (zm, settings->speed);
  zmachine_set_user_random_seed (zm, settings->user_random_seed);
  zmachine_set_graphics_size (zm, settings->graphics_width,
    settings->graphics_height);

  g_string_free (story_filename, TRUE); 

  return (Interpreter *) zm;
  }


/*======================================================================
  storyreader_get_picture_by_znum
======================================================================*/
const Picture *storyreader_get_picture_by_znum (const StoryReader *self, 
    int znum)
  {
  // TODO check that znum really does reference a picture
  int i, l = g_list_length (self->priv->picture_list);
  for (i = 0; i < l; i++)
    {
    GObject *o = g_list_nth_data (self->priv->picture_list, i);
    Picture *p = (Picture *)o;
    if (p->znum == znum) return p; 
    }
  return NULL;
  }


/*======================================================================
  storyreader_get_sound_by_znum
======================================================================*/
const Sound *storyreader_get_sound_by_znum (const StoryReader *self, 
    int znum)
  {
  // TODO check that znum really does reference a sound 
  int i, l = g_list_length (self->priv->sound_list);
  for (i = 0; i < l; i++)
    {
    GObject *o = g_list_nth_data (self->priv->sound_list, i);
    Sound *p = (Sound *)o;
    if (p->znum == znum) return p; 
    }
  return NULL;
  }


/*======================================================================
  storyreader_show_dialog
======================================================================*/
void storyreader_show_dialog (const StoryReader *self, GtkWindow *w)
  {
   GtkDialog *dialog = GTK_DIALOG (gtk_dialog_new_with_buttons (APPNAME,
        w,
        GTK_DIALOG_DESTROY_WITH_PARENT,
        GTK_STOCK_OK,
        GTK_RESPONSE_NONE,
        NULL));


  GString *s = g_string_new (self->priv->story_filename);
  s = g_string_append (s, "\n");
  if (self->priv->metadata)
    {
    if (self->priv->metadata->fspc_image >= 0)
      {
      const Picture *p = storyreader_get_picture_by_znum 
        (self, self->priv->metadata->fspc_image);
      if (p)
        {
        GdkPixbuf *pb = picture_make_pixbuf (p, 300, 300); 
        GtkImage *image = GTK_IMAGE (gtk_image_new_from_pixbuf (pb));
        g_object_unref (pb);
        gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox),
            GTK_WIDGET (image), FALSE, FALSE, 5);
        }
      else
        {
        g_message ("Image %d referred to in Fspc tag does not exist", 
           self->priv->metadata->fspc_image);
        }
      }

    if (self->priv->metadata->title)
      { 
      s = g_string_append (s, "<b>");
      s = g_string_append (s, self->priv->metadata->title);
      s = g_string_append (s, "</b>");
      s = g_string_append (s, "\n");
      }
    if (self->priv->metadata->author)
      { 
      s = g_string_append (s, self->priv->metadata->author);
      s = g_string_append (s, "\n");
      }
    if (self->priv->metadata->description)
      { 
      s = g_string_append (s, "<i>");
      s = g_string_append (s, self->priv->metadata->description);
      s = g_string_append (s, "</i>");
      s = g_string_append (s, "\n");
      }
    }
  else
    {
    s = g_string_append (s, 
     "(The story file does not provide any author or title information)\n");
    }

  GtkLabel *l = GTK_LABEL (gtk_label_new (NULL));
  gtk_label_set_use_markup (l, TRUE);
  gtk_label_set_markup (l, s->str);
  g_string_free (s, TRUE);
   gtk_box_pack_start (GTK_BOX (GTK_DIALOG(dialog)->vbox),
        GTK_WIDGET (l), FALSE, FALSE, 5);
 
   g_signal_connect_swapped (dialog,
        "response", 
        G_CALLBACK (gtk_widget_destroy),
        dialog);

   gtk_widget_show_all (GTK_WIDGET(dialog));
   gtk_dialog_run (dialog);
  }


/*======================================================================
  storyreader_get_num_pictures
======================================================================*/
int storyreader_get_num_pictures (const StoryReader *self)
  {
  return g_list_length (self->priv->picture_list);
  }


/*======================================================================
  storyreader_get_num_sounds
======================================================================*/
int storyreader_get_num_sounds (const StoryReader *self)
  {
  return g_list_length (self->priv->sound_list);
  }


/*======================================================================
  storyreader_show_dialog
======================================================================*/
const char *storyreader_get_title (const StoryReader *self)
  {
  return self->priv->title;
  }





