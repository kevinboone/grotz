#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gtk/gtk.h>
#include "blorbreader.h"
#include "Picture.h"
#include "Sound.h"

GQuark blorbreader_error_quark ()
{
  static GQuark error_quark=0;
  if (error_quark == 0)
  {
    error_quark = g_quark_from_string("blorbreader-error-quark");
  }
  return error_quark;
}

#define BITYPE_UNKNOWN 0
#define BITYPE_PICT 1
#define BITYPE_SND 2
#define BITYPE_CODE 3

typedef struct
  {
  int type;
  int z_num;
  int offset;
  } BlorbIndexEntry;

//ChunkMemory struct is used to pass a block of memory between
// callback functions that can only pass a single argument
typedef struct
  {
  char chunk_id [5]; // Chunk IDs are at most 4 bytes. 0 == match any type
  unsigned char *data; 
  int len; 
  int znum; // -1 == match any znum
  } ChunkMemory;


typedef gboolean (*BlorbReaderIterator) (int znum, int type,  
     unsigned char *chunk_id, unsigned char *chunk, int chunk_len, 
     void *user_data);


/*======================================================================
  blorbreader_read_int32
======================================================================*/
int blorbreader_read_int32 (unsigned char *p)
  {
  return (*p << 24) 
   | (*(p+1) << 16)
   | (*(p+2) << 8)
   | (*(p+3));
  }



/*======================================================================
  blorbreader_iterate_chunks
======================================================================*/
void blorbreader_iterate_chunks (const char *filename, BlorbReaderIterator 
    iterator, void *user_data, GError **error)
  {
  FILE *f = fopen (filename, "rb");

  if (f) 
    {
    unsigned char buff[512];
    fseek (f, 12, SEEK_SET);
    fread (buff, 4, 1, f);
    if (buff[0] == 'R' && buff[1] == 'I' && buff[2] == 'd'
         && buff[3] == 'x')
      {
      fseek (f, 4, SEEK_CUR);
      fread (buff, 4, 1, f);
      int i, num_res = blorbreader_read_int32 (buff);

      BlorbIndexEntry **bi = (BlorbIndexEntry**) malloc
        (num_res * sizeof (BlorbIndexEntry*));
       
      for (i = 0; i < num_res && !(*error); i++)
        {
        BlorbIndexEntry *bie = (BlorbIndexEntry*) malloc
          (num_res * sizeof (BlorbIndexEntry));
        bi[i] = bie;
        fread (buff, 12, 1, f);
        if (strncmp ((char *)buff, "Pict", 4) == 0)
           bie->type = BITYPE_PICT;
        else if (strncmp ((char *)buff, "Snd", 3) == 0)
           bie->type = BITYPE_SND;
        else if (strncmp ((char *)buff, "Exec", 4) == 0)
           bie->type = BITYPE_CODE;
        else
           bie->type = BITYPE_UNKNOWN;
        int z_num = blorbreader_read_int32 (buff + 4); 
        int offset = blorbreader_read_int32 (buff + 8); 
        bie->z_num = z_num;
        bie->offset = offset;
        }

      for (i = 0; i < num_res && !(*error); i++)
        {
        BlorbIndexEntry *bie = bi[i]; 
        
        fseek (f, bie->offset, SEEK_SET);
        fread (buff, 8, 1, f);
        //fseek (f, bie->offset, SEEK_SET);
        
        int len = blorbreader_read_int32 (buff + 4); 
  
        unsigned char *chunk = (unsigned char *)
          malloc (len);  

        fread (chunk, len, 1, f);
   
        gboolean iret = iterator (bie->z_num, bie->type, buff, chunk, 
          len, user_data);

        free (chunk);
        if (!iret) break;
        }

      for (i = 0; i < num_res; i++)
        free (bi[i]); 
      free (bi);
      }
    else
      {
      g_set_error (error, blorbreader_error_quark(), -1,
        "Not a valid blorb file: %s", filename);
      }

    fclose (f);
    } 
  else
    g_set_error (error, blorbreader_error_quark(), -1,
      "Can't open file for reading: %s", filename);
  }


