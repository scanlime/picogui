/* $Id: checkbox.c,v 1.4 2001/08/30 15:05:09 micahjd Exp $
 *
 * checkbox.c - a customized button, used for "check boxes"
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
#include <pgserver/widget.h>

g_error checkbox_install(struct widget *self) {
  g_error e;

  /* Start with a button */
  e = button_install(self);
  errorcheck;

  /* Customize */
  customize_button(self,PGTH_O_CHECKBOX,PGTH_O_CHECKBOX_ON,
		   PGTH_O_CHECKBOX_HILIGHT,PGTH_O_CHECKBOX_ON_NOHILIGHT,
		   NULL,NULL);

  /* Use alternate click event handling to toggle */
  widget_set(self,PG_WP_EXTDEVENTS,PG_EXEV_TOGGLE);

  /* Stack vertically */
  widget_set(self,PG_WP_SIDE,PG_S_TOP);

  return sucess;
}

/* The End */



