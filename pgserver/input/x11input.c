/* $Id: x11input.c,v 1.1 2001/11/19 18:46:24 micahjd Exp $
 *
 * x11input.h - input driver for X11 events
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
#include <pgserver/input.h>
#include <pgserver/widget.h>
#include <pgserver/pgnet.h>
#include <pgserver/configfile.h>
#include <X11/Xlib.h>
#include <X11/Xutil.h>

/* Global X display- defined and initialized in the x11 driver */
extern Display *xdisplay;

/* We figure out the x11 connection's file descriptor */
int x11_fd;

g_error x11input_init(null) {
  /* Get a file descriptor for the X11 display */
  if (!xdisplay)
    return mkerror(PG_ERRT_BADPARAM,36);   /* No matching video driver */
  x11_fd = ConnectionNumber(xdisplay);
  return sucess;
}

void x11input_close(void) {
  /* The X11 video driver handles closing down everything... */
}

void x11input_fd_init(int *n,fd_set *readfds,struct timeval *timeout) {
  if ((*n)<(x11_fd+1))
    *n = x11_fd+1;
  FD_SET(x11_fd,readfds);
}

int x11input_fd_activate(int fd) {
  int i;
  XEvent xevent;

  if(fd != x11_fd) return 0;
  i = XPending(xdisplay);

  /* Recieve and process X events */
  for (;i;i--) {
    XNextEvent(xdisplay, &xevent);


    printf("Event\n");

  }

  return 1;
}

g_error x11input_regfunc(struct inlib *i) {
        i->init = &x11input_init;
        i->close = &x11input_close;
        i->fd_init = &x11input_fd_init;
        i->fd_activate = &x11input_fd_activate;
        return sucess;
}

/* The End */
