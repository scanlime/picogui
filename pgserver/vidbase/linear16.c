/* $Id: linear16.c,v 1.8 2002/01/30 12:03:16 micahjd Exp $
 *
 * Video Base Library:
 * linear16.c - For 16bpp linear framebuffers (5-6-5 RGB mapping)
 *
 * BIG FAT WARNING:
 * This is just a stub that only implements pixel() getpixel() and the color
 * conversions. A fast linear16 is on the way, but this will substitute for now.
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2002 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This program is free software; you can redistribute it and/or
 * modify it under the terms of the GNU General Public License
 * as published by the Free Software Foundation; either version 2
 * of the License, or (at your option) any later version.
 * 
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 * 
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
 * 
 * Contributors:
 * 
 * 
 * 
 */

#include <pgserver/common.h>

#include <pgserver/inlstring.h>    /* inline-assembly __memcpy */
#include <pgserver/video.h>
#include <pgserver/autoconf.h>

/* Macros to easily access the members of vid->display */
#define FB_MEM     (((struct stdbitmap*)dest)->bits)
#define FB_BPL     (((struct stdbitmap*)dest)->pitch)
#define FB_ISNORMAL(bmp,lgop) (lgop == PG_LGOP_NONE && ((struct stdbitmap*)bmp)->bpp == vid->bpp)

/* Macro for addressing framebuffer pixels. Note that this is only
 * used when an accumulator won't do, but it is a macro so a line address
 * lookup table might be implemented later if really needed.
 */
#define LINE(y)        ((unsigned short *)((y)*FB_BPL+FB_MEM))
#define PIXELADDR(x,y) ((x)+LINE(y))
#define PIXEL(x,y)     (*PIXELADDR(x,y))

#undef DEBUG

/************************************************** Minimum functionality */

void linear16_pixel(hwrbitmap dest, s16 x,s16 y,hwrcolor c,s16 lgop) {
#ifdef DRIVER_S1D13806
# ifdef DEBUG
  unsigned short * addr = PIXELADDR (x,y);
# endif /* DEBUG */
#endif

  if (!FB_ISNORMAL(dest,lgop)) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }

#ifdef DRIVER_S1D13806
# ifdef DEBUG
   printf ("set %dx%d @ %p\n", x, y, addr);
# endif /* DEBUG */
#endif

   PIXEL(x,y) = c;
}
hwrcolor linear16_getpixel(hwrbitmap dest, s16 x,s16 y) {
#ifdef DRIVER_S1D13806
  unsigned short * addr;
  hwrcolor c;
#endif

  if (!FB_ISNORMAL(dest,PG_LGOP_NONE))
    return def_getpixel(dest,x,y);
  
#ifdef DRIVER_S1D13806
  addr = PIXELADDR (x,y);
  c = * addr;

  return c << 8 | c >> 8;
#else
  return PIXEL(x,y);
#endif
}

/*********************************************** Accelerated (?) primitives */

/* A simple slab function speeds things up a lot compared to def_slab */
void linear16_slab(hwrbitmap dest, s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
  u16 *p;

  if (!FB_ISNORMAL(dest,lgop)) {
    def_slab(dest,x,y,w,c,lgop);
    return;
  }

  p = PIXELADDR(x,y);
  while (w--)
    *(p++) = c;
}

   
/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear16(struct vidlib *vid) {
  setvbl_default(vid);
   
  vid->pixel          = &linear16_pixel;
  vid->getpixel       = &linear16_getpixel;
  vid->slab           = &linear16_slab;
}

/* The End */

