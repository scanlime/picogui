/* $Id: videotest.c,v 1.13 2001/05/29 22:31:22 micahjd Exp $
 *
 * videotest.c - implements the -s command line switch, running various
 *               tests on the video driver
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
#include <pgserver/render.h>
#include <pgserver/appmgr.h>
#include <time.h>               /* For benchmarking */

#define NUM_PATTERNS    5
#define TEST_DURATION   2      /* Length of each benchmark run, in seconds */

/************ Line test pattern */

void testpat_line(void) {
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   struct fontdesc *fd;
   int patx,paty,patw;
   int i;
   
   rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,defaultfont);
   
   /* Background */
   VID(rect) (vid->display,0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

   /* Lines 5 pixels from each edge */
   VID(slab) (vid->display,0,5,vid->lxres,fg,PG_LGOP_NONE);
   VID(slab) (vid->display,0,vid->lyres-6,vid->lxres,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,5,0,vid->lyres,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,vid->lxres-6,0,vid->lyres,fg,PG_LGOP_NONE);
   
   /* More lines lining the edges (to test for off-by-one framebuffer bugs) */
   VID(slab) (vid->display,7,0,vid->lxres-14,fg,PG_LGOP_NONE);
   VID(slab) (vid->display,7,vid->lyres-1,vid->lxres-14,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,0,7,vid->lyres-14,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,vid->lxres-1,7,vid->lyres-14,fg,PG_LGOP_NONE);
   
   /* 4x4 rectangles in each corner, to make sure we can address those
    * extremities with ease */
   VID(rect) (vid->display,0,0,4,4,fg,PG_LGOP_NONE);
   VID(rect) (vid->display,vid->lxres-4,0,4,4,fg,PG_LGOP_NONE);
   VID(rect) (vid->display,0,vid->lyres-4,4,4,fg,PG_LGOP_NONE);
   VID(rect) (vid->display,vid->lxres-4,vid->lyres-4,4,4,fg,PG_LGOP_NONE);
   
   /* Horizontal and vertical text labels along the inside of the lines */
   outtext(vid->display,fd,7,7,fg,"PicoGUI Video Test Pattern #1",NULL,
	   0,0,PG_LGOP_NONE,0);
   outtext(vid->display,fd,7,vid->lyres-8,fg,
	   "PicoGUI Video Test Pattern #1",NULL,0,0,PG_LGOP_NONE,90);

   /* Center the test pattern bounding box */
   patw = ((vid->lxres<vid->lyres)?vid->lxres:vid->lyres) -
     24 - fd->fs->normal->h*2;
   patx = (vid->lxres - patw) >> 1;
   paty = (vid->lyres - patw) >> 1;
   
   /* Draw little alignment marks, a 1-pixel gap from the test pattern */
   VID(slab) (vid->display,patx-5,paty,4,fg,PG_LGOP_NONE);
   VID(slab) (vid->display,patx-5,paty+patw,4,fg,PG_LGOP_NONE);
   VID(slab) (vid->display,patx+patw+2,paty,4,fg,PG_LGOP_NONE);
   VID(slab) (vid->display,patx+patw+2,paty+patw,4,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,patx,paty-5,4,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,patx+patw,paty-5,4,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,patx,paty+patw+2,4,fg,PG_LGOP_NONE);
   VID(bar) (vid->display,patx+patw,paty+patw+2,4,fg,PG_LGOP_NONE);

   /* Line thingies within the box */
   for (i=0;i<=patw;i+=4) {
      VID(line) (vid->display,patx+i,paty,patx+patw,paty+i,fg,PG_LGOP_NONE);
      VID(line) (vid->display,patx,paty+i,patx+i,paty+patw,fg,PG_LGOP_NONE);
   }
   outtext(vid->display,fd,(vid->lxres-fd->fs->normal->h)>>1,
	   (vid->lyres-fd->fs->normal->h)>>1,
	   fg,"1",NULL,0,0,PG_LGOP_NONE,0);
}

/************ Color test pattern */