/*======================================================================
  blorbreader_iterate_optional_chunks
  Sigh -- whoever thought this was a good way to store data?
  'optional' chunks are unindexed -- we have to plod through the
  file chunk by chunk to find the optional bits
======================================================================*/
void blorbreader_iterate_optional_chunks (const char *filename, 
    BlorbReaderIterator 
    iterator, void *user_data, GError **error)
  {
  FILE *f = fopen (filename, "rb");

  if (f) 
    {
    unsigned char buff[512];
    fseek (f, 12, SEEK_SET);
    fread (buff, 4, 1, f);
    if (buff[0] == 'R' && buff[1] == 'I' && buff[2] == 'd'
         && buff[3] == 'x')
      {
      fseek (f, 12, SEEK_SET);
      gboolean stop = FALSE;
      while (!feof (f) && !stop)
        {
        if (fread (buff, 8, 1, f) != 1) 
          stop = TRUE;
        else
          {
          unsigned char *chunk_type = buff;
          int chunk_len = blorbreader_read_int32 (buff + 4);

          unsigned char *chunk = (unsigned char *) malloc (chunk_len);
          fread (chunk, chunk_len, 1, f);
          if (chunk_len % 2 != 0)
            {
            // Odd length chunks are padded one byte
            char dummy;
            fread (&dummy, 1, 1, f);
            }

          // Can't give the iterator a znum because these are
          //  non-index chunks
          gboolean iret = iterator (-1, BITYPE_UNKNOWN, chunk_type, chunk, 
            chunk_len, user_data);

          if (!iret) stop = TRUE;
 
          free (chunk);
          }
        }
      }
    else
      {
      g_set_error (error, blorbreader_error_quark(), -1,
        "Not a valid blorb file: %s", filename);
      }

    fclose (f);
    } 
  else
    g_set_error (error, blorbreader_error_quark(), -1,
      "Can't open file for reading: %s", filename);
  }



/*======================================================================
  blorbreader_getfirst_iterator
======================================================================*/
gboolean blorbreader_getfirst_iterator (int znum, int type,  
     unsigned char *chunk_id, unsigned char *chunk, int chunk_len, 
     void *user_data)
  {
  ChunkMemory *chunk_mem = (ChunkMemory *) user_data; 
  
  char temp_cid[5];
  memcpy (temp_cid, chunk_id, 4);
  temp_cid[4] = 0;

  gboolean match = FALSE;

  if (strncmp (temp_cid, chunk_mem->chunk_id, 
       strlen (chunk_mem->chunk_id)) == 0 
      && strlen (chunk_mem->chunk_id) > 0 
      && chunk_mem->znum < 0)
    match = TRUE;

  if (znum == chunk_mem->znum && znum > 0)
    match = TRUE;

  if (match)
    {
    chunk_mem->data = (unsigned char *) malloc (chunk_len);  
    chunk_mem->len = chunk_len;
    memcpy (chunk_mem->data, chunk, chunk_len);
    return FALSE;
    }

  return TRUE;
  }


/*======================================================================
  blorbreader_picture_iterator
======================================================================*/
gboolean blorbreader_picture_iterator (int znum, int type,  
     unsigned char *chunk_id, unsigned char *chunk, int chunk_len, 
     void *user_data)
  {
  if (type != BITYPE_PICT) return TRUE;

  GList **list = (GList **) user_data;

  if (chunk_id[0] == 'P' && chunk_id[1] == 'N' && chunk_id[2] == 'G')
    {
    // We have a PNG chunk

    if (chunk_len > 24)
      {
      int width = blorbreader_read_int32 (chunk + 16);
      int height = blorbreader_read_int32 (chunk + 20);

      Picture *p = picture_new (znum, width, height, PICTURE_FORMAT_PNG);
      *list = g_list_append (*list, p);
      picture_set_data (p, chunk, chunk_len);
      }
    else
      {
      g_message ("Corrupt CLB chunk in resource %d\n", znum);
      } 
    }
  else if (chunk_id[0] == 'J' && chunk_id[1] == 'P' && chunk_id[2] == 'E')
    {
    // We have a JPEG chunk

    if (chunk_len > 24)
      {
      int width = 100; 
      int height = 100; 
      //int width = blorbreader_read_int32 (chunk + 16);
      //int height = blorbreader_read_int32 (chunk + 20);

      Picture *p = picture_new (znum, width, height, PICTURE_FORMAT_JPG);
      *list = g_list_append (*list, p);
      picture_set_data (p, chunk, chunk_len);
      }
    else
      {
      g_message ("Corrupt CLB chunk in resource %d\n", znum);
      } 
    }
  else if (chunk_id[0] == 'R' && chunk_id[1] == 'e' && chunk_id[2] == 'c')
    {
    // Dummy rectangle -- no image data
      int width = blorbreader_read_int32 (chunk);
      int height = blorbreader_read_int32 (chunk + 4);

      Picture *p = picture_new (znum, width, height, PICTURE_FORMAT_PNG);
      *list = g_list_append (*list, p);
    }

  return TRUE; // continue
  }


