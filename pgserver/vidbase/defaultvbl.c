/* $Id: defaultvbl.c,v 1.34 2001/04/05 03:32:25 micahjd Exp $
 *
 * Video Base Library:
 * defaultvbl.c - Maximum compatibility, but has the nasty habit of
 *                pushing around individual pixels. Use if your device
 *                is nothing like a linear framebuffer.
 *                 
 *                This does, however, have sensable implementations
 *                for things like stdbitmap and loading the clipping
 *                registers, so other drivers may snatch a few functions
 *                here and there.
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
#include <pgserver/font.h>

/******* Table of available bitmap formats */

struct bitformat bitmap_formats[] = {

#ifdef CONFIG_FORMAT_PNM
     { {'P','N','M',0}, &pnm_detect, &pnm_load, NULL },
#endif
   
     { {0,0,0,0}, NULL, NULL, NULL }
};

/******* no-op functions */

g_error def_setmode(int xres,int yres,int bpp,unsigned long flags) {
  return sucess;
}

void def_font_newdesc(struct fontdesc *fd) {
}

void emulate_dos(void) {
}

g_error def_enterexitmode(void) {
   return sucess;
}

void def_update(int x,int y,int w,int h) {
}

void def_coord_logicalize(int *x,int *y) {
}

/******* colors */

hwrcolor def_color_pgtohwr(pgcolor c) {
  if (vid->bpp<8) {
    /* grayscale */
    return (((getred(c)*3+getgreen(c)*6+getblue(c))/10) >>
	    (8-vid->bpp))&((1<<vid->bpp)-1);
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */
    return ((getred(c) & 0xC0) |
	    ((getgreen(c) >> 2) & 0x38) |
	    ((getblue(c) >> 5) & 0x07));
  }
  else if (vid->bpp==16) {
    /* 5-6-5 color */
    return (((getred(c) << 8) & 0xF800) |
	    ((getgreen(c) << 3) & 0x07E0) |
	    ((getblue(c) >> 3) & 0x001F));
  }
  else
    /* True color */
    return c;
}

pgcolor def_color_hwrtopg(hwrcolor c) {
  if (vid->bpp<8) {
    /* grayscale */
    return (((getred(c)*3+getgreen(c)*6+getblue(c))/10) >>
	    (8-vid->bpp))&((1<<vid->bpp)-1);
  }
  else if (vid->bpp==8) {
    /* 2-3-3 color */
    return ((getred(c) & 0xC0) |
	    ((getgreen(c) >> 2) & 0x38) |
	    ((getblue(c) >> 5) & 0x07));
  }
  else if (vid->bpp==16) {
    /* 5-6-5 color */
    return (((getred(c) << 8) & 0xF800) |
	    ((getgreen(c) << 3) & 0x07E0) |
	    ((getblue(c) >> 3) & 0x001F));
  }
  else
    /* True color */
    return c;
}

void def_addpixel(int x,int y,pgcolor c) {
  /* Blech. Convert it to an RGB first for the proper effect.
     Don't let the RGB components roll over either
  */
  int r,g,b;
  pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(x,y));

  r = getred(oc);
  g = getgreen(oc);
  b = getblue(oc);
  r += getred(c);
  g += getgreen(c);
  b += getblue(c);
  
  if (r>255) r=255;
  if (g>255) g=255;
  if (b>255) b=255;

  (*vid->pixel) (x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)));
}

void def_subpixel(int x,int y,pgcolor c) {
  /* Same idea, but subtract */
  int r,g,b;
  pgcolor oc = (*vid->color_hwrtopg) ((*vid->getpixel)(x,y));

  r = getred(oc);
  g = getgreen(oc);
  b = getblue(oc);
  r -= getred(c);
  g -= getgreen(c);
  b -= getblue(c);
  
  if (r<0) r=0;
  if (g<0) g=0;
  if (b<0) b=0;

  (*vid->pixel) (x,y,(*vid->color_pgtohwr)(mkcolor(r,g,b)));
}

/* Should be more than OK for most situations */
void def_clear(void) {
  (*vid->rect) (0,0,vid->xres,vid->yres,(*vid->color_pgtohwr)(0));
}

void def_slab(int x,int y,int w,hwrcolor c) {
  /* You could make this create a very thin rectangle, but then if niether
   * were implemented they would be a pair of mutually recursive functions! */
   
  for (;w;w--,x++)
     (*vid->pixel) (x,y,c);
}

void def_bar(int x,int y,int h,hwrcolor c) {
  (*vid->rect) (x,y,1,h,c);
}

