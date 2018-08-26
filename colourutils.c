#include <stdio.h>
#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <gdk/gdk.h>
#include "colourutils.h"

/*======================================================================
  colourutils_rgb8_to_gdk
  GDK colours use 16 bits for each of R, G, and B. So we need
  to multiply each of our RGB8 components by 256
======================================================================*/
void colourutils_rgb8_to_gdk (RGB8COLOUR rgb8, GdkColor *gdk)
  {
  gdk->red =   RGB8_GETRED (rgb8) << 8;
  gdk->blue =  RGB8_GETBLUE (rgb8) << 8;
  gdk->green = RGB8_GETGREEN (rgb8) << 8;
  }


/*======================================================================
  colourutils_gdk_to_rgb8
  GDK colours use 16 bits for each of R, G, and B. So we need
  to multiply each of our RGB8 components by 256
======================================================================*/
RGB8COLOUR colourutils_gdk_to_rgb8 (GdkColor *gdk)
  {
  return MAKE_RGB8COLOUR (gdk->red >> 8, gdk->green >> 8, gdk->blue >> 8);
  }



