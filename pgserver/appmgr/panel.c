/* $Id: panel.c,v 1.4 2003/01/01 03:42:56 micahjd Exp $
 *
 * panel.c - Traditional panel/panelbar application manager
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000-2003 Micah Dowty <micahjd@users.sourceforge.net>
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
#include <pgserver/appmgr.h>

/* The boundary between toolbars and app panels */
handle appmgr_panel_tbboundary;


/**************************************** Public interface */
 
g_error appmgr_panel_init(void) {
  struct widget *bgwidget;
  g_error e;
  
  e = dts_new();
  errorcheck;

  /* Make the background widget */
  e = widget_create(&bgwidget,&res[PGRES_BACKGROUND_WIDGET],
		    PG_WIDGET_BACKGROUND,dts->root, 0, -1);
  errorcheck;
  e = widget_attach(bgwidget, dts->root, &dts->root->head->next,0);
  errorcheck;

  /* Turn off the background's DIVNODE_UNDERCONSTRUCTION flags */
  activate_client_divnodes(-1);
  
  return success;
}

void appmgr_panel_shutdown(void) {
  /* Delete the background widget */
  handle_free(-1, res[PGRES_BACKGROUND_WIDGET]);
}

g_error appmgr_panel_reg(struct app_info *i) {
  struct widget *w = NULL, *tbb = NULL;
  struct divtree *tree;
  g_error e;

  /* Dereference the toolbar boundary */
  if (iserror(rdhandle((void**) &tbb,PG_TYPE_WIDGET,-1,appmgr_panel_tbboundary)))
    tbb = NULL;  

  /* Allocate root widget, do any setup specific to the app type */
  switch (i->type) {

  case PG_APP_TOOLBAR:
    /* Create a simple toolbar as a root widget. Create it after the previous
     * toolbar, if applicable, and update htbboundary. */
    if (tbb) {
      e = widget_derive(&w,&i->rootw,PG_WIDGET_TOOLBAR,tbb,
			appmgr_panel_tbboundary,PG_DERIVE_AFTER,i->owner);
    }
    else {
      e = widget_create(&w,&i->rootw,PG_WIDGET_TOOLBAR,dts->root, 0, i->owner);
      errorcheck;
      e = widget_attach(w,dts->root,&dts->root->head->next,0);
      errorcheck;
    }

    /* The toolbar boundary is now after this widget */
    appmgr_panel_tbboundary = i->rootw;

    /* Size specs are ignored for the toolbar.
     * They won't be moved by the appmgr, so sidemask has no effect.
     * Set the side here, though.
     */
    e = widget_set(w,PG_WP_SIDE,i->side);
    errorcheck;

    w->isroot = 1;

    /* If there is a popup in the nontoolbar area, we need to update all layers
     * and reclip the popups. This is necessary because, for example, the user
     * may want to turn on a virtual keyboard while in a dialog box.
     */
    if (popup_toolbar_passthrough()) {
      for (tree=dts->top;tree;tree=tree->next) {
	tree->head->flags |= DIVNODE_NEED_RECALC;
	tree->flags |= DIVTREE_NEED_RECALC | DIVTREE_ALL_REDRAW | 
	  DIVTREE_CLIP_POPUP;
      }
    }
    
    break;

  case PG_APP_NORMAL:
    /* Put the new app right after the toolbar boundary */
    if (tbb) {
      e = widget_derive(&w,&i->rootw,PG_WIDGET_PANEL,tbb,
			appmgr_panel_tbboundary,PG_DERIVE_AFTER,i->owner);
    }
    else {
      e = widget_create(&w,&i->rootw,PG_WIDGET_PANEL,dts->root, 0, i->owner);
      errorcheck;
      e = widget_attach(w,dts->root,&dts->root->head->next,0);
      errorcheck;
    }

    /* Set all the properties */
    e = widget_set(w,PG_WP_TEXT,i->name);
    errorcheck;
    e = widget_set(w,PG_WP_SIDE,i->side);
    errorcheck;
    e = widget_set(w,PG_WP_SIZE,(i->side & (PG_S_LEFT|PG_S_RIGHT)) ? 
		   i->default_size.w : i->default_size.h);
    errorcheck;

    w->isroot = 1;
    break;

  default:
    return mkerror(PG_ERRT_BADPARAM,30);  /* Nonexistant app type */
  }

  return success;
}
	
struct divnode *appmgr_panel_nontoolbar_area(void) {
  struct widget *w;

  /* Recalculate the root divtree, necessary if we just changed video modes,
   * or if the toolbars have moved since the last update and a popup is being
   * created */
  divnode_recalc(&dts->root->head,NULL);

  /* Dereference the toolbar boundary */
  if (iserror(rdhandle((void**) &w,PG_TYPE_WIDGET,-1,appmgr_panel_tbboundary)))
    w = NULL;

  if (!w)
    return dts->root->head;

  return w->in->next;
}

/**************************************** Registration */

struct appmgr appmgr_panel = {
             name:  "panel",
             init:  appmgr_panel_init,
         shutdown:  appmgr_panel_shutdown,
              reg:  appmgr_panel_reg,
  nontoolbar_area:  appmgr_panel_nontoolbar_area,
};

/* The End */