/* There are about a million ways to optimize this-
   At the very least, combine it with pixel() and
   have it keep track of the y coord as a memory offset
   to get rid of the multiply in pixel().

   The old SDL driver used this optimization, but it
   was too driver-specific, so had to revert to the
   generic bresenham's for the default implementation.
*/
void def_line(int x1,int y1,int x2,int y2,hwrcolor c) {
  int stepx, stepy;
  int dx = x2-x1;
  int dy = y2-y1;
  int fraction;

  dx = x2-x1;
  dy = y2-y1;

  if (dx<0) { 
    dx = -(dx << 1);
    stepx = -1; 
  } else {
    dx = dx << 1;
    stepx = 1;
  }
  if (dy<0) { 
    dy = -(dy << 1);
    stepy = -1; 
  } else {
    dy = dy << 1;
    stepy = 1;
  }

  (*vid->pixel) (x1,y1,c);

  /* Major axis is horizontal */
  if (dx > dy) {
    fraction = dy - (dx >> 1);
    while (x1 != x2) {
      if (fraction >= 0) {
	y1 += stepy;
	fraction -= dx;
      }
      x1 += stepx;
      fraction += dy;
      
      (*vid->pixel) (x1,y1,c);
    }
  } 
  
  /* Major axis is vertical */
  else {
    fraction = dx - (dy >> 1);
    while (y1 != y2) {
      if (fraction >= 0) {
	x1 += stepx;
	fraction -= dy;
      }
      y1 += stepy;
      fraction += dx;
      
      (*vid->pixel) (x1,y1,c);
    }
  }
}

void def_rect(int x,int y,int w,int h,hwrcolor c) {
  for (;h;h--,y++) (*vid->slab) (x,y,w,c);
}

void def_gradient(int x,int y,int w,int h,int angle,
		  pgcolor c1, pgcolor c2,int translucent) {
  /*
    The angle is expressed in degrees.
    If translucent is positive it will add the gradient to the existing
    pixels. If negative, it will subtract.  If it is zero, it performs
    a normal overwrite operation.
    
    This implementation is based on the function:
    color = y*sin(angle) + x*cos(angle)
    with scaling added to keep color between the specified values.
    The main improvement (?) is that it has been munged to use
    fixed-point calculations, and only addition and shifting in the
    inner loop.
   
    Wow, algebra and trigonometry are useful for something!  ;-)

    Unless the device has hardware-accelerated gradients (!) the
    set-up will be about the same.  Optimizations could include the usual:
       Use framebuffer offset for y
       Put pixel code directly in here
       Assembler
  */

  /* Lotsa vars! */
  long r_vs,g_vs,b_vs,r_sa,g_sa,b_sa,r_ca,g_ca,b_ca,r_ica,g_ica,b_ica;
  long r_vsc,g_vsc,b_vsc,r_vss,g_vss,b_vss,sc_d;
  long r_v1,g_v1,b_v1,r_v2,g_v2,b_v2;
  int i,s,c,x1;

  /* Look up the sine and cosine */
  angle %= 360;
  if (angle<0) angle += 360;
  if      (angle <= 90 ) s =  trigtab[angle];
  else if (angle <= 180) s =  trigtab[180-angle];
  else if (angle <= 270) s = -trigtab[angle-180];
  else                   s = -trigtab[360-angle];
  angle += 90;
  if (angle>=360) angle -= 360;
  if      (angle <= 90 ) c =  trigtab[angle];
  else if (angle <= 180) c =  trigtab[180-angle];
  else if (angle <= 270) c = -trigtab[angle-180];
  else                   c = -trigtab[360-angle];

  /* Calculate denominator of the scale value */
  sc_d = h*((s<0) ? -((long)s) : ((long)s)) +
    w*((c<0) ? -((long)c) : ((long)c));

  /* Decode colors */
  r_v1 = getred(c1);
  g_v1 = getgreen(c1);
  b_v1 = getblue(c1);
  r_v2 = getred(c2);
  g_v2 = getgreen(c2);
  b_v2 = getblue(c2);

  /* Calculate the scale values and 
   * scaled sine and cosine (for each channel) */
  r_vs = ((r_v2<<16)-(r_v1<<16)) / sc_d;
  g_vs = ((g_v2<<16)-(g_v1<<16)) / sc_d;
  b_vs = ((b_v2<<16)-(b_v1<<16)) / sc_d;

  /* Zero the accumulators */
  r_sa = g_sa = b_sa = r_ca = g_ca = b_ca = r_ica = g_ica = b_ica = 0;

  /* Calculate the sine and cosine scales */
  r_vsc = (r_vs*((long)c)) >> 8;
  r_vss = (r_vs*((long)s)) >> 8;
  g_vsc = (g_vs*((long)c)) >> 8;
  g_vss = (g_vs*((long)s)) >> 8;
  b_vsc = (b_vs*((long)c)) >> 8;
  b_vss = (b_vs*((long)s)) >> 8;

  /* If the scales are negative, start from the opposite side */
  if (r_vss<0) r_sa  = -r_vss*h;
  if (r_vsc<0) r_ica = -r_vsc*w; 
  if (g_vss<0) g_sa  = -g_vss*h;
  if (g_vsc<0) g_ica = -g_vsc*w; 
  if (b_vss<0) b_sa  = -b_vss*h;
  if (b_vsc<0) b_ica = -b_vsc*w; 

  if (r_v2<r_v1) r_v1 = r_v2;
  if (g_v2<g_v1) g_v1 = g_v2;
  if (b_v2<b_v1) b_v1 = b_v2;

  /* Finally, the loop! */

  if (translucent==0) {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
      for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	   i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {

	(*vid->pixel) (x1,y,(*vid->color_pgtohwr)
		      (mkcolor(
			       r_v1 + ((r_ca+r_sa) >> 8),
			       g_v1 + ((g_ca+g_sa) >> 8),
			       b_v1 + ((b_ca+b_sa) >> 8))));
      }
  }
  else if (translucent>0) {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
      for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	   i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {

	(*vid->addpixel) (x1,y,mkcolor(
				      r_v1 + ((r_ca+r_sa) >> 8),
				      g_v1 + ((g_ca+g_sa) >> 8),
				      b_v1 + ((b_ca+b_sa) >> 8)));
      }
  }
  else {
    for (;h;h--,r_sa+=r_vss,g_sa+=g_vss,b_sa+=b_vss,y++)
      for (x1=x,r_ca=r_ica,g_ca=g_ica,b_ca=b_ica,i=w;i;
	   i--,r_ca+=r_vsc,g_ca+=g_vsc,b_ca+=b_vsc,x1++) {

	(*vid->subpixel) (x1,y,mkcolor(
				      r_v1 + ((r_ca+r_sa) >> 8),
				      g_v1 + ((g_ca+g_sa) >> 8),
				      b_v1 + ((b_ca+b_sa) >> 8)));
      }
  }
}

