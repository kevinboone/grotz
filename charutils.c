#include <string.h>
#include <gtk/gtk.h>
#include "charutils.h"

/*
 * The following block of code is for converting Unicode formats, and was
 * mercilessly ripped out of the Unicode Consortium's sample code. Please
 * don't ask me how it works, because I don't know. In particular, I am
 * not sure whether we need to take further steps to deal with byte order
 * conversion
 */
typedef unsigned int UTF32;
typedef unsigned short UTF16;
typedef unsigned char UTF8;
static const int halfShift  = 10; 
static const UTF32 halfBase = 0x0010000UL;
static const UTF32 halfMask = 0x3FFUL;
#define UNI_REPLACEMENT_CHAR (UTF32)0x0000FFFD
#define UNI_SUR_HIGH_START  (UTF32)0xD800
#define UNI_SUR_HIGH_END    (UTF32)0xDBFF
#define UNI_SUR_LOW_START   (UTF32)0xDC00
#define UNI_SUR_LOW_END     (UTF32)0xDFFF
static const UTF8 firstByteMark[7] = 
  { 0x00, 0x00, 0xC0, 0xE0, 0xF0, 0xF8, 0xFC };

typedef enum 
  {
  conversionOK,   /* conversion successful */
  sourceExhausted, /* partial character in source, but hit end */
  targetExhausted, /* insuff. room in target for conversion */
  sourceIllegal  /* source sequence is illegal/malformed */
  } UTFConversionResult;

static UTFConversionResult _convert_utf16_to_utf8 (const UTF16** sourceStart, 
      const UTF16* sourceEnd, UTF8** targetStart, UTF8* targetEnd) 
{
  UTFConversionResult result = conversionOK;
  const UTF16* source = *sourceStart;
  UTF8* target = *targetStart;
  while (source < sourceEnd) 
  {
    UTF32 ch;
    unsigned short bytesToWrite = 0;
    const UTF32 byteMask = 0xBF;
    const UTF32 byteMark = 0x80; 
    const UTF16* oldSource = source; 
    ch = *source++;
    if (ch >= UNI_SUR_HIGH_START && ch <= UNI_SUR_HIGH_END) 
     {
       if (source < sourceEnd) 
       {
         UTF32 ch2 = *source;
         if (ch2 >= UNI_SUR_LOW_START && ch2 <= UNI_SUR_LOW_END) 
         {
           ch = ((ch - UNI_SUR_HIGH_START) << halfShift)
              + (ch2 - UNI_SUR_LOW_START) + halfBase;
           ++source;
         } 
       } 
       else 
       { 
         --source; 
         result = sourceExhausted;
         break;
       }
     } 
     if (ch < (UTF32)0x80) 
     {
         bytesToWrite = 1;
      } 
      else if (ch < (UTF32)0x800) 
      {     
        bytesToWrite = 2;
      } 
      else if (ch < (UTF32)0x10000) 
      {   
        bytesToWrite = 3;
      } 
      else if (ch < (UTF32)0x110000) 
      {  
        bytesToWrite = 4;
      } 
      else 
      {
         bytesToWrite = 3;
         ch = UNI_REPLACEMENT_CHAR;
      }

      target += bytesToWrite;
      if (target > targetEnd) 
      {
        source = oldSource; 
        target -= bytesToWrite; result = targetExhausted; 
        break;
      }
      switch (bytesToWrite) 
      { /* note: everything falls through. */
        case 4: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
        case 3: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
        case 2: *--target = (UTF8)((ch | byteMark) & byteMask); ch >>= 6;
        case 1: *--target =  (UTF8)(ch | firstByteMark[bytesToWrite]);
      }
      target += bytesToWrite;
    }
  *sourceStart = source;
  *targetStart = target;
  return result;
}

/* 
 * End of Unicode Consortium sample code
 */

/*======================================================================
  charutils_gunichar2_strlen 
=====================================================================*/
int charutils_gunichar2_strlen (const gunichar2 *s)
  {
  int l = 0;
  const gunichar2 *ss = s;
  while (*ss++) l++;
  return l;  
  }


/*======================================================================
  charutils_utf16_char_to_utf8 
  Convert a single utf16 character to UTF8, leaving the result
  null-terminated (so long as it fits in the allocated buffer)
=====================================================================*/
void charutils_utf16_char_to_utf8 (gunichar2 c, char *out, int len)
  {
  gunichar2 in[2];
  in[0] = c;
  in[1] = 0;

  UTF8 *t = (UTF8*)out;
  const UTF16 *s = in;

  //memset (t, 0, len);

  _convert_utf16_to_utf8 (&s, s+1, &t, t+len);
  *t = 0;
  }


/*======================================================================
  charutils_append_utf16_char_to_utf8_string
  Convert a single utf16 character to UTF8, leaving the result
  null-terminated (so long as it fits in the allocated buffer)
=====================================================================*/
void charutils_append_utf16_char_to_utf8_string (gunichar2 c, GString *str)
  {
  gunichar2 in[2];
  in[0] = c;
  in[1] = 0;

  char out[10];

  UTF8 *t = (UTF8*)out;
  const UTF16 *s = in;

  //memset (t, 0, len);

  _convert_utf16_to_utf8 (&s, s+1, &t, t+sizeof(t));
  *t = 0;

  g_string_append (str, out);
  }




/*======================================================================
  charutils_utf16_string_to_utf8 
  Convert a single utf16 character to UTF8, leaving the result
  null-terminated (so long as it fits in the allocated buffer)
  If len < 0, input str is assumed to be null-terminated
  Caller must free the string returned
=====================================================================*/
GString *charutils_utf16_string_to_utf8 (const gunichar2 *str, int len)
  {
  GString *out_string = g_string_new ("");

  if (len < 0)
    {
    const gunichar2 *p = str;
    len = 0;
    while (*p++) len++;
    }

  int i;
  for (i = 0; i < len; i++)
    {
    gunichar2 in[2];
    in[0] = str[i];
    in[1] = 0;

    char out[10];
    const UTF16 *s = in;
    UTF8 *t = (UTF8*)out;
    memset (t, 0, sizeof (out));

    _convert_utf16_to_utf8 (&s, s+1, &t, t+sizeof(out));
    //*t = 0;

    out_string = g_string_append (out_string, out);
    }

  return out_string;
  }