/*======================================================================
  blorbreader_sound_iterator
======================================================================*/
gboolean blorbreader_sound_iterator (int znum, int type,  
     unsigned char *chunk_id, unsigned char *chunk, int chunk_len, 
     void *user_data)
  {
  if (type != BITYPE_SND) return TRUE;

  GList **list = (GList **) user_data;

  // For some Byzantine reason, AIF sounds are stored as a full 
  // FORM chunk, unlike all the other BLORB resources. This must have
  // made sense to somebody, at some time. In any event, we do have the
  // problem now that these routines are set up in a way that doesn't
  // work for this resource type. We have to fudge it by hoping that
  // the first 8 bytes from chunk_id actually contain the 8 bytes of
  // the FORM header tht we actually need here, and that we skipped for
  // all the other reasource types. Sigh :/ 
  if (chunk_id[0] == 'F' && chunk_id[1] == 'O' && chunk_id[2] == 'R')
    {
    if (chunk_len > 8)
      {
      Sound *p = sound_new (znum, SOUND_FORMAT_AIFF);
      *list = g_list_append (*list, p);
      unsigned char *aif = malloc (chunk_len + 8);
      memcpy (aif, chunk_id, 8);
      memcpy (aif + 8, chunk, chunk_len);
      sound_set_data (p, aif, chunk_len + 8);

      #ifdef DUMPSOUNDS
      char fname[100];
      sprintf (fname, "aiftest%d.aif", znum);
      FILE *f = fopen (fname, "wb");
      fwrite (aif, chunk_len + 8, 1, f);
      fclose(f);
      #endif

      free (aif);
      }
    else
      {
      g_message ("Corrupt CLB chunk in resource %d\n", znum);
      } 
    }
  else if (chunk_id[0] == 'O' && chunk_id[1] == 'G' && chunk_id[2] == 'G')
    {
    // We have an OGG chunk

    if (chunk_len > 16)
      {
      Sound *p = sound_new (znum, SOUND_FORMAT_OGG);
      *list = g_list_append (*list, p);
      sound_set_data (p, chunk, chunk_len);
      }
    else
      {
      g_message ("Corrupt CLB chunk in resource %d\n", znum);
      } 
    }
  else if (chunk_id[0] == 'M' && chunk_id[1] == 'O' && chunk_id[2] == 'D')
    {
    // We have an MOD chunk

    if (chunk_len > 16)
      {
      Sound *p = sound_new (znum, SOUND_FORMAT_MOD);
      *list = g_list_append (*list, p);
      sound_set_data (p, chunk, chunk_len);
      }
    else
      {
      g_message ("Corrupt CLB chunk in resource %d\n", znum);
      } 
    }

  return TRUE; // continue
  }


/*======================================================================
  blorbreader_get_pictures
======================================================================*/
GList *blorbreader_get_pictures (const char *filename, GError **error)
  {
  GList *list = NULL;
  blorbreader_iterate_chunks (filename, blorbreader_picture_iterator, 
    &list, error);
  return list;
  }


/*======================================================================
  blorbreader_get_sounds
======================================================================*/
GList *blorbreader_get_sounds (const char *filename, GError **error)
  {
  GList *list = NULL;
  blorbreader_iterate_chunks (filename, blorbreader_sound_iterator, 
    &list, error);
  return list;
  }