void def_dim(int x,int y,int w,int h) {
  int i,xx;

  /* 'Cute' little checkerboard thingy. Devices with high/true color
   * or even 256 color should make this do real dimming to avoid
   * looking stupid  ;-)
   */
  for (;h;h--,y++)
    for (xx=x+(h&1),i=w;i;i--,xx+=2)
      (*vid->pixel) (xx,y,0);
}

void def_scrollblit(int src_x,int src_y,
		    int dest_x,int dest_y,
		    int w,int h) {
  
  /* This shouldn't be _too_ bad in most cases...
     The pixels within a line are as fast as blit()
     but this introduces function call overhead between lines
  */

  for (src_y+=h-1,dest_y+=h-1;h;h--,src_y--,dest_y--)
    (*vid->blit) (NULL,src_x,src_y,dest_x,dest_y,w,1,PG_LGOP_NONE);
}

void def_charblit(unsigned char *chardat,int dest_x,
		      int dest_y,int w,int h,int lines,
		      hwrcolor c,struct cliprect *clip) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || dest_y>clip->y2 || (dest_x+w)<clip->x1 || 
      (dest_y+h)<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y1>dest_y) {
      hc = clip->y1-dest_y; /* Do it this way so skewing doesn't mess up when clipping */
      dest_y += hc;
      chardat += hc*bw;
    }
    if (clip->y2<(dest_y+h))
      h = clip->y2-dest_y+1;
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x1>dest_x)
      xmin = clip->x1-dest_x;
    if (clip->x2<(dest_x+w))
      xmax = clip->x2-dest_x+1;
  }

  for (;hc<h;hc++,dest_y++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x++,xpix++)
	if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	  (*vid->pixel) (x,dest_y,c); 
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

/* Like charblit, but rotate 90 degrees anticlockwise whilst displaying
 *
 * As this code was mostly copied from linear8, it probably has the same
 * subtle "smudging" bug noted in linear8.c
 */
void def_charblit_v(unsigned char *chardat,int dest_x,
		  int dest_y,int w,int h,int lines,
		  hwrcolor c,struct cliprect *clip) {
  int bw = w;
  int iw,hc,y;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x>clip->x2 || (dest_y-w)>clip->y2 || (dest_x+h)<clip->x1 || 
      dest_y<clip->y1)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->x1>dest_x) {
      hc = clip->x1-dest_x; /* Do it this way so skewing doesn't mess up when clipping */
      dest_x += hc;
      chardat += hc*bw;
    }
    if (clip->x2<(dest_x+h-1))
      h = clip->x2-dest_x+1;
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->y1>dest_y-w+1)
      xmax = 1+dest_y-clip->y1;
    if (clip->y2<(dest_y))
      xmin = dest_y-clip->y2;
  }

  for (;hc<h;hc++,dest_x++) {
    if (olines && lines==hc) {
      lines += olines;
      dest_y++;
      flag=1;
    }
    for (iw=bw,y=dest_y,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,y--,xpix++)
	if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	  (*vid->pixel) (dest_x,y,c);
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}

