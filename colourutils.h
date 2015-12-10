#pragma once

typedef int RGB8COLOUR;
#define MAKE_RGB8COLOUR(r,g,b)(((r)<<16)|((g)<<8)|(b))
#define RGB8WHITE 0x00FFFFFF
#define RGB8BLACK 0x0
#define RGB8DEFAULT -1
#define RGB8TRANSPARENT -2
#define RGB8_GETRED(x) (((x) >> 16) & 0xFF)
#define RGB8_GETGREEN(x) (((x) >> 8) & 0xFF)
#define RGB8_GETBLUE(x) (((x)) & 0xFF)

void colourutils_rgb8_to_gdk (RGB8COLOUR rgb8, GdkColor *gdk);
RGB8COLOUR colourutils_gdk_to_rgb8 (GdkColor *gdk);

