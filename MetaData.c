#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include "MetaData.h"
#include "blorbreader.h"
//#include "dialogs.h"

G_DEFINE_TYPE (MetaData, metadata, G_TYPE_OBJECT);


/*======================================================================
  metadata_init
=====================================================================*/

static void metadata_init (MetaData *self)
{
  g_debug ("Initialising metadata\n");
  self->dispose_has_run = FALSE;
}


/*======================================================================
  metadata_dispose
======================================================================*/

static void metadata_dispose (GObject *_self)
{
  MetaData *self = (MetaData *) _self;
  if (self->dispose_has_run) return;
  g_debug ("Disposing metadata\n");
  self->dispose_has_run = TRUE;
  if (self->author)
  {
    free (self->author);
    self->author = NULL;
  }
  if (self->title)
  {
    free (self->title);
    self->title = NULL;
  }
  if (self->description)
  {
    free (self->description);
    self->description = NULL;
  }
  G_OBJECT_CLASS (metadata_parent_class)->dispose (_self);
}


/*======================================================================
  metadata_class_init
======================================================================*/
static void metadata_class_init
  (MetaDataClass *klass)
{
  GObjectClass *baseClass = G_OBJECT_CLASS (klass);
  baseClass->dispose = metadata_dispose;
}


/*======================================================================
  metadata_find_xml_element
  returns NULL if not found. Caller must free return if non-NULL
======================================================================*/
char *metadata_find_xml_element (const char *xml, char *element)
{
  GString *start_tag = g_string_new ("");
  GString *end_tag = g_string_new ("");

  g_string_printf (start_tag, "<%s>", element);
  g_string_printf (end_tag, "</%s>", element);

  char *ret = NULL;

  char *p = strstr (xml, start_tag->str);
  if (p) 
  {
    p += strlen (start_tag->str); 
    char *q = strstr (p, end_tag->str);
    if (q)
    {
      ret = strdup (p);
      ret[q - p] = 0;
    }
  }

  g_string_free (start_tag, TRUE);
  g_string_free (end_tag, TRUE);

  return ret;
}


/*======================================================================
  metadata_parse_ifindex
======================================================================*/
void metadata_parse_ifindex (MetaData *self, char *s)
{
  self->author = metadata_find_xml_element (s, "author");
  self->description = metadata_find_xml_element (s, "headline");
  self->title = metadata_find_xml_element (s, "title");
}

/*======================================================================
  metadata_new_from_file
======================================================================*/
MetaData *metadata_new_from_file (const char *filename, GError **error)
{
  unsigned char *data = NULL;
  int data_len = 0;
  blorbreader_get_first_optional_chunk_of_type (filename, 
      "IFmd", &data, &data_len, error);
  if (*error)
    return NULL;

    // TODO process metadata
    // Don't forget -- not NULL-terminated!
  char *md = (char *) malloc (data_len + 1);
  memset (md, 0, data_len + 1);
  memcpy (md, data, data_len);
  free (data);

  // Setting the image number to -1 should allow the first
  //  available image to get loaded (if there is one)
  //  even if there was no 'Fspc' chunk in the blorb
  GError *error2 = NULL;
  int fspc_image = -1;
  blorbreader_get_first_optional_chunk_of_type (filename, 
      "Fspc", &data, &data_len, &error2);
  if (error2)
      {
      // Failing to find an Fspc chunk does not mean we have to give up
      g_error_free (error2);
      } 
  else
      { 
      fspc_image = blorbreader_read_int32 (data); 
      free (data);
      }

  MetaData *self = g_object_new 
   (METADATA_TYPE, NULL);

  self->fspc_image = fspc_image;
  metadata_parse_ifindex (self, md);
  free (md);

  return self;
}