/* Upside-down version of character blit (only needed for rotation) */
#ifdef CONFIG_ROTATE
void def_charblit_u(unsigned char *chardat,int dest_x,
		      int dest_y,int w,int h,int lines,
		      hwrcolor c,struct cliprect *clip) {
  int bw = w;
  int iw,hc,x;
  int olines = lines;
  int bit;
  int flag=0;
  int xpix,xmin,xmax;
  unsigned char ch;

  /* Is it at all in the clipping rect? */
  if (clip && (dest_x<clip->x1 || dest_y<clip->y1 || (dest_x-w)>clip->x2 || 
      (dest_y-h)>clip->y2)) return;

  /* Find the width of the source data in bytes */
  if (bw & 7) bw += 8;
  bw = bw >> 3;
  xmin = 0;
  xmax = w;
  hc = 0;

  /* Do vertical clipping ahead of time (it does not require a special case) */
  if (clip) {
    if (clip->y2<dest_y) {
      hc = dest_y-clip->y2; /* Do it this way so skewing doesn't mess up when clipping */
      dest_y -= hc;
      chardat += hc*bw;
    }
    if (clip->y1>(dest_y-h))
      h = dest_y-clip->y1+1;
    
    /* Setup for horizontal clipping (if so, set a special case) */
    if (clip->x2<dest_x)
      xmin = dest_x-clip->x2;
    if (clip->x1>(dest_x-w))
      xmax = dest_x-clip->x1+1;
  }

  for (;hc<h;hc++,dest_y--) {
    if (olines && lines==hc) {
      lines += olines;
      dest_x--;
      flag=1;
    }
    for (x=dest_x,iw=bw,xpix=0;iw;iw--)
      for (bit=8,ch=*(chardat++);bit;bit--,ch=ch<<1,x--,xpix++)
	if (ch&0x80 && xpix>=xmin && xpix<xmax) 
	  (*vid->pixel) (x,dest_y,c); 
    if (flag) {
      xmax++;
      flag=0;
    }
  }
}
#endif /* CONFIG_ROTATE */

#ifdef CONFIG_FORMAT_XBM
g_error def_bitmap_loadxbm(struct stdbitmap **bmp,
			   unsigned char *data,
			   int w,int h,
			   hwrcolor fg,
			   hwrcolor bg) {
  int i,bit;
  unsigned char c;
  unsigned char *p,*pline;
  g_error e;
  int shift;

  e = (*vid->bitmap_new) ((hwrbitmap *) bmp,w,h);
  errorcheck;
  p = pline = (*bmp)->bits;
   
  /* Shift in the pixels! */
  for (;h;h--,p=pline+=(*bmp)->pitch)
    for (bit=0,i=w;i>0;i--) {
      if (!bit) c = *(data++);
      
      switch (vid->bpp) {

	/* We don't need to check for byte-rollover here because
	   it will always occur on an even boundary.
	   (Unless we add a 3bpp or a 7bpp mode or something
	   really freaky like that)
	*/

      case 1:
      case 2:
      case 4:
        *p = 0;
	for (shift=8-vid->bpp;shift && i;shift-=vid->bpp) {
	   *p |= ((c&1) ? fg : bg) << shift;
	   c >>= 1;
	   bit++;
	   i--;
	}
	if (i)
	   *(p++) |= ((c&1) ? fg : bg);
	break;

      case 8:
	*(p++) = (c&1) ? fg : bg;
	break;

      case 16:
	*(((unsigned short *)p)++) = (c&1) ? fg : bg;
	break;

      case 24:
	if (c&1) {
	  *(p++) = (u8) fg;
	  *(p++) = (u8) (fg >> 8);
	  *(p++) = (u8) (fg >> 16);
	}
	else {
	  *(p++) = (u8) bg;
	  *(p++) = (u8) (bg >> 8);
	  *(p++) = (u8) (bg >> 16);
	}
	break;

      case 32:
	*(((unsigned long *)p)++) = (c&1) ? fg : bg;
	break;

#if DEBUG_VIDEO
	/* Probably not worth the error-checking time
	   in non-debug versions, as it would be caught
	   earlier (hopefully?)
	*/

      default:
	printf("Converting to unsupported BPP\n");
#endif

      }

      c >>= 1;
      bit++;
      bit &= 7;
    }

  return sucess;
}
#endif /* CONFIG_FORMAT_XBM */

/* Try loading a bitmap using each available format */
g_error def_bitmap_load(hwrbitmap *bmp,u8 *data,u32 datalen) {
   struct bitformat *fmt = bitmap_formats;
   
   while (fmt->name[0]) {   /* Dummy record has empty name */
      if (fmt->detect && fmt->load && (*fmt->detect)(data,datalen))
	return (*fmt->load)(bmp,data,datalen);
      fmt++;
   }
   return mkerror(PG_ERRT_BADPARAM,8); /* Format not recognized by any loaders */
}