void testpat_color(void) {
   hwrcolor bg = VID(color_pgtohwr) (0x000000);
   hwrcolor fg = VID(color_pgtohwr) (0xFFFFFF);
   struct fontdesc *fd;
   int patx,paty,patw;
   int y=0;
   int h;
   
   rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,defaultfont);
   h = fd->fs->normal->h;
   
   /* Background */
   VID(rect) (vid->display,0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);
   
   outtext(vid->display,fd,0,y,fg,"Black -> White",NULL,0,0,PG_LGOP_NONE,0);
   y+=h;
   VID(gradient) (vid->display,0,y,vid->lxres,h*2,0,0x000000,0xFFFFFF,
		  PG_LGOP_NONE);
   y+=2*h;

   outtext(vid->display,fd,0,y,fg,"White -> Black",NULL,0,0,PG_LGOP_NONE,0);
   y+=h;
   VID(gradient) (vid->display,0,y,vid->lxres,h*2,0,0xFFFFFF,0x000000,
		  PG_LGOP_NONE);
   y+=2*h;

   outtext(vid->display,fd,0,y,fg,"Black -> Red",NULL,0,0,PG_LGOP_NONE,0);
   y+=h;
   VID(gradient) (vid->display,0,y,vid->lxres,h*2,0,0x000000,0xFF0000,
		  PG_LGOP_NONE);
   y+=2*h;
   
   outtext(vid->display,fd,0,y,fg,"Black -> Green",NULL,0,0,PG_LGOP_NONE,0);
   y+=h;
   VID(gradient) (vid->display,0,y,vid->lxres,h*2,0,0x000000,0x00FF00,
		  PG_LGOP_NONE);
   y+=2*h;
   
   outtext(vid->display,fd,0,y,fg,"Black -> Blue",NULL,0,0,PG_LGOP_NONE,0);
   y+=h;
   VID(gradient) (vid->display,0,y,vid->lxres,h*2,0,0x000000,0x0000FF,
		  PG_LGOP_NONE);
   y+=2*h;

   outtext(vid->display,fd,0,y,fg,"Blue -> Red",NULL,0,0,PG_LGOP_NONE,0);
   y+=h;
   VID(gradient) (vid->display,0,y,vid->lxres,h*2,0,0x0000FF,0xFF0000,
		  PG_LGOP_NONE);
   y+=2*h;

}

/************ Blit/unblit test pattern */

