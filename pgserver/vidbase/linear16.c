/* $Id: linear16.c,v 1.9 2002/02/03 14:19:01 micahjd Exp $
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
#define LINE(y)        ((u16 *)((y)*FB_BPL+FB_MEM))
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

/* Fun-fun-fun blit functions! */
void linear16_blit(hwrbitmap dest,
		   s16 dst_x, s16 dst_y,s16 w, s16 h,
		   hwrbitmap sbit,s16 src_x,s16 src_y,
		   s16 lgop) {
  u16 *dst;
  struct stdbitmap *srcbit = (struct stdbitmap *) sbit;
  s16 i,offset_dst;
  
  if (!FB_ISNORMAL(dest,PG_LGOP_NONE)) {
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
  }

  /* We support a few common LGOPs, but supporting all of them would just
   * waste space. */
  switch (lgop) {
   case PG_LGOP_NONE:
   case PG_LGOP_OR:
   case PG_LGOP_AND:
   case PG_LGOP_XOR:
     break;
   default:
     def_blit(dest,dst_x,dst_y,w,h,sbit,src_x,src_y,lgop);
     return;
  }
   
  /* Calculations needed by both normal and tiled blits */
  dst = PIXELADDR(dst_x,dst_y);
  offset_dst = (FB_BPL>>1) - w;

  /* The following little macro mess is to repeat the
     loop using different logical operations.
     (Putting the switch inside the loop would be
     easier to read, but much slower)

     You still have to admit that this blitter is _much_ better
     written than the one on the old SDL driver...

     This loop uses __memcpy for the normal blits, and for lgop blits
     it uses loops, performing as much as possible 4 bytes at a time
  */

  /* Normal blit loop */
#define BLITLOOP(op)                                               \
    for (;h;h--,src+=offset_src,dst+=offset_dst) {                 \
      for (i=w;i;i--,src++,dst++)                                  \
	*dst op *src;                                              \
    }
  
  /* Tiled blit loop - similar to tileblit() but always restarts the bitmap
   * on a tile boundary, instead of tiling a bitmap section */
#define TILEBLITLOOP(op)                                           \
   while (h) {                                                            \
      for (;sh && h;sh--,h--,src_line+=srcbit->pitch,dst+=offset_dst) {       \
	 src = src_line + src_x;                                          \
	 swm = (swp < w) ? swp : w;                                       \
	 for (dw=w;dw;) {                                                 \
	    for (sw=swm;sw;sw--,src++,dst++,dw--)                         \
	      *dst op *src;                                               \
	    src = src_line;                                               \
	    swm = (srcbit->w < dw) ? srcbit->w : dw;                      \
	 }                                                                \
      }                                                                   \
      sh = srcbit->h;                                                     \
      src_line = srcbit->bits;                                            \
   }

  /* Is this a normal or tiled blit? */
  if (w>(srcbit->w-src_x) || h>(srcbit->h-src_y)) {   /* Tiled */
    u16 *src,*src_line;
    s16 dw,sh,swm,sw,swp;
     
    /* A few calculations for tiled blits */
    src_x %= srcbit->w;
    src_y %= srcbit->h;
    src_line = ((u16*)srcbit->bits) + src_y*(srcbit->pitch>>1); 
    sh = srcbit->h - src_y;
    swp = srcbit->w - src_x;

    switch (lgop) {

    case PG_LGOP_NONE:  
       while (h) {
	  for (;sh && h;sh--,h--,src_line+=srcbit->pitch>>1,dst+=offset_dst) {
	     src = src_line + src_x;
	     swm = (swp < w) ? swp : w;
	     for (dw=w;dw;) {
		__memcpy(dst,src,swm<<1);
		dst += swm;
		src = src_line;
		dw -= swm;
		swm = (srcbit->w < dw) ? srcbit->w : dw;
	     }
	  }
	  sh = srcbit->h;
	  src_line = srcbit->bits;
       }
       break;

    case PG_LGOP_OR:         TILEBLITLOOP(|=);                   break;
    case PG_LGOP_AND:        TILEBLITLOOP(&=);                   break;
    case PG_LGOP_XOR:        TILEBLITLOOP(^=);                   break;
    }
  }
  else {                                        /* Normal */
    s16 offset_src;
    u16 *src;

    /* Only needed for normal blits */
    src = ((u16*)srcbit->bits) + src_x + src_y*(srcbit->pitch>>1);
    offset_src = (srcbit->pitch>>1) - w;

    switch (lgop) {
    case PG_LGOP_NONE: 
       for (;h;h--,src+=(srcbit->pitch>>1),dst+=(FB_BPL>>1))
	 __memcpy(dst,src,w<<1);
       break;
    case PG_LGOP_OR:         BLITLOOP(|=);                   break;
    case PG_LGOP_AND:        BLITLOOP(&=);                   break;
    case PG_LGOP_XOR:        BLITLOOP(^=);                   break;
    }
  }
}


/*********************************************** Registration */

/* Load our driver functions into a vidlib */
void setvbl_linear16(struct vidlib *vid) {
  setvbl_default(vid);
   
  vid->pixel          = &linear16_pixel;
  vid->getpixel       = &linear16_getpixel;
  vid->slab           = &linear16_slab;
  vid->blit           = &linear16_blit;
}

/* The End */