/* 90 degree anticlockwise rotation */
g_error def_bitmap_rotate90(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit;
   u8 *src,*srcline,*dest;
   int oshift,shift,mask;
   int shiftset  = 8-vid->bpp;
   int subpixel  = ((8/vid->bpp)-1);
   int subpixel2 = ((1<<vid->bpp)-1);
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap with width/height reversed */
   srcbit = *bmp;
   e = (*vid->bitmap_new)(&destbit,srcbit->h,srcbit->w);
   errorcheck;
   
   src = srcline = srcbit->bits;
   for (h=srcbit->h,x=y=0;h;h--,y++,src=srcline+=srcbit->pitch) {

      /* Per-line mask calculations for <8bpp destination blits */
      if (vid->bpp<8) {
	 shift = (subpixel-(y&subpixel)) * vid->bpp;
	 mask  = subpixel2<<shift;
      }
      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {
	 
	 /* Read in a pixel */
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    c = ((*src) >> oshift) & ((1<<vid->bpp)-1);
	    if (!oshift) {
	       oshift = shiftset;
	       src++;
	    }
	    else
	      oshift -= vid->bpp;
	    break; 
	    
	  case 8:
	    c = *(src++);
	    break;
	    
	  case 16:
	    c = *(((u16*)src)++);
	    break;
	    
	  case 24:
	    c = (*(src++)) | ((*(src++))<<8) | ((*(src++))<<16);
	    break;
	    
	  case 32:
	    c = *(((u32*)src)++);
	    break;     
	 }
	 
	 /* Plot the pixel */
	 dest = destbit->bits + ((y*vid->bpp)>>3) + 
	   (srcbit->w-1-x)*destbit->pitch;
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    *dest &= ~mask;
	    *dest |= (c << shift) & mask;
	    break; 
	    
	  case 8:
	    *dest = c;
	    break;
	    
	  case 16:
	    *((u16*)dest) = c;
	    break;
	    
	  case 24:
	    *(dest++) = (u8) c;
	    *(dest++) = (u8) (c >> 8);
	    *dest     = (u8) (c >> 16);
	    break;
	    
	  case 32:
	    *((u32*)dest) = c;
	    break;     
	 }	
      }   
   }
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return sucess;
}

g_error def_bitmap_new(struct stdbitmap **bmp,
		       int w,int h) {
  g_error e;
  int lw;
   
  /* The bitmap and the header can be allocated seperately,
     but usually it is sufficient to make them one big
     chunk of memory.  It's 1 alloc vs 2.
  */

  /* Pad the line width up to the nearest byte */
  lw = (unsigned long) w * vid->bpp;
  if ((vid->bpp<8) && (lw & 7))
     lw += 8;
  lw >>= 3;
  /* The +1 is to make blits for < 8bpp simpler. Shouldn't really
   * be necessary, though. FIXME */
  e = g_malloc((void **) bmp,sizeof(struct stdbitmap) + (lw * h) + 1);
  errorcheck;

  (*bmp)->pitch = lw;
  (*bmp)->freebits = 0;
  (*bmp)->bits = ((unsigned char *)(*bmp)) + 
    sizeof(struct stdbitmap);
  (*bmp)->w = w;
  (*bmp)->h = h;
  
  return sucess;
}

void def_bitmap_free(struct stdbitmap *bmp) {
  if (!bmp) return;
  if (bmp->freebits) g_free(bmp->bits);
  g_free(bmp);
}

g_error def_bitmap_getsize(struct stdbitmap *bmp,int *w,int *h) {
  *w = bmp->w;
  *h = bmp->h;
}

#ifndef min
#define min(a,b) (((a)<(b))?(a):(b))
#endif

void def_tileblit(struct stdbitmap *src,
		  int src_x,int src_y,int src_w,int src_h,
		  int dest_x,int dest_y,int dest_w,int dest_h) {
  int i,j;

  /* Do a tiled blit */
  for (i=0;i<dest_w;i+=src_w)
    for (j=0;j<dest_h;j+=src_h)
      (*vid->blit) (src,src_x,src_y,dest_x+i,dest_y+j,min(dest_w-i,src_w),
		   min(dest_h-j,src_h),PG_LGOP_NONE);
}

/* Scary blit... Very bad for normal screens, but then again so is most of this
 * VBL. Could be helpful on odd devices like ncurses or some LCDs */
void def_blit(struct stdbitmap *srcbit,int src_x,int src_y,
	      int dest_x, int dest_y,
	      int w, int h, int lgop) {
   struct stdbitmap screen;
   int i;
   char *src,*srcline;
   hwrcolor c,s;
   u8 shiftset = 8-vid->bpp;
   u8 oshift;
   
   if (srcbit && (w>(srcbit->w-src_x) || h>(srcbit->h-src_y))) {
      int i,j;
      
      /* Do a tiled blit */
      for (i=0;i<w;i+=srcbit->w)
	for (j=0;j<h;j+=srcbit->h)
	  def_blit(srcbit,0,0,dest_x+i,dest_y+j,min(srcbit->w,w-i),min(srcbit->h,h-j),lgop);
    
      return;
   }
   
   /* Screen-to-screen blit */
   if (!srcbit) {
      srcbit = &screen;
      screen.bits = NULL;
      screen.w = vid->xres;
      screen.h = vid->yres;
   }
   
   src = srcline = srcbit->bits + ((src_x*vid->bpp)>>3) + src_y*srcbit->pitch;
   
   for (;h;h--,src_y++,dest_y++,src=srcline+=srcbit->pitch)
     for (oshift=shiftset,i=0;i<w;i++) {
	if (lgop!=PG_LGOP_NONE)
	  s = (*vid->getpixel) (dest_x+i,dest_y);

	if (srcbit->bits)
	  switch (vid->bpp) {

	   case 1:
	   case 2:
	   case 4:
	     c = ((*src) >> oshift) & ((1<<vid->bpp)-1);
	     if (!oshift) {
		oshift = shiftset;
		src++;
	     }
	     else
	       oshift -= vid->bpp;
	     break; 
	   
	   case 8:
	     c = *(src++);
	     break;
	     
	   case 16:
	     c = *(((unsigned short *)src)++);
	     break;
	     
	   case 24:
	     c = (*(src++)) | ((*(src++))<<8) | ((*(src++))<<16);
	     break;
	     
	   case 32:
	     c = *(((unsigned long *)src)++);
	     break;     
	  }
	else
	  c = (*vid->getpixel) (src_x+i,src_y);

	switch (lgop) {
	 case PG_LGOP_NONE:       s  = c;  break;
	 case PG_LGOP_OR:         s |= c;  break;
	 case PG_LGOP_AND:        s &= c;  break;
	 case PG_LGOP_XOR:        s ^= c;  break;
	 case PG_LGOP_INVERT:     s  = c ^ 0xFFFFFFFF;  break;
	 case PG_LGOP_INVERT_OR:  s |= c ^ 0xFFFFFFFF;  break;
	 case PG_LGOP_INVERT_AND: s &= c ^ 0xFFFFFFFF;  break;
	 case PG_LGOP_INVERT_XOR: s ^= c ^ 0xFFFFFFFF;  break;
	}

	(*vid->pixel) (dest_x+i,dest_y,s);
     }
}

