/* $Id: font.h,v 1.21 2002/01/18 16:42:59 pney Exp $
 *
 * font.h - structures for representing fonts
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

#ifndef __H_FONT
#define __H_FONT

struct fontdesc;
struct fontstyle_node;
struct font;

extern struct fontstyle_node *fontstyles;   /* Head of a linked list
					       of font styles */

#include <pgserver/handle.h>
#include <pgserver/video.h>

typedef long stylet;

/* A description of a particular instance of a font - the font structure
   and parameters that effect how it is rendered
*/
struct fontdesc {
  struct font *font;
  struct fontstyle_node *fs;   /* The fontstyle this was generated from */
  u32 style;            /* Store the style that created this fontdesc */
  int interline_space;  /* Extra spacing between lines, for
			   doublespace, etc. */
  int interchar_space;  /* Extra spacing between characters,
			   for expanded text */
  int margin;
  int boldw;    /* If nonzero, font will be offset and duplicated this
		   many times to produce bold.  Must use lgop == PG_LGOP_OR
		   for this to work. */
  int hline;    /* If this is non-negative, draw a horizontal line at this many
		   pixels down from the top of the font. Can be used to
		   create underline, overline, or strikeout. */
  int skew;     /* If nonzero, an italic is generated by shearing the
		   image horizontally one pixel for every 'skew' vertical
		   pixels */
  int italicw;  /* Extra width added by the italic */
  int passwdc;  /* Password char. If null or positive, define the returned
		   character to display text in a 'password way'. */
  int (*decoder)(const u8 **str);   /* Text decoder (for Unicode, etc) */
  void *extra;  /* Extra driver-specific data */
};

/* This is a description that goes along with a font style.
   It indicates its name, size, bold, italic, etc.
   This is searched to produce a fontdesc. */
struct fontstyle_node {
  const u8 *name;
  int size;
  long flags;
  struct fontstyle_node *next;

  /* Various versions of this font. If one of these is NULL it can be
   * synthesized from the above information */
  struct font *normal;  /* The only required one */
  struct font *bold;
  struct font *italic;
  struct font *bolditalic;

  int boldw;    /* Width of a bold if a bold version is not available */
};

/* One glyph in a font */
struct fontglyph {
  s32 encoding;
  u32 bitmap;   /* Offset to bitmap data                  */
  s16 dwidth;   /* Delta X between this char and the next */
  s16 w,h;      /* Width and height of the glyph bitmap   */
  s16 x,y;      /* X and Y displacement for the glyph     */
};

/* An individual bitmapped font */
struct font {
  u16 numglyphs;    /* Total number of glyphs in the table */
  u16 defaultglyph; /* Default glyph, when the requested glyph isn't found */
  s16 w,h;          /* Font size (pixels) */            
  s16 ascent;       /* Distance glyphs will extend above the baseline */
  s16 descent;      /* Distance glyphs will extend below the baseline */

  struct fontglyph const *glyphs;   /* Table of glyphs */
  u8 const *bitmaps;                /* Chunk of bitmap data for the font */
};

/******** Functions */

/* Copy a character to the screen, producing any special effects as
 * requested by the fontdesc.
 */
void outchar(hwrbitmap dest, struct fontdesc *fd,
	     s16 *x, s16 *y,hwrcolor col,int c,struct quad *clip,
	     s16 lgop, s16 angle);

/* These functions interpret the '\n' character, but no other control
 * chars
 */
void outtext(hwrbitmap dest, struct fontdesc *fd,
	     s16 x,s16 y,hwrcolor col, const u8 *txt,struct quad *clip,
	     s16 lgop, s16 angle);
void sizetext(struct fontdesc *fd, s16 *w, s16 *h, const u8 *txt);

/* Find a font with specified characteristics, and prepare
 * a fontdesc structure for it.  The closest font will be matched.
 * Any of the FSTYLE_* flags can be used to indicate that attribute
 * is required. */
g_error findfont(handle *pfh,int owner, const u8 *name,int size,stylet flags);

/* Decode one character from the specified UTF-8 string, 
 * advancing the pointer.
 * If it decodes an invalid character, it returns -1
 */
int decode_utf8(const u8 **str);

/* Very simple decoder for ASCII and ISO Latin-1 */
int decode_ascii(const u8 **str);

/* Utility to do a binary search for a font glyph */
const struct fontglyph *font_findglyph(const struct fontglyph *start, 
				 const struct fontglyph *end, s32 key);

#endif /* __H_FONT */

/* The End */

