/* $Id: rotate270.c,v 1.7 2002/02/23 05:25:28 micahjd Exp $
 *
 * rotate270.c - Video wrapper to rotate the screen 270 degrees
 *
 * This is crufty, but I don't know of any other way to do it that wouldn't
 * bog down the drivers when not rotated.
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
#include <pgserver/video.h>
#include <pgserver/appmgr.h>
#include <pgserver/render.h>

/******* Simple wrapper functions */

void rotate270_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->pixel)(dest,dx-1-y,x,c,lgop);
}
hwrcolor rotate270_getpixel(hwrbitmap src,s16 x,s16 y) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(src,&dx,&dy);
   return (*vid->getpixel)(src,dx-1-y,x);
}
void rotate270_update(s16 x,s16 y,s16 w,s16 h) {
   (*vid->update)(vid->xres-y-h,x,h,w);
}
void rotate270_rect(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		   hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->rect)(dest,dx-y-h,x,h,w,c,lgop);
}
void rotate270_ellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
		       hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->ellipse)(dest,dx-y-h,x,h,w,c,lgop);
}
void rotate270_fellipse(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,
			hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->fellipse)(dest,dx-y-h,x,h,w,c,lgop);
}
void rotate270_coord_logicalize(s16 *x,s16 *y) {
   s16 ty = *y;
   *y = vid->xres-1-*x;
   *x = ty;
}
void rotate270_coord_physicalize(s16 *x,s16 *y) {
   s16 ty = *y;
   *y = *x;
   *x = vid->xres-1-ty;
}

/******* Special-case wrapper functions */

void rotate270_gradient(hwrbitmap dest,s16 x,s16 y,s16 w,s16 h,s16 angle,
		       pgcolor c1,pgcolor c2,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->gradient)(dest,dx-y-h,x,h,w,angle-270,c1,c2,lgop);
}
void rotate270_slab(hwrbitmap dest,s16 x,s16 y,s16 w,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bar)(dest,dx-1-y,x,w,c,lgop);
}
void rotate270_bar(hwrbitmap dest,s16 x,s16 y,s16 h,hwrcolor c,s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->slab)(dest,dx-y-h,x,h,c,lgop);
}
void rotate270_line(hwrbitmap dest,s16 x1,s16 y1,s16 x2,
		   s16 y2,hwrcolor c, s16 lgop) {
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->line)(dest,dx-1-y1,x1,dx-1-y2,x2,c,lgop);
}
void rotate270_blit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
		   hwrbitmap src,s16 src_x,s16 src_y,
		   s16 lgop) {
   s16 bh,sy2;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bh,&sy2);
   sy2 = bh-(h%bh)-src_y;
   
   (*vid->scrollblit)(dest,dx-dest_y-h,dest_x,h,w,
		      src,sy2,src_x,lgop);
}
void rotate270_scrollblit(hwrbitmap dest,s16 dest_x,s16 dest_y,s16 w, s16 h,
		   hwrbitmap src,s16 src_x,s16 src_y,
		   s16 lgop) {
   s16 bh,sy2;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bh,&sy2);
   sy2 = bh-(h%bh)-src_y;
   
   (*vid->blit)(dest,dx-dest_y-h,dest_x,h,w,
		src,sy2,src_x,lgop);
}
void rotate270_tileblit(hwrbitmap dest,s16 dest_x,s16 dest_y,
		       s16 dest_w,s16 dest_h,
		       hwrbitmap src,s16 src_x,s16 src_y,
		       s16 src_w,s16 src_h,s16 lgop) {
   s16 bw,bh;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);
   (*vid->bitmap_getsize)(src,&bh,&bw);
   (*vid->tileblit)(dest,dx-dest_y-dest_h,dest_x,dest_h,dest_w,
		    src,bh-src_h-src_y,src_x,src_h,src_w,
		    lgop);
}
void rotate270_charblit(hwrbitmap dest,u8 *chardat,s16 dest_x,s16 dest_y,
		       s16 w,s16 h,s16 lines,s16 angle,hwrcolor c,
		       struct quad *clip, s16 lgop) {
   struct quad cr;
   struct quad *crp;
   s16 dx,dy;
   (*vid->bitmap_getsize)(dest,&dx,&dy);

   if (clip) {
      cr.x1 = vid->xres - 1 - clip->y2;
      cr.y1 = clip->x1;
      cr.x2 = vid->xres - 1 - clip->y1;
      cr.y2 = clip->x2;
      crp = &cr;
   }
   else
     crp = NULL;

   /* Rotate the text */
   angle += 270;
   angle %= 360;
   if (angle<0) angle += 360;
   
   (*vid->charblit)(dest,chardat,dx-dest_y,dest_x,w,h,
		    lines,angle,c,crp,lgop);

}