/* Another scary blit for desperate situations */
void def_unblit(int src_x,int src_y,
		struct stdbitmap *destbit,int dest_x,int dest_y,
		int w,int h) {
   int i;
   char *dest = destbit->bits;
   char *destline = dest;
   hwrcolor c;
   int shiftset = 8-vid->bpp;
   int oshift;
   
   for (;h;h--,src_y++,dest=destline+=destbit->pitch)
     for (oshift=shiftset,i=0;i<w;i++) {
	c = (*vid->getpixel) (src_x+i,src_y);

	switch (vid->bpp) {

	 case 1:
	 case 2:
	 case 4:
	   if (oshift==shiftset)
	     *dest = c << oshift;
	   else
	     *dest |= c << oshift;
	   if (!oshift) {
	      oshift = shiftset;
	      dest++;
	   }
	   else
	     oshift -= vid->bpp;
	   break;

	 case 8:
	   *(dest++) = c;
	   break;
	   
	 case 16:
	   *(((unsigned short *)dest)++) = c;
	   break;
	   
	 case 24:
	   *(dest++) = (unsigned char) c;
	   *(dest++) = (unsigned char) (c >> 8);
	   *(dest++) = (unsigned char) (c >> 16);
	   break;
	   	 
	 case 32:
	   *(((unsigned long *)dest)++) = c;
	   break;
	   
	}
      
   }
}

void def_sprite_show(struct sprite *spr) {
#ifdef DEBUG_VIDEO
   printf("def_sprite_show\n");
#endif
   
  if (spr->onscreen || !spr->visible || !vid->xres) return;
   
  /* Clip to a divnode */
  if (spr->clip_to) {
    if (spr->x < spr->clip_to->x) spr->x = spr->clip_to->x;
    if (spr->y < spr->clip_to->y) spr->y = spr->clip_to->y;
    if (spr->x+spr->w > spr->clip_to->x+spr->clip_to->w)
      spr->x = spr->clip_to->x + spr->clip_to->w - spr->w;
    if (spr->y+spr->h > spr->clip_to->y+spr->clip_to->h)
      spr->y = spr->clip_to->y + spr->clip_to->h - spr->h;

    spr->ow = spr->w; spr->oh = spr->h;
  }
  else {
    /* Clip to screen edge, cursor style. For correct mouse cursor
     * functionality and for sanity. 
     */
    if (spr->x<0) 
       spr->x = 0;
    else if (spr->x>(vid->lxres-1))
       spr->x = vid->lxres-1;
     
    if (spr->y<0)
       spr->y = 0;
    else if (spr->y>(vid->lyres-1))
       spr->y = vid->lyres-1;
    
    spr->ow = vid->lxres - spr->x;
    if (spr->ow > spr->w) spr->ow = spr->w;
    spr->oh = vid->lyres - spr->y;
    if (spr->oh > spr->h) spr->oh = spr->h;     
  }

  /* Update coordinates */
  spr->ox = spr->x; spr->oy = spr->y;
  
  /* Grab a new backbuffer */
  VID(unblit) (spr->x,spr->y,spr->backbuffer,
  	       0,0,spr->ow,spr->oh);

  /* Display the sprite */
  if (spr->mask && *spr->mask) {
     VID(blit) (*spr->mask,0,0,
		spr->x,spr->y,spr->ow,spr->oh,PG_LGOP_AND);
     VID(blit) (*spr->bitmap,0,0,
		spr->x,spr->y,spr->ow,spr->oh,PG_LGOP_OR);
  }
   else
     VID(blit) (*spr->bitmap,0,0,
		spr->x,spr->y,spr->ow,spr->oh,PG_LGOP_NONE);
   
  add_updarea(spr->x,spr->y,spr->ow,spr->oh);

  spr->onscreen = 1;
   
   /**** Debuggative Cruft - something I used to test line clipping ****/
/*
    {
	int xp[] = {
	   55,-5,-55,5,30,0,-30,0
	};
	int yp[] = {
	   5,55,-5,-55,0,-30,0,30
	};
	struct divnode d;
	struct gropnode g;
	int i;
	memset(&d,0,sizeof(d));
	memset(&g,0,sizeof(g));
	d.x = 100;
	d.y = 100;
	d.w = 93;
	d.h = 72;
	d.grop = &g;
	g.type = PG_GROP_LINE;
	g.param[0] = (*vid->color_pgtohwr) (0xFFFF00);
	VID(rect) (d.x,d.y,d.w,d.h,(*vid->color_pgtohwr)(0x004000));
	g.x = spr->x-d.x;
	g.y = spr->y-d.y;
	for (i=0;i<8;i++) {
	   g.x += g.w; 
	   g.y += g.h;
	   g.w = xp[i];
	   g.h = yp[i];
	   grop_render(&d);
	}
	VID(update) (d.x,d.y,d.w,d.h);
     }
*/

   /**** A very similar debuggative cruft to test text clipping ****/
/*
    {
      struct cliprect cr;
      struct fontdesc fd;

      memset(&fd,0,sizeof(fd));
      fd.fs = fontstyles;
      fd.font = fd.fs->normal;
      fd.hline = -1;
 
      cr.x1 = 100;
      cr.y1 = 100;
      cr.x2 = 150;
      cr.y2 = 150;
      VID(rect) (cr.x1,cr.y1,cr.x2-cr.x1+1,cr.y2-cr.y1+1,(*vid->color_pgtohwr)(0x004000));
//      outtext(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"Hello,\nWorld!",&cr);
      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"Hello,\nWorld!",&cr);
//      outtext_v(&fd,spr->x,spr->y,(*vid->color_pgtohwr) (0xFFFF80),"E",&cr);
      VID(update) (0,0,vid->lxres,vid->lyres);
    }
*/
   
}

