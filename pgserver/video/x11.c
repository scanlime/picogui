/* $Id: x11.c,v 1.10 2001/11/21 06:52:01 micahjd Exp $
 *
 * x11.c - Use the X Window System as a graphics backend for PicoGUI
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/video.h>
#include <pgserver/configfile.h>
#include <pgserver/appmgr.h>
#include <pgserver/render.h>
#include <pgserver/input.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>
#include <stdio.h>

/* Global X display- shared with x11input driver */
Display *xdisplay;

/* Redefine PicoGUI's hwrbitmap so it references an X11 Drawable */
struct x11bitmap {
  Drawable d;
  s16 w,h;
  struct groprender *rend;   /* Context for pgRender() */
  struct x11bitmap *tile;    /* Cached tile            */
} x11_display;

/* This is our back-buffer when we double-buffer. Even if we're not
 * double-buffering everything, we still double-buffer sprites
 */
struct x11bitmap *x11_backbuffer;

/* In X11 there is a comparatively large per-blit penalty, and
 * no API function for tiling a bitmap. To speed things up
 * substantially, bitmaps below this size that need to be tiled
 * are pre-tiled up to at least this size and stored in the
 * bitmap's 'tile' variable.
 *
 * Larger values for this increase speed and memory consumption.
 */
#define X11_TILESIZE 128

/* A table of graphics contexts for each LGOP mode */
GC x11_gctab[PG_LGOPMAX+1];

/* Hook for handling expose events from the input driver */
void (*x11_expose)(int x, int y, int w, int h);

/******************************************** Implementations */

void x11_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

#ifdef CONFIG_X11_NOPIXEL
  /* Paint it bright red, marking areas we can't render */
  c = vid->color_pgtohwr(0xFF0000);
  g = x11_gctab[PG_LGOP_NONE];
#else
  if (!g) {
    def_pixel(dest,x,y,c,lgop);
    return;
  }
#endif
  XSetForeground(xdisplay,g,c);
  XDrawPoint(xdisplay,xb->d,g,x,y);
}

hwrcolor x11_getpixel(hwrbitmap src,s16 x,s16 y) {
#ifdef CONFIG_X11_NOPIXEL
  /* Return bright red, to mark the areas we're blocking out */
  return vid->color_pgtohwr(0xFF0000);
#else

  struct x11bitmap *xb = (struct x11bitmap *) src;
  XImage *img;
  hwrcolor c;

  /* This is really _really_ damn slow. Our goal is to never 
   * have to actually use this function, but we need it for
   * compatibility. I thought about caching the XImage to
   * improve consecutive reads from the same drawable, but
   * there's no good way to determine if the drawable's been
   * modified since the last call.
   */
  img = XGetImage (xdisplay, xb->d, x, y, 1, 1, AllPlanes, ZPixmap);
  c = XGetPixel(img,0,0);
  XDestroyImage(img);
  return c;
#endif
}