/******* Bitmap rotation */

/* Tack that rotation onto any bitmap loading */

g_error rotate270_bitmap_new(hwrbitmap *bmp,s16 w,s16 h,u16 bpp) {
   return (*vid->bitmap_new)(bmp,h,w,bpp);
}

#ifdef CONFIG_FORMAT_XBM
g_error rotate270_bitmap_loadxbm(hwrbitmap *bmp,
				const u8 *data,
				s16 w,s16 h,
				hwrcolor fg,
				hwrcolor bg) {
   g_error e;
   e = (*vid->bitmap_loadxbm)(bmp,data,w,h,fg,bg);
   errorcheck;
   e = (*vid->bitmap_rotate90)(bmp);
   errorcheck;
   e = (*vid->bitmap_rotate90)(bmp);
   errorcheck;
   return (*vid->bitmap_rotate90)(bmp);
}
#endif

g_error rotate270_bitmap_load(hwrbitmap *bmp,const u8 *data,u32 datalen) {
   g_error e;
   e = (*vid->bitmap_load)(bmp,data,datalen);
   errorcheck;
   e = (*vid->bitmap_rotate90)(bmp);
   errorcheck;
   e = (*vid->bitmap_rotate90)(bmp);
   errorcheck;
   return (*vid->bitmap_rotate90)(bmp);
}

g_error rotate270_bitmap_getsize(hwrbitmap bmp,s16 *w,s16 *h) {
   return (*vid->bitmap_getsize)(bmp,h,w);
}

/* Rotate all loaded bitmaps when this mode is entered/exited */

g_error rotate270_entermode(void) {
   g_error e;
   e = bitmap_iterate(vid->bitmap_rotate90);
   errorcheck;
   e = bitmap_iterate(vid->bitmap_rotate90);
   errorcheck;
   return bitmap_iterate(vid->bitmap_rotate90);
}

g_error rotate270_exitmode(void) {
   g_error e;
   
   /* Rotate 3 more times to get back to normal.
    * This is slower, but maybe saves memory? */
   e = rotate270_entermode();
   errorcheck;
   e = rotate270_entermode();
   errorcheck;
   e = rotate270_entermode();
   errorcheck;
   return success;
}

void rotate270_coord_keyrotate(s16 *k) {
  switch (*k) {
  case PGKEY_UP:    *k = PGKEY_LEFT;  break;
  case PGKEY_RIGHT: *k = PGKEY_UP;    break;
  case PGKEY_DOWN:  *k = PGKEY_RIGHT; break;
  case PGKEY_LEFT:  *k = PGKEY_DOWN;  break;
  }
}

/******* Registration */

void vidwrap_rotate270(struct vidlib *vid) {
   vid->pixel = &rotate270_pixel;
   vid->getpixel = &rotate270_getpixel;
   vid->update = &rotate270_update;
   vid->slab = &rotate270_slab;
   vid->bar = &rotate270_bar;
   vid->line = &rotate270_line;
   vid->rect = &rotate270_rect;
   vid->ellipse = &rotate270_ellipse;
   vid->fellipse = &rotate270_fellipse;
   vid->gradient = &rotate270_gradient;
   vid->blit = &rotate270_blit;
   vid->tileblit = &rotate270_tileblit;
   vid->charblit = &rotate270_charblit;
   vid->scrollblit = &rotate270_scrollblit;
   vid->coord_logicalize = &rotate270_coord_logicalize;
   vid->coord_physicalize = &rotate270_coord_physicalize;
#ifdef CONFIG_FORMAT_XBM
   vid->bitmap_loadxbm = &rotate270_bitmap_loadxbm;
#endif
   vid->bitmap_load = &rotate270_bitmap_load;
   vid->bitmap_new = &rotate270_bitmap_new;
   vid->bitmap_getsize = &rotate270_bitmap_getsize;
   vid->entermode = &rotate270_entermode;
   vid->exitmode = &rotate270_exitmode;
   vid->coord_keyrotate = &rotate270_coord_keyrotate;
}

/* The End */