void def_sprite_hide(struct sprite *spr) {
  static struct cliprect cr;

#ifdef DEBUG_VIDEO
   printf("def_sprite_hide\n");
#endif
   
  if ( (!spr->onscreen) ||
       (spr->ox == -1) )
     return;

  cr.x1 = spr->x;
  cr.y1 = spr->y;
  cr.x2 = spr->x+spr->w-1;
  cr.y2 = spr->y+spr->h-1;
   
  /* Protect that area of the screen */
  def_sprite_protectarea(&cr,spr->next);
   
  /* Put back the old image */
  VID(blit) (spr->backbuffer,0,0,
	     spr->ox,spr->oy,spr->ow,spr->oh,PG_LGOP_NONE);
  add_updarea(spr->ox,spr->oy,spr->ow,spr->oh);

  spr->onscreen = 0;
}

void def_sprite_update(struct sprite *spr) {
#ifdef DEBUG_VIDEO
   printf("def_sprite_update\n");
#endif
   
  (*vid->sprite_hide) (spr);
  (*vid->sprite_show) (spr);

  /* Redraw */
  realize_updareas();
}

/* Traverse first -> last, showing sprites */
void def_sprite_showall(void) {
  struct sprite *p = spritelist;

#ifdef DEBUG_VIDEO
   printf("def_sprite_showall\n");
#endif

   while (p) {
    (*vid->sprite_show) (p);
    p = p->next;
  }
}

/* Traverse last -> first, hiding sprites */
void r_spritehide(struct sprite *s) {
  if (!s) return;
  r_spritehide(s->next);
  (*vid->sprite_hide) (s);
}
void def_sprite_hideall(void) {
  r_spritehide(spritelist);
}

/* Hide necessary sprites in a given area */
void def_sprite_protectarea(struct cliprect *in,struct sprite *from) {
   /* Base case: from is null */
   if (!from) return;

#ifdef DEBUG_VIDEO
   printf("def_sprite_protectarea\n");
#endif

   /* Load this all on the stack so we go backwards */
   def_sprite_protectarea(in,from->next);
   
   /* Hide this sprite if necessary */
   if ( ((from->x+from->w) >= in->x1) &&
        ((from->y+from->h) >= in->y1) &&
        (from->x <= in->x2) &&
        (from->y <= in->y2) )
     (*vid->sprite_hide) (from);
}

/* Optional
 *   This is called for every bitmap when entering a new bpp or loading
 *   a new driver. Converts a bitmap from a linear array of 32-bit
 *   pgcolor values to the hwrcolors for this mode
 *
 * Default implementation: stdbitmap
 */
