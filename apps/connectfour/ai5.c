/*
 * Filename:  ai5.c
 * Author:    Brandon Smith
 * Date:      April 3, 2002
 * Purpose:   the Fifth level AI
 *
 * Copyright (C) 2002 Brandon Smith <lottabs2@yahoo.com> 
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
 *
 */

#include "connectfour.h"
#include "ai.h"
#include "ai1.h"
#include "ai2.h"
#include "ai3.h"
#include "ai4.h"
#include "ai5.h"

#define DEBUG
#define FUNCTION_DEBUG

void ai5(struct board *it)
{
  int temp;

  temp = nextmovewin(it);
#ifdef DEBUG
  fprintf(stderr,"nextmovewin returned %d\n",temp);
#endif
  if(temp != -1)
  {
    move(it,temp);
    return;
  }

  temp = linetrapwin(it);
#ifdef DEBUG
  fprintf(stderr,"linetrapwin returned %d\n",temp);
#endif
  if(temp != -1)
  {
    move(it,temp);
    return;
  }

  temp = gentrap(it);
#ifdef DEBUG
  fprintf(stderr,"gentrap returned %d\n",temp);
#endif
  if(temp != -1)
  {
    move(it,temp);
    return;
  }
  
  temp = linetraplose(it);
#ifdef DEBUG
  fprintf(stderr,"linetraplose returned %d\n",temp);
#endif
  if(temp != -1)
  {
    move(it,temp);
    return;
  }

  temp = nextmovelose(it,-1);
#ifdef DEBUG
  fprintf(stderr,"nextmovelose returned %d\n",temp);
#endif
  if(temp != -1)
  {
    notmove(it,temp);
    return;
  }

  prandmove(it);
}

int gentrap(struct board *it)
{
  int temp;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"gentrap called\n");
#endif
  
  temp = gentrapwin(it);
  if(temp != -1)
    return temp;
  else
    return gentraplose(it);
}

int gentrapwin(struct board *it)
{

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"gentrapwin called\n");
#endif

  return -1;
}

int gentraplose(struct board *it)
{

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"gentraplose called\n");
#endif
  
  return -1;
}

int spotwin(struct board *it, int x, int y, int player)
{
  int i,j;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"spotwin called - x = %d, y = %d, player = %d\n",x,y,player);
#endif

  for(i-0;i<3;i++)
  {
    /* Horizontal */
    if((gmask(it,x,8,x-2+i,y)+gmask(it,x,8,x-1+i,y)+gmask(it,x,8,x+i,y)) == player * 2)
      for(j=0;j<5;j++)
	if(gmask(it,x,8,x-2+i,y) == 0 && gmask(it,x,8,x-2+i,y-1) != 0)
	  return maskout(x,x-2+i);
    /* positive slope */
    if((gmask(it,x,y,x-2+i,y-2+i)+gmask(it,x,y,x-1+i,y-1+i)+gmask(it,x,y,x+i,y+i)) == player*2)
      for(j=0;j<5;j++)
	if(gmask(it,x,y,x-2+i,y-2+i) == 0 && gmask(it,x,y,x-2+i,(y-2+i)-1) != 0)
	  return maskout(x,x-2+i);
    /* negative slope */
    if((gmask(it,x,y,x-2+i,y+2-i)+gmask(it,x,y,x-1+i,y+1-i)+gmask(it,x,y,x+i,y-i)) == player*2)
      for(j=0;j<5;j++)
	if(gmask(it,x,y,x-2+i,y+2-i) == 0 && gmask(it,x,y,x-2+i,(y+2-i)-1) != 0)
	  return maskout(x,x-2+i);
  }
  return -1;
}

/*this masks an X and Y thing out of a board, so that the caller doesn't "see" it */
int gmask(struct board *it, int maskx, int masky, int x, int y)
{
#ifdef FUNCTION_DEBUG
  fprintf(stderr,"gmask called - maskx = %d, masky = %d, x = %d, y = %d\n",maskx,masky,x,y);
#endif

  if(x >= maskx) x++;
  if(y >= masky) y++;
  return glook(it,x,y);
}

int maskout(int mask, int val)
{

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"maskout called\n");
#endif

  if(val >= mask)
    val++;
  return val;
}

void prandmove(struct board *it)
{
  int i = 0;

#ifdef FUNCTION_DEBUG
  fprintf(stderr,"prandmove called\n");
#endif

  while(++i < 5)
    if(!(move(it,rand()%3 + it->aipref) < 0))
      return;
  
  randommove(it);
}