/*======================================================================
  blorbreader_get_first_chunk_of_type
======================================================================*/
void blorbreader_get_first_chunk_of_type (const char *filename, 
    const char *type, unsigned char **data, int *data_len, GError **error)
  {
  ChunkMemory chunk_mem;
  chunk_mem.data = NULL;
  chunk_mem.len = 0;
  chunk_mem.znum = -1;
  strncpy (chunk_mem.chunk_id, type, 4);
  chunk_mem.chunk_id[4] = 0;
  blorbreader_iterate_chunks (filename, blorbreader_getfirst_iterator, 
    &chunk_mem, error);
  if (*error) return;

  if (!chunk_mem.data)
    {
      g_set_error (error, blorbreader_error_quark(), -1,
        "No chunk of type %s found in blorb file", type);
      return;
    }

  *data = chunk_mem.data;
  *data_len = chunk_mem.len;
  }


/*======================================================================
  blorbreader_get_chunk_with_znum
======================================================================*/
void blorbreader_get_chunk_with_znum (const char *filename, 
    int znum, unsigned char **data, int *data_len, GError **error)
  {
  ChunkMemory chunk_mem;
  chunk_mem.data = NULL;
  chunk_mem.len = 0;
  chunk_mem.chunk_id[0] = 0;
  chunk_mem.znum = znum;
  blorbreader_iterate_chunks (filename, blorbreader_getfirst_iterator, 
    &chunk_mem, error);
  if (*error) return;

  if (!chunk_mem.data)
    {
      g_set_error (error, blorbreader_error_quark(), -1,
        "No chunk with znum=%d found in blorb file", znum);
      return;
    }

  *data = chunk_mem.data;
  *data_len = chunk_mem.len;
  }


/*======================================================================
  blorbreader_get_first_optional_chunk_of_type
======================================================================*/
void blorbreader_get_first_optional_chunk_of_type (const char *filename, 
    const char *type, unsigned char **data, int *data_len, GError **error)
  {
  ChunkMemory chunk_mem;
  chunk_mem.data = NULL;
  chunk_mem.len = 0;
  strncpy (chunk_mem.chunk_id, type, 4);
  chunk_mem.chunk_id[4] = 0;
  chunk_mem.znum = -1;
  blorbreader_iterate_optional_chunks (filename, 
      blorbreader_getfirst_iterator, 
      &chunk_mem, error);
  if (*error) return;

  if (!chunk_mem.data)
    {
      g_set_error (error, blorbreader_error_quark(), -1,
        "No optional chunk of type %s found in blorb file", type);
      return;
    }

  *data = chunk_mem.data;
  *data_len = chunk_mem.len;
  }


/*======================================================================
  blorbreader_get_pixbuf
  znum == -1 means get first image available
======================================================================*/
GdkPixbuf *blorbreader_get_pixbuf (const char *filename, int znum, 
    int max_width, int max_height, GError **error)
  {
  unsigned char *data = NULL;
  int data_len = 0;
  if (znum < 0)
    blorbreader_get_first_chunk_of_type (filename, 
        "PNG", &data, &data_len, error);
  else
    blorbreader_get_chunk_with_znum (filename, 
        znum, &data, &data_len, error);
  if (*error)
    return NULL;
      
  // TODO: things will go nasty here if the chunk we asked for was
  //  present, but _wasnt'_ an image

  GInputStream *s = g_memory_input_stream_new_from_data 
    (data, data_len, NULL); 
  GdkPixbuf *pb = gdk_pixbuf_new_from_stream (s, NULL, error);
  g_object_unref (s);
  if (*error)
    {
    free (data);
    return NULL;
    }
  free (data);

  int pbw = gdk_pixbuf_get_width (pb);
  int pbh = gdk_pixbuf_get_height (pb);

  if (pbw < max_width && pbw < max_height) return pb;

  int new_w = pbw;
  int new_h = pbh;

  gboolean portrait = FALSE;
  if (pbh > pbw) portrait = TRUE;

  if (portrait)
    {
    new_h = max_height;
    new_w = max_height *pbw / pbh;
    }
  else
    {
    new_w = max_width;
    new_h = max_width *pbh / pbw;
    }

  // Aspect ratio should be correct at this point, but may be
  //  still too large

  double scale = 1.0;
  if (new_w > max_width)
    scale = (double) max_width / (double) new_w; 
  if (new_h > max_height)
    scale = (double) max_height / (double) new_h; 
 
  new_w *= scale;
  new_h *= scale;

  GdkPixbuf *pbs = gdk_pixbuf_scale_simple (pb, 
        new_w, new_h, 
        GDK_INTERP_BILINEAR);    

  g_object_unref (pb);

  return pbs;
  }

