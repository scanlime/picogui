/*
 * Filename:  
 * Author:    Brandon Smith
 * Date:      
 * Purpose:   
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

void ai4(struct board *it)
{
  int temp;

  temp = nextmovewin(it);
  if(temp != -1)
  {
    move(it,temp);
    return;
  }

  temp = linetrap(it);
  if(temp != -1)
  {
    move(it,temp);
    return;
  }

  temp = nextmovelose(it,-1);
  if(temp != -1)
  {
    notmove(it,temp);
    return;
  }

  randommove(it);
}

int linetrap(struct board *it)
{
  int x, y;
  int total;
  int i;

  /*horizontal line traps*/
  for(x=1;x<4;x++)
    for(y=0;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y) + glook(it,x+2,y);
      if(total == 2 || total == -2)
	if(glook(it,x-1,y-1) != 0 && glook(it,x-1,y) == 0 &&
	   glook(it,x+3,y-1) != 0 && glook(it,x+3,y) == 0)
	  for(i=0;i<3;i++)
	    if(glook(it,x+i,y) == 0 && glook(it,x+i,y-1) != 0)
	      return x+i;
    }
  
  /*positive slope*/
  
  for(x=1;x<4;x++)
    for(y=1;y<3;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y+1) + glook(it,x+2,y+2);
      if(total == 2 || total == -2)
	if(glook(it,x-1,y-2) != 0 && glook(it,x-1,y-1) == 0 &&
	   glook(it,x+3,y+2) != 0 && glook(it,x+3,y+3) == 0)
	  for(i=0;i<3;i++)
	    if(glook(it,x+i,y+i) == 0 && glook(it,x+i,y-1+i) != 0)
	      return x+i;
    }

  /*negative slope*/

  for(x=1;x<4;x++)
    for(y=3;y<6;y++)
    {
      total = glook(it,x,y) + glook(it,x+1,y-1) + glook(it,x+2,y-2);
      if(total == 2 || total == -2)
	if(glook(it,x-1,y) != 0 && glook(it,x-1,y+1) == 0 &&
	   glook(it,x+3,y-3) != 0 && glook(it,x+3,y-2) == 0)
	  for(i=0;i<3;i++)
	    if(glook(it,x+i,y-i) == 0 && glook(it,x+i,y-1-i) != 0)
	      return x+i;
    }
  return -1;
}
