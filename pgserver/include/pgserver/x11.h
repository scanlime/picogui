/* $Id: x11.h,v 1.1 2002/11/04 08:36:25 micahjd Exp $
 *
 * x11.h - Header shared by all the x11 driver components in picogui
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

#ifndef __H_PGX11
#define __H_PGX11

#include <pgserver/render.h>
#include <pgserver/video.h>
#include <pgserver/divtree.h>

#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <X11/keysym.h>
#ifdef __SVR4
#include <X11/Sunkeysym.h>
#endif


/******************************************************** Data types */

/* The new data type used for hwrbitmap.
 * This must be able to represent top-level windows as well as
 * offscreen bitmaps, therefore most of the fields here are optional.
 */
struct x11bitmap {
  Drawable d;                /* This will always be an x11 drawable */
  struct groprender *rend;   /* gropnode rendering info used by picogui */
  s16 w,h;                   /* Width and height */
  hwrbitmap frontbuffer;     /* If this is a backbuffer, this is the associated front buffer */
  Region display_region;     /* A region specifying the entire bitmap */
  struct divtree *dt;        /* The corresponding divtree if this is a window */
};

/* Convenience macro to cast a hwrbitmap to x11bitmap */
#define XB(b) ((struct x11bitmap *)(b))


/******************************************************** Globals */

/* Display and file descriptor for the X server */
extern Display *x11_display;
extern int x11_fd;

/* Stipple bitmap */
#define x11_stipple_width 8
#define x11_stipple_height 8
extern const u8 x11_stipple_bits[];

/* A table of graphics contexts for each LGOP mode */
extern GC x11_gctab[PG_LGOPMAX+1];

/* The window used by x11_window_debug */
extern hwrbitmap x11_debug_window;


/******************************************************** Shared utilities */

/* Redisplay the area inside the given expose region */
void x11_expose(Region r);
void x11_gc_setup(Drawable d);

/******************************************************** Primitives */

void x11_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop);
hwrcolor x11_getpixel(hwrbitmap src,s16 x,s16 y);
void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop);
void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop);
void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop);
void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop);
void x11_update(hwrbitmap display,s16 x,s16 y,s16 w,s16 h);
g_error x11_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend);
g_error x11_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h);
g_error x11_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp);
void x11_bitmap_free(hwrbitmap bmp);
void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop);
void x11_message(u32 message, u32 param, u32 *ret);
int x11_is_rootless(void);
g_error x11_init(void);
g_error x11_setmode(s16 xres,s16 yres,s16 bpp,u32 flags);
void x11_close(void);
hwrbitmap x11_window_debug(void);  
hwrbitmap x11_window_fullscreen(void);  
g_error x11_window_new(hwrbitmap *hbmp, struct divtree *dt);
void x11_window_free(hwrbitmap window);
void x11_window_set_title(hwrbitmap window, const struct pgstring *title);
void x11_window_set_position(hwrbitmap window, s16 x, s16 y);
void x11_window_set_size(hwrbitmap window, s16 w, s16 h);
void x11_window_get_position(hwrbitmap window, s16 *x, s16 *y);
void x11_window_get_size(hwrbitmap window, s16 *w, s16 *h);

#endif /* __H_PGX11 */

/* The End */