void x11_rect(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_rect(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillRectangle(xdisplay,xb->d,g,x,y,w,h);
}

void x11_line(hwrbitmap dest, s16 x1,s16 y1,s16 x2,s16 y2,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_line(dest,x1,y1,x2,y2,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XDrawLine(xdisplay,xb->d,g,x1,y1,x2,y2);
}

void x11_slab(hwrbitmap dest, s16 x,s16 y,s16 w, hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_slab(dest,x,y,w,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillRectangle(xdisplay,xb->d,g,x,y,w,1);
}

void x11_bar(hwrbitmap dest, s16 x,s16 y,s16 h, hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_bar(dest,x,y,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillRectangle(xdisplay,xb->d,g,x,y,1,h);
}

void x11_ellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_ellipse(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XDrawArc(xdisplay,xb->d,g,x,y,w,h,0,360*64);
}

void x11_fellipse(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h,hwrcolor c, s16 lgop) {
  struct x11bitmap *xb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  if (!g) {
    def_fellipse(dest,x,y,w,h,c,lgop);
    return;
  }
  XSetForeground(xdisplay,g,c);
  XFillArc(xdisplay,xb->d,g,x,y,w,h,0,360*64);
}

/* Blit the backbuffer to the display- we always need this,
 * at the very least for sprite buffering
 */
void x11_buffered_update(s16 x,s16 y,s16 w,s16 h) {
  XCopyArea(xdisplay,((struct x11bitmap*)vid->display)->d,x11_display.d,
	    x11_gctab[PG_LGOP_NONE],x,y,w,h,x,y);
  XFlush(xdisplay);
}

/* Non-buffered update. Only need this if we're not doublebuffering
 */
void x11_nonbuffered_update(s16 x,s16 y,s16 w,s16 h) {
  XFlush(xdisplay);
}

/* Similar to the update function, but triggered by the X server */
void x11_buffered_expose(int x,int y,int w,int h) {
  /* If we're double-buffered, this is just x11_buffered_update without the XFlush */
  XCopyArea(xdisplay,((struct x11bitmap*)vid->display)->d,x11_display.d,
	    x11_gctab[PG_LGOP_NONE],x,y,w,h,x,y);
}

/* We're not so lucky... set all the GCs to clip to this expose rectangle,
 * redraw all the divtree layers, then set the GCs back to normal.
 * Ugly, but avoids an extra buffer.
 */
void x11_nonbuffered_expose(int x,int y,int w,int h) {
  struct divtree *p;
  int i;
  XRectangle cliprect;
  
  cliprect.x      = x;
  cliprect.y      = y;
  cliprect.width  = w;
  cliprect.height = h;
  
  for (i=0;i<=PG_LGOPMAX;i++)
    if (x11_gctab[i])
      XSetClipRectangles(xdisplay, x11_gctab[i], 0, 0, &cliprect, 1, Unsorted);

  for (p=dts->top;p;p=p->next)
    p->flags |= DIVTREE_ALL_REDRAW;
  update(NULL,1);

  cliprect.x      = 0;
  cliprect.y      = 0;
  cliprect.width  = vid->xres;
  cliprect.height = vid->yres;

  for (i=0;i<=PG_LGOPMAX;i++)
    if (x11_gctab[i])
      XSetClipRectangles(xdisplay, x11_gctab[i], 0, 0, &cliprect, 1, Unsorted);
}

g_error x11_bitmap_get_groprender(hwrbitmap bmp, struct groprender **rend) {
  g_error e;
  struct x11bitmap *xb = (struct x11bitmap *) bmp;

  if (xb->rend) {
    *rend = xb->rend;
    return sucess;
  }

  /* New groprender context for this bitmap */
  e = g_malloc((void **) rend,sizeof(struct groprender));
  errorcheck;
  xb->rend = *rend;
  memset(*rend,0,sizeof(struct groprender));
  (*rend)->lgop = PG_LGOP_NONE;
  (*rend)->output = bmp;
  (*rend)->hfont = defaultfont;
  (*rend)->clip.x2 = xb->w - 1;
  (*rend)->clip.y2 = xb->h - 1;
  (*rend)->orig_clip = (*rend)->clip;
  (*rend)->output_rect.w = xb->w;
  (*rend)->output_rect.h = xb->h;

  return sucess;
}

g_error x11_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
  struct x11bitmap *xb = (struct x11bitmap *) bmp;
  *w = xb->w;
  *h = xb->h;
  return sucess;
}

g_error x11_bitmap_new(hwrbitmap *bmp,s16 w,s16 h) {
  struct x11bitmap **pxb = (struct x11bitmap **) bmp;
  g_error e;

  int lw;
  u32 size;

  /* Allocate an x11bitmap structure for this */
  e = g_malloc((void **) pxb,sizeof(struct x11bitmap));
  errorcheck;
  memset(*pxb,0,sizeof(struct x11bitmap));
  (*pxb)->w = w;
  (*pxb)->h = h;

  /* Allocate a corresponding X pixmap */
  (*pxb)->d  = XCreatePixmap(xdisplay,x11_display.d,w,h,vid->bpp);

  return sucess;
}

void x11_bitmap_free(hwrbitmap bmp) {
  struct x11bitmap *xb = (struct x11bitmap *) bmp;
  if (xb->tile)
    x11_bitmap_free((hwrbitmap) xb->tile);
  if (xb->rend)
    g_free(xb->rend);
  XFreePixmap(xdisplay,xb->d);
  g_free(xb);
}

void x11_blit(hwrbitmap dest, s16 x,s16 y,s16 w,s16 h, hwrbitmap src,
	      s16 src_x, s16 src_y, s16 lgop) {
  struct x11bitmap *sxb = (struct x11bitmap *) src;
  struct x11bitmap *dxb = (struct x11bitmap *) dest;
  GC g = x11_gctab[lgop];

  /* If it's an unsupported LGOP fall back on def_blit.
   */
  if (!g) {
    def_blit(dest,x,y,w,h,src,src_x,src_y,lgop);
    return;
  }

  /* If we're tiling, expand the bitmap to the minimum tile size
   * if necessary, and cache it. Then, if the tile is still smaller
   * than our blit size send it to the default blitter to be
   * decomposed into individual tiles.
   */

  if ((w > sxb->w || h > sxb->h) && 
      (sxb->w < X11_TILESIZE || sxb->h < X11_TILESIZE)) {
    /* Create a pre-tiled image */
    if (!sxb->tile) {
      vid->bitmap_new( (hwrbitmap*)&sxb->tile,
		       (X11_TILESIZE/sxb->w + 1) * sxb->w,
		       (X11_TILESIZE/sxb->h + 1) * sxb->h );
      def_blit((hwrbitmap)sxb->tile,0,0,sxb->tile->w,
	       sxb->tile->h,(hwrbitmap)sxb,0,0,PG_LGOP_NONE);
    }
    sxb = sxb->tile;
  }

  if (w > sxb->w || h > sxb->h)
    def_blit(dest,x,y,w,h,(hwrbitmap) sxb,src_x,src_y,lgop);
  else
    XCopyArea(xdisplay,sxb->d,dxb->d,g,src_x,src_y,w,h,x,y);
}

/* The default sprite code is fine on slow LCDs, or
 * on double-buffered displays. On X11 without double-buffering,
 * the flickering's pretty bad. x11_sprite_update() does a little
 * magic to double-buffer sprite updates.
 */
void x11_buffered_sprite_update(struct sprite *spr) {

  /* Only bother with this if the sprite is big. Small sprites
   * won't flicker anyway, and this code would slow them down
   * dramatically, due to the 2 extra blits.
   *
   * This defines 32x32 as a small sprite, but that's a fudge factor.
   */
  if (spr->x<=32 && spr->y<=32) {
    def_sprite_update(spr);
    return;
  }

  /* What's the relevant portion of the display? Assume that the
   * old and new positions of the sprite are close together. Use
   * add_updarea to calculate us an update region.
   */
  add_updarea(spr->x,spr->y,spr->w,spr->h);
  add_updarea(spr->ox,spr->oy,spr->ow,spr->oh);

  /* Copy the relevant portion of the display to the backbuffer */
  XCopyArea(xdisplay,x11_display.d,x11_backbuffer->d,
  	    x11_gctab[PG_LGOP_NONE],upd_x,upd_y,upd_w,upd_h,upd_x,upd_y);

  /* Normal sprite update, but draw to the backbuffer instead
   * of the display itself.
   */
  vid->display = (hwrbitmap) x11_backbuffer;
  vid->sprite_hide(spr);
  vid->sprite_show(spr);
  
  /* Copy backbuffer to display */
  x11_buffered_update(upd_x,upd_y,upd_w,upd_h);
  vid->display = (hwrbitmap) &x11_display;
}

/* Connect to the default X server */
g_error x11_init(void) {
  xdisplay = XOpenDisplay(NULL);
  if (!xdisplay)
    return mkerror(PG_ERRT_IO,46);   /* Error initializing video */

  /* Load the matching input driver */
  return load_inlib(&x11input_regfunc,&inlib_main);
}

/* Create a window */
g_error x11_setmode(s16 xres,s16 yres,s16 bpp,unsigned long flags) {
  int black;
  XEvent ev;
  XTextProperty titleprop;
  char title[80];
  g_error e;
  Visual *xvisual;

  /* Default resolution is 640x480 
   */
  if (!xres) xres = 640;
  if (!yres) yres = 480;

  /* Create the window
   */
  black = BlackPixel(xdisplay, DefaultScreen(xdisplay));
  x11_display.d = XCreateSimpleWindow(xdisplay, DefaultRootWindow(xdisplay),
				      0, 0, xres, yres, 0, black, black);
  
  /* Map the window, waiting for the MapNotify event 
   */
  XSelectInput(xdisplay, x11_display.d, StructureNotifyMask);  
  XMapWindow(xdisplay,x11_display.d);
  do {
    XNextEvent(xdisplay, &ev);
  } while (ev.type != MapNotify);

  /* Save display information
   */
  xvisual = DefaultVisual(xdisplay,0);
  vid->xres = xres;
  vid->yres = yres;
  vid->bpp  = DefaultDepth(xdisplay,0);
  x11_display.w = xres;
  x11_display.h = yres;

  /* Set up graphics contexts for each LGOP that X can support directly 
   */
  x11_gctab[PG_LGOP_NONE]       = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_OR]         = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_AND]        = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_XOR]        = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT]     = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_OR]  = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_AND] = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_INVERT_XOR] = XCreateGC(xdisplay,x11_display.d,0,NULL);
  x11_gctab[PG_LGOP_STIPPLE]    = XCreateGC(xdisplay,x11_display.d,0,NULL);

  XSetFunction(xdisplay,x11_gctab[PG_LGOP_OR],        GXor);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_AND],       GXand);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_XOR],       GXxor);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT],    GXcopyInverted);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT_OR], GXorInverted);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT_AND],GXandInverted);
  XSetFunction(xdisplay,x11_gctab[PG_LGOP_INVERT_XOR],GXequiv);

  /* FIXME: Stipple LGOP not working in X?
   *
  XSetLineAttributes(xdisplay,x11_gctab[PG_LGOP_STIPPLE],
		     1,LineOnOffDash,CapRound,JoinRound);
  XSetFillStyle(xdisplay,x11_gctab[PG_LGOP_STIPPLE],FillStippled);
  XSetFillRule(xdisplay,x11_gctab[PG_LGOP_STIPPLE],EvenOddRule);
   */
  
  /* Set up some kind of double-buffering */
  switch (get_param_int("video-x11","doublebuffer",1)) {
    
    /* Fully double-buffered */
  case 1:
    e = vid->bitmap_new((hwrbitmap*) &x11_backbuffer,vid->xres,vid->yres);
    errorcheck;  
    vid->display = (hwrbitmap) x11_backbuffer;
    vid->update = &x11_buffered_update;
    x11_expose = &x11_buffered_expose;
    break;

    /* Only double-buffer sprites */
  case 2:
    e = vid->bitmap_new((hwrbitmap*) &x11_backbuffer,vid->xres,vid->yres);
    errorcheck;  
    vid->display = (hwrbitmap) &x11_display;
    vid->update = &x11_nonbuffered_update;
    x11_expose = &x11_nonbuffered_expose;
    vid->sprite_update = &x11_buffered_sprite_update;
    break;
    
    /* No double-buffer */
  default:
    vid->display = (hwrbitmap) &x11_display;
    vid->update = &x11_nonbuffered_update;
    x11_expose = &x11_nonbuffered_expose;
   break;
  }

  /* Blank the cursor if we don't want it */
  if (!get_param_int("input-x11","xcursor",1)) {
    XColor black, white;
    Pixmap p;
    Cursor c;
    GC g;
    
    p = XCreatePixmap(xdisplay,x11_display.d,8,8,1);
    g = XCreateGC(xdisplay,p,0,NULL);
    XFillRectangle(xdisplay,p,g,0,0,8,8);
    
    black.red = black.green = black.blue = 0x0000;
    white.red = white.green = white.blue = 0xFFFF;
    
    c = XCreatePixmapCursor(xdisplay,p,p,&black,&white,0,0);
    XDefineCursor(xdisplay, x11_display.d, c);
  }

  /* Set the window title
   */
  sprintf(title,get_param_str("video-x11","caption","PicoGUI (X11@%dx%dx%d)"),
  	  vid->xres,vid->yres,vid->bpp);
  XStoreName(xdisplay, x11_display.d, title);

  /* Set input event mask */
  XSelectInput(xdisplay, x11_display.d,
	       KeyPressMask | KeyReleaseMask | ExposureMask | ButtonMotionMask |
	       ButtonPressMask | ButtonReleaseMask | PointerMotionMask);
  XAutoRepeatOn(xdisplay);

  XFlush(xdisplay);
  return sucess;
}

void x11_close(void) {
  if (x11_display.rend)
    g_free(x11_display.rend);
  if (x11_backbuffer)
    vid->bitmap_free((hwrbitmap) x11_backbuffer);
  unload_inlib(inlib_main);   /* Take out our input driver */
  XCloseDisplay(xdisplay);
  xdisplay = NULL;
}

/******************************************** Driver registration */

g_error x11_regfunc(struct vidlib *v) {
  setvbl_default(v);
  
  v->init = &x11_init;
  v->setmode = &x11_setmode;
  v->close = &x11_close;
  v->pixel = &x11_pixel;
  v->getpixel = &x11_getpixel;
  v->bitmap_get_groprender = &x11_bitmap_get_groprender;
  v->bitmap_getsize = &x11_bitmap_getsize;
  v->bitmap_new = &x11_bitmap_new;
  v->bitmap_free = &x11_bitmap_free;
  v->rect = &x11_rect;
  v->blit = &x11_blit;
  v->slab = &x11_slab;
  v->bar = &x11_bar;
  v->line = &x11_line;

  if (!get_param_int("video-x11","defaultellipse",0)) {
    v->ellipse = &x11_ellipse;
    v->fellipse = &x11_fellipse;
  }

  return sucess;
}

/* The End */