g_error def_bitmap_modeconvert(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit;
   u32 *src;
   u8 *destline,*dest;
   int oshift;
   int shiftset  = 8-vid->bpp;
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap at our present bpp */
   srcbit = *bmp;
   e = (*vid->bitmap_new)(&destbit,srcbit->w,srcbit->h);
   errorcheck;

   src = (u32 *) srcbit->bits;
   dest = destline = destbit->bits;
   
   for (h=srcbit->h,x=y=0;h;h--,y++,dest=destline+=destbit->pitch) {      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {	 
	 /* Read in a pixel */
	 c = (*vid->color_pgtohwr)(*(src++));
	 
	 /* Output in new device bpp */
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    if (oshift==shiftset)
	      *dest = c << oshift;
	    else
	      *dest |= c << oshift;
	    if (!oshift) {
	       oshift = shiftset;
	       dest++;
	    }
	    else
	      oshift -= vid->bpp;
	    break;
	    
	  case 8:
	    *(((unsigned char *)dest)++) = c;
	    break;
	    
	  case 16:
	    *(((unsigned short *)dest)++) = c;
	    break;
	    
	  case 24:
	    *(dest++) = (unsigned char) c;
	    *(dest++) = (unsigned char) (c >> 8);
	    *(dest++) = (unsigned char) (c >> 16);
	    break;
	    
	  case 32:
	    *(((unsigned long *)dest)++) = c;
	    break;
	 }
      }   
   }
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return sucess;
}
   
/* Optional
 *   The reverse of bitmap_modeconvert, this converts the bitmap from
 *    the hardware-specific format to a pgcolor array
 * 
 * Default implementation: stdbitmap
 */
g_error def_bitmap_modeunconvert(struct stdbitmap **bmp) {
   struct stdbitmap *destbit,*srcbit;
   u8 *src,*srcline;
   u32 *dest;  /* Must be word-aligned, this is always the case if
		* struct stdbitmap is padded correctly */
   int oshift;
   int shiftset  = 8-vid->bpp;
   g_error e;
   int h,i,x,y;
   hwrcolor c;
   
   /* New bitmap at 32bpp (this is hackish, but I don't see anything
    * seriously wrong with it... yet...) 
    */
   srcbit = *bmp;
   i = vid->bpp;
   vid->bpp = 32;
   e = (*vid->bitmap_new)(&destbit,srcbit->w,srcbit->h);
   vid->bpp = i;
   errorcheck;
   
   src = srcline = srcbit->bits;
   dest = destbit->bits;
   
   for (h=srcbit->h,x=y=0;h;h--,y++,src=srcline+=srcbit->pitch) {      
      for (oshift=shiftset,i=srcbit->w,x=0;i;i--,x++) {
	 
	 /* Read in a pixel */
	 switch (vid->bpp) {
	  case 1:
	  case 2:
	  case 4:
	    c = ((*src) >> oshift) & ((1<<vid->bpp)-1);
	    if (!oshift) {
	       oshift = shiftset;
	       src++;
	    }
	    else
	      oshift -= vid->bpp;
	    break; 
	    
	  case 8:
	    c = *(src++);
	    break;
	    
	  case 16:
	    c = *(((u16*)src)++);
	    break;
	    
	  case 24:
	    c = (*(src++)) | ((*(src++))<<8) | ((*(src++))<<16);
	    break;
	    
	  case 32:
	    c = *(((u32*)src)++);
	    break;     
	 }
	 
	 *(dest++) = (*vid->color_hwrtopg)(c);
      }   
   }
   
   /* Clean up */
   *bmp = destbit;
   (*vid->bitmap_free)(srcbit);
   return sucess;
}
   
/* Load our driver functions into a vidlib */
void setvbl_default(struct vidlib *vid) {
  /* Set defaults */
  vid->color_pgtohwr = &def_color_pgtohwr;
  vid->color_hwrtopg = &def_color_hwrtopg;
  vid->font_newdesc = &def_font_newdesc;
  vid->addpixel = &def_addpixel;
  vid->subpixel = &def_subpixel;
  vid->clear = &def_clear;
  vid->slab = &def_slab;
  vid->bar = &def_bar;
  vid->line = &def_line;
  vid->rect = &def_rect;
  vid->gradient = &def_gradient;
  vid->dim = &def_dim;
  vid->scrollblit = &def_scrollblit;
  vid->charblit = &def_charblit;
  vid->charblit_v = &def_charblit_v;
#ifdef CONFIG_ROTATE
  vid->charblit_u = &def_charblit_u;
#endif
  vid->tileblit = &def_tileblit;
#ifdef CONFIG_FORMAT_XBM
  vid->bitmap_loadxbm = &def_bitmap_loadxbm;
#endif
  vid->bitmap_load = &def_bitmap_load;
  vid->bitmap_new = &def_bitmap_new;
  vid->bitmap_free = &def_bitmap_free;
  vid->bitmap_getsize = &def_bitmap_getsize;
  vid->sprite_show = &def_sprite_show;
  vid->sprite_hide = &def_sprite_hide;
  vid->sprite_update = &def_sprite_update;
  vid->sprite_showall = &def_sprite_showall;
  vid->sprite_hideall = &def_sprite_hideall;
  vid->sprite_protectarea = &def_sprite_protectarea;
  vid->blit = &def_blit;
  vid->unblit = &def_unblit;
  vid->coord_logicalize = &def_coord_logicalize;
  vid->bitmap_rotate90 = &def_bitmap_rotate90;
  vid->entermode = &def_enterexitmode;
  vid->exitmode = &def_enterexitmode;
  vid->bitmap_modeconvert = &def_bitmap_modeconvert;
  vid->bitmap_modeunconvert = &def_bitmap_modeunconvert;
}

/* The End */

