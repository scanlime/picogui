/* $Id: ncurses.c,v 1.18 2001/05/13 06:04:33 micahjd Exp $
 *
 * ncurses.c - ncurses driver for PicoGUI. This lets PicoGUI make
 *             nice looking and functional text-mode GUIs.
 *             Note that this driver ignores fonts and bitmaps.
 *             Because units are now in characters, not pixels,
 *             you should probably load a theme designed for this.
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
#include <pgserver/input.h>
#include <pgserver/appmgr.h>
#include <pgserver/font.h>
#include <pgserver/render.h>

#include <curses.h>
#include <gpm.h>

/* Buffer with the current status of the screen */
chtype *ncurses_screen;

/* The most recent mouse event, exported by ncursesinput */
extern Gpm_Event ncurses_last_event;

/******************************************** Fake font */
/* This is a little hack to trick PicoGUI's text rendering */

/* Font width table: all chars are 1 character wide! */
u8 const ncurses_font_vwtab[256] = {
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
     1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

/* Table of indices to the bitmap table.
 * Each character is one character long, so this is simple.
 */
u32 const ncurses_font_trtab[256] = {
   0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
   0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
   0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
   0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
   0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
   0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
   0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
   0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
   0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
   0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
   0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
   0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
   0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
   0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
   0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

/* Table of font 'bitmaps', really just their character code.
 */
u8 const ncurses_font_bitmaps[256] = {
   0x00,0x01,0x02,0x03,0x04,0x05,0x06,0x07,0x08,0x09,0x0A,0x0B,0x0C,0x0D,0x0E,0x0F,
   0x10,0x11,0x12,0x13,0x14,0x15,0x16,0x17,0x18,0x19,0x1A,0x1B,0x1C,0x1D,0x1E,0x1F,
   0x20,0x21,0x22,0x23,0x24,0x25,0x26,0x27,0x28,0x29,0x2A,0x2B,0x2C,0x2D,0x2E,0x2F,
   0x30,0x31,0x32,0x33,0x34,0x35,0x36,0x37,0x38,0x39,0x3A,0x3B,0x3C,0x3D,0x3E,0x3F,
   0x40,0x41,0x42,0x43,0x44,0x45,0x46,0x47,0x48,0x49,0x4A,0x4B,0x4C,0x4D,0x4E,0x4F,
   0x50,0x51,0x52,0x53,0x54,0x55,0x56,0x57,0x58,0x59,0x5A,0x5B,0x5C,0x5D,0x5E,0x5F,
   0x60,0x61,0x62,0x63,0x64,0x65,0x66,0x67,0x68,0x69,0x6A,0x6B,0x6C,0x6D,0x6E,0x6F,
   0x70,0x71,0x72,0x73,0x74,0x75,0x76,0x77,0x78,0x79,0x7A,0x7B,0x7C,0x7D,0x7E,0x7F,
   0x80,0x81,0x82,0x83,0x84,0x85,0x86,0x87,0x88,0x89,0x8A,0x8B,0x8C,0x8D,0x8E,0x8F,
   0x90,0x91,0x92,0x93,0x94,0x95,0x96,0x97,0x98,0x99,0x9A,0x9B,0x9C,0x9D,0x9E,0x9F,
   0xA0,0xA1,0xA2,0xA3,0xA4,0xA5,0xA6,0xA7,0xA8,0xA9,0xAA,0xAB,0xAC,0xAD,0xAE,0xAF,
   0xB0,0xB1,0xB2,0xB3,0xB4,0xB5,0xB6,0xB7,0xB8,0xB9,0xBA,0xBB,0xBC,0xBD,0xBE,0xBF,
   0xC0,0xC1,0xC2,0xC3,0xC4,0xC5,0xC6,0xC7,0xC8,0xC9,0xCA,0xCB,0xCC,0xCD,0xCE,0xCF,
   0xD0,0xD1,0xD2,0xD3,0xD4,0xD5,0xD6,0xD7,0xD8,0xD9,0xDA,0xDB,0xDC,0xDD,0xDE,0xDF,
   0xE0,0xE1,0xE2,0xE3,0xE4,0xE5,0xE6,0xE7,0xE8,0xE9,0xEA,0xEB,0xEC,0xED,0xEE,0xEF,
   0xF0,0xF1,0xF2,0xF3,0xF4,0xF5,0xF6,0xF7,0xF8,0xF9,0xFA,0xFB,0xFC,0xFD,0xFE,0xFF
};

struct font const ncurses_font = {
   /* bitmaps = */ (u8*) &ncurses_font_bitmaps,
   /* h = */ 1,
   /* hspace = */ 0,
   /* vspace = */ 0,
   /* vwtab = */ (u8*) &ncurses_font_vwtab,
   /* trtab = */ (u32*) &ncurses_font_trtab
};

/* Bogus fontstyle node */
struct fontstyle_node ncurses_font_style = {
   /* name = */ "Ncurses Pseudofont",
   /* size = */ 1,
   /* flags = */ PG_FSTYLE_FIXED,
   /* next = */ NULL,
   /* normal = */ (struct font *) &ncurses_font,
   /* bold = */ NULL,
   /* italic = */ NULL,
   /* bolditalic = */ NULL,
   /* ulineh = */ 1,
   /* slineh = */ 0,
   /* boldw = */ 0
};
        
/******************************************** Implementations */

g_error ncurses_init(void) {
   chtype rgbmap[] = { COLOR_BLACK, COLOR_BLUE, COLOR_GREEN, COLOR_CYAN, 
	COLOR_RED, COLOR_MAGENTA, COLOR_YELLOW, COLOR_WHITE };
   int f,b;
   g_error e;
   unsigned long size;
   unsigned long *p;

   /* Load the ncursesinput driver, and let it initialize
    * ncurses and gpm for us. */
   e = load_inlib(&ncursesinput_regfunc,&inlib_main);
   errorcheck;
   
   /* Set colors */
   for (b=0;b<8;b++)
     for (f=0;f<8;f++)
       init_pair((b<<3) + f,rgbmap[f],rgbmap[b]);
   
   /* Save the actual video mode */
   vid->xres = COLS;
   vid->yres = LINES;
   vid->bpp  = sizeof(chtype)*8;    /* Our pixel is a curses chtype */
   vid->display = NULL;
   
   /* Allocate our buffer */
   e = g_malloc((void**) &ncurses_screen,vid->xres * vid->yres * sizeof(chtype));
   errorcheck;
   for (p=ncurses_screen,size=vid->xres*vid->yres;size;size--,p++)
     *p = ' ';
   
   return sucess;
}

void ncurses_close(void) {
   /* Take out our input driver */
   unload_inlib(inlib_main);

   g_free(ncurses_screen);
}

void ncurses_pixel(hwrbitmap dest,s16 x,s16 y,hwrcolor c,s16 lgop) {
   if (dest || (lgop!=PG_LGOP_NONE))
     def_pixel(dest,x,y,c,lgop);
   else
     mvaddch(y,x,ncurses_screen[x + vid->xres * y] = c);
}

hwrcolor ncurses_getpixel(hwrbitmap src,s16 x,s16 y) {
   if (src)
     return def_getpixel(src,x,y);
   else
     return ncurses_screen[x + vid->xres * y];
}

void ncurses_update(s16 x,s16 y,s16 w,s16 h) {
   refresh();

   /* Show the cursor */
   GPM_DRAWPOINTER(&ncurses_last_event);
}

/**** Hack the normal font rendering a bit so we use regular text */

void ncurses_font_newdesc(struct fontdesc *fd, char *name,
			  int size, stylet flags) {
   fd->font = (struct font *) &ncurses_font;
   fd->margin = 0;
   fd->hline = -1;
   fd->italicw = 0;
   fd->fs = &ncurses_font_style;
}

void ncurses_charblit(hwrbitmap dest, u8 *chardat,s16 dest_x,
		      s16 dest_y,s16 w,s16 h,s16 lines,s16 angle,
		      hwrcolor c,struct quad *clip,bool fill, hwrcolor bg,
		      s16 lgop) {
   chtype *location;
   
   if (lgop != PG_LGOP_NONE) {
      def_charblit(dest,chardat,dest_x,dest_y,w,h,lines,angle,c,clip,fill,bg,
		   lgop);
      return;
   }
   
   /* Make sure we're within clip */
   if (clip && (dest_x<clip->x1 || dest_y<clip->y1 ||
		dest_x>clip->x2 || dest_y>clip->y2))
     return;
   
   /* Get the previous contents, strip out all but the background,
    * and add our new foreground and text */
   location = ncurses_screen + dest_x + vid->xres * dest_y;
   *location = COLOR_PAIR((PAIR_NUMBER(*location & (~A_CHARTEXT)) & 0x38) | 
			  ((PAIR_NUMBER(c) & 0x38)>>3)) | (c & A_BOLD) | (*chardat);
     
   /* Send it */
   mvaddch(dest_y,dest_x,*location);
}

/**** We use a ncurses character cell as our hwrcolor */

/* This can handle input colors in different formats, always returning
 * a ncurses attribute value */

hwrcolor ncurses_color_pgtohwr(pgcolor c) {

   if (c & 0x20000000) {
      /* Normal character:  0x20BBFFCC
       * BB = background
       * FF = foreground
       * CC = character
       */
      
      return COLOR_PAIR( ((c & 0x070000)>>13) | ((c & 0x000700)>>8) ) |
	((c & 0x000800) ? A_BOLD : 0) | (c & 0x0000FF);
   }
   
   else if (c & 0x40000000) {
      /* ACS character:  0x40BBFFCC
       * BB = background
       * FF = foreground
       * CC = ACS character code
       */
      
      return COLOR_PAIR( ((c & 0x070000)>>13) | ((c & 0x000700)>>8) ) |
	((c & 0x000800) ? A_BOLD : 0) | acs_map[c & 0x0000FF];
   }

   else {
      /* RGB value interpreted as a background attribute */
      
      int sc = 7;
      if ((c & 0xFF0000) > 0x400000) sc |= 32;
      if ((c & 0x00FF00) > 0x004000) sc |= 16;
      if ((c & 0x0000FF) > 0x000040) sc |= 8;
      return (COLOR_PAIR(sc) | ( ((c&0xFF0000) > 0xA00000) || 
				 ((c&0x00FF00) > 0x00A000) || 
				 ((c&0x0000FF) > 0x0000A0) ? A_BOLD : 0)) | ' ';
   }
   
}

/**** A hack to turn off the picogui sprite cursor */

void ncurses_sprite_show(struct sprite *spr) {
   if (spr==cursor) {
      spr->visible = 0;
    
      /* Do it ourselves */
      GPM_DRAWPOINTER(&ncurses_last_event);
   }
      
   def_sprite_show(spr);
}

/******************************************** Driver registration */

g_error ncurses_regfunc(struct vidlib *v) {
   setvbl_default(v);
   
   v->init = &ncurses_init;
   v->close = &ncurses_close;
   
   v->pixel = &ncurses_pixel;
   v->getpixel = &ncurses_getpixel;
   v->update = &ncurses_update;  
   v->color_pgtohwr = &ncurses_color_pgtohwr;
   
   v->font_newdesc = &ncurses_font_newdesc;
   v->charblit = &ncurses_charblit;

   v->sprite_show = &ncurses_sprite_show;
   
   return sucess;
}

/* The End */