void testpat_unblit(void) {
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   struct fontdesc *fd;
   int patx,paty,patw;
   int patxstart;
   int i;
   hwrbitmap bit;
   char buf[20];
   
   rdhandle((void**)&fd,PG_TYPE_FONTDESC,-1,defaultfont);
   
   /* Background */
   VID(rect) (vid->display,0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

   /* test pattern bounding box */
   patw = 50;
   patx = patxstart = 10;
   VID(bar) (vid->display,patw+25,0,vid->lyres,fg,PG_LGOP_NONE);
   
   /* Repeat for different pixel alignments */
   for (paty=10;paty+patw<vid->lyres;paty+=patw+15,patx=++patxstart,patw--) {
      
      /* Draw little alignment marks, a 1-pixel gap from the test pattern */
      VID(slab) (vid->display,patx-5,paty,4,fg,PG_LGOP_NONE);
      VID(slab) (vid->display,patx-5,paty+patw,4,fg, PG_LGOP_NONE);
      VID(slab) (vid->display,patx+patw+2,paty,4,fg, PG_LGOP_NONE);
      VID(slab) (vid->display,patx+patw+2,paty+patw,4,fg, PG_LGOP_NONE);
      VID(bar) (vid->display,patx,paty-5,4,fg, PG_LGOP_NONE);
      VID(bar) (vid->display,patx+patw,paty-5,4,fg, PG_LGOP_NONE);
      VID(bar) (vid->display,patx,paty+patw+2,4,fg, PG_LGOP_NONE);
      VID(bar) (vid->display,patx+patw,paty+patw+2,4,fg, PG_LGOP_NONE);
      
      /* Line thingies within the box */
      VID(rect) (vid->display,patx,paty,patw+1,patw+1,fg, PG_LGOP_NONE);
      VID(rect) (vid->display,patx+1,paty+1,patw-1,patw-1,bg, PG_LGOP_NONE);
      for (i=0;i<=patw;i+=3)
	VID(line) (vid->display,patx+i,paty+patw,patx+patw,paty+i,
		   fg, PG_LGOP_NONE);
      sprintf(buf,"%d/%d",patx&7,patw);
      outtext(vid->display,fd,patx+2,paty+2,fg,buf,NULL,0,0,PG_LGOP_NONE,0);
      
      /* Blit the bounding box */
      VID(bitmap_new) (&bit,patw+1,patw+1);
      VID(blit) (bit,0,0,patw+1,patw+1,vid->display,patx,paty,PG_LGOP_NONE);
      
      /* Same pattern, shifted to the side in various alignments */
      for (patx=patw+40,i=0;patx+patw<vid->lxres;patx=((patx+patw+25)&(~7))+(i++)) {
	 
	 VID(slab) (vid->display,patx-5,paty,4,fg,PG_LGOP_NONE);
	 VID(slab) (vid->display,patx-5,paty+patw,4,fg,PG_LGOP_NONE);
	 VID(slab) (vid->display,patx+patw+2,paty,4,fg,PG_LGOP_NONE);
	 VID(slab) (vid->display,patx+patw+2,paty+patw,4,fg,PG_LGOP_NONE);
	 VID(bar) (vid->display,patx,paty-5,4,fg,PG_LGOP_NONE);
	 VID(bar) (vid->display,patx+patw,paty-5,4,fg,PG_LGOP_NONE);
	 VID(bar) (vid->display,patx,paty+patw+2,4,fg,PG_LGOP_NONE);
	 VID(bar) (vid->display,patx+patw,paty+patw+2,4,fg,PG_LGOP_NONE);
	 
	 VID(blit) (vid->display,patx,paty,patw+1,patw+1,bit,0,0,PG_LGOP_NONE);
      }

      VID(bitmap_free) (bit);
   }
}

/************ Slab test pattern */

void testpat_slab(void) {
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   int i;

   /* Background */
   VID(rect) (vid->display,0,0,vid->lxres,vid->lyres,bg,PG_LGOP_NONE);

   for (i=0;i<16;i++) {
      VID(slab) (vid->display,5+i,5+(i<<1),12,fg,PG_LGOP_NONE);
      VID(slab) (vid->display,35+i,5+(i<<1),8,fg,PG_LGOP_NONE);
      VID(slab) (vid->display,65+i,5+(i<<1),5,fg,PG_LGOP_NONE);

      VID(slab) (vid->display,5,45+(i<<1),i+1,fg,PG_LGOP_NONE);
      VID(slab) (vid->display,35+i,45+(i<<1),i+1,fg,PG_LGOP_NONE);
   }
}

/************ Stipple rectangle test pattern */

void testpat_stipple(void) {
   hwrcolor bg = VID(color_pgtohwr) (0xFFFFFF);
   hwrcolor fg = VID(color_pgtohwr) (0x000000);
   int i;
   int h = vid->lyres/16;

   /* Background */
   VID(rect) (vid->display,0,0,vid->lxres>>1,vid->lyres,bg,PG_LGOP_NONE);
   VID(rect) (vid->display,vid->lxres>>1,0,vid->lxres>>1,vid->lyres,
	      fg,PG_LGOP_NONE);

   for (i=0;i<16;i++) {
      VID(rect) (vid->display,i,i*h,
		vid->lxres-(i<<1),h,i&1 ? fg:bg,PG_LGOP_STIPPLE);

   }
}

/************ Front-end */

void videotest_help(void) {
   puts("\nVideo test modes:\n"
	"\t1\tLine test pattern\n"
   	"\t2\tColor test pattern\n"
	"\t3\tBlit/unblit test pattern\n"
	"\t4\tSlab alignment test pattern\n"
	"\t5\tStippled rectangle test pattern\n"
	"\t99\tAll tests\n"
	"\tnegative value: repeat test in a loop\n"
	);
}


static void videotest_run_one(int number,int update) {
  switch (number) {
  case 1:
    testpat_line();
    break;
  case 2:
    testpat_color();
    break;
  case 3:
    testpat_unblit();
    break;
  case 4:
    testpat_slab();
    break;
  case 5:
    testpat_stipple();
    break;
  default:
    printf("Unknown video test mode");
    exit(1);
  }
  if (update)
     VID(update) (0,0,vid->lxres,vid->lyres);
}


void videotest_run(s16 number) {
  int loop, cycle, nr;
  const int delay = 1;

  loop = number<0;
  number = number<0 ? -number : number;
  cycle = number==99;
  nr = cycle ? 1 : number;

  for(;;) {
    videotest_run_one(nr,1);

    if(loop || cycle) {
      if (sleep(delay))    /* If sleep is interrupted, exit -- micah */
	 exit(0);
      if(cycle) {
	if(++nr>4) {
	  if(!loop) break;
	  else nr = 1;
	}
      }
    }
    else break;
  }
}

/************ Benchmarking */

/* Runs a test, returns FPS 
 * (we could just return a float, but PicoGUI doesn't have any floating
 * point in it and I don't want to start now... Used fixed point to make
 * a string, and return that string.)
 */
const char *videotest_time_one(int number,int update) {
   time_t start,seconds;
   unsigned long frames = 0;
   static char fpsbuf[10];
   
   start = time(NULL);
   do {
      videotest_run_one(number,update);
      frames++;
      seconds = time(NULL) - start;
   } while (seconds < TEST_DURATION);

   sprintf(fpsbuf,"%5d.%02d",frames/seconds,(frames*100/seconds)%100);
   return fpsbuf;
}

/* Run all benchmark tests */
void videotest_benchmark(void) {
   int i;
   
   printf("\nPlease wait... Each test will take %d seconds to complete.\n"
	  "Results are measured in frames per second. Complete includes\n"
	  "screen hardware updates, raw is just the software. Raw should\n"
	  "equal complete if the driver does not double-buffer\n\n",
	  TEST_DURATION*2);
   
   printf(" Test  | Complete / Raw\n"
	  "------------------------\n");
   for (i=1;i<=NUM_PATTERNS;i++) {
      printf(" %5d | %s / ",i,videotest_time_one(i,1));
      printf("%s\n",videotest_time_one(i,0));
   }

   printf("\nDone.\n");
}
      

/* The End */

