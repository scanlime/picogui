/* $Id: widget.c,v 1.88 2001/07/25 00:51:46 micahjd Exp $
 *
 * widget.c - defines the standard widget interface used by widgets, and
 * handles dispatching widget events and triggers.
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
#include <pgserver/pgnet.h>
#include <pgserver/hotspot.h>

/* Table of widgets */
#ifdef RUNTIME_FUNCPTR
struct widgetdef widgettab[PG_WIDGETMAX+1];
void widgettab_init(void) {
   struct widgetdef *p = widgettab;
#else
struct widgetdef widgettab[] = {
#endif
   
DEF_STATICWIDGET_TABLE(toolbar)
DEF_STATICWIDGET_TABLE(label)
DEF_WIDGET_TABLE(scroll)
DEF_STATICWIDGET_TABLE(indicator)
DEF_STATICWIDGET_TABLE(bitmap)
DEF_WIDGET_TABLE(button)
	       
#ifdef CONFIG_NOPANELBAR
DEF_STATICWIDGET_TABLE(panel)
#else
DEF_WIDGET_TABLE(panel)
#endif
		 
DEF_WIDGET_TABLE(popup)
DEF_STATICWIDGET_TABLE(box)
DEF_WIDGET_TABLE(field)
DEF_WIDGET_TABLE(background)
DEF_HYBRIDWIDGET_TABLE(menuitem,button)

#ifdef CONFIG_WIDGET_TERMINAL
DEF_WIDGET_TABLE(terminal)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,102))
#endif

#ifdef CONFIG_WIDGET_CANVAS
DEF_WIDGET_TABLE(canvas)
#else
DEF_ERRORWIDGET_TABLE(mkerror(PG_ERRT_BADPARAM,103))
#endif
				     
DEF_HYBRIDWIDGET_TABLE(checkbox,button)
DEF_HYBRIDWIDGET_TABLE(flatbutton,button)
};

/* These are needed to determine which widget is under the pointing
   device, keep track of status */
struct divnode *div_under_crsr;
struct widget *under;
struct widget *prev_under;
int prev_btn;
struct widget *capture;
struct widget *kbdfocus;
int capturebtn;           /* Button that is holding the capture */

/* Sorted (chronological order) list of widgets
   with timers
*/
struct widget *timerwidgets;

/* Set to the client # if a client has taken over the resource */
int keyboard_owner;
int pointer_owner;
int sysevent_owner;
   
int timerlock = 0;
/******** Widget interface functions */

g_error widget_create(struct widget **w,int type,
		      struct divtree *dt,struct divnode **where,
		      handle container,int owner) {
  g_error e;

  if ((type > PG_WIDGETMAX) || (!dt) || (!where)) return 
      mkerror(PG_ERRT_BADPARAM,20);

#ifdef DEBUG_KEYS
  num_widgets++;
#endif
  e = g_malloc((void **)w,sizeof(struct widget));
  errorcheck;
  memset(*w,0,sizeof(struct widget));

  (*w)->owner = owner;
  (*w)->type = type;
  (*w)->def = widgettab + type;
  (*w)->dt = dt;
  (*w)->container = container;

  if ((*w)->def->install)
     (*(*w)->def->install)(*w);
   else {
      /* This widget is not supported, return the error code we
       * conveniently crammed into the 'remove' field */

      g_free(*w);
      return (g_error) (*w)->def->remove;
   }
   
  /* Add the widget to the divtree */
  *(*w)->out = *where;
  *where = (*w)->in;
  (*w)->where = where;
  if (*(*w)->out && (*(*w)->out)->owner)
    (*(*w)->out)->owner->where = (*w)->out;
     
  /* Resize for the first time */
  resizewidget(*w);

  dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
  dt->flags |= DIVTREE_NEED_RECALC;
  return sucess;
}

g_error widget_derive(struct widget **w,
		      int type,struct widget *parent,
		      handle hparent,int rship,int owner) {
  switch (rship) {

  case PG_DERIVE_INSIDE:
    return widget_create(w,type,parent->dt,parent->sub,hparent,owner);

  case PG_DERIVE_AFTER:
    return widget_create(w,type,parent->dt,parent->out,parent->container,owner);

  case PG_DERIVE_BEFORE:
  case PG_DERIVE_BEFORE_OLD:
    return widget_create(w,type,parent->dt,parent->where,parent->container,owner);
   
  default:
    return mkerror(PG_ERRT_BADPARAM,22);

  }
}

/* Used internally */
void remove_from_timerlist(struct widget *w) {
  if (timerwidgets) {
    if (w==timerwidgets) {
      timerwidgets = w->tnext;
    }
    else {
      struct widget *p = timerwidgets;
      while (p->tnext)
	if (p->tnext == w) {
	  /* Take us out */
	  p->tnext = w->tnext;
	  break;
	}
	else
	  p = p->tnext;
    }
  }
}

void widget_remove(struct widget *w) {
  struct divnode *sub_end;  
  handle hw;

  if (!in_shutdown) {
    /* Get us out of the hotkey list */
    install_hotkey(w,0);

    /* Get out of the timer list */
    remove_from_timerlist(w);

    /* Get rid of any pointers we have to it */
    if (w==under) under = NULL;
    if (w==prev_under) prev_under = NULL;
    if (w==capture) capture = NULL;
    if (w==kbdfocus) kbdfocus = NULL;

    /* Remove inner widgets if it can be done safely
       (only remove if they have handles) */
    while (w->sub && *w->sub) {    
      if ((*w->sub)->owner && (hw = hlookup((*w->sub)->owner,NULL)))
	handle_free(-1,hw);
      else
	break;
    }
    
    if (w->sub && *w->sub) {    
      /* More pointer mangling...  :) If this widget has other 
	 widgets inside of it, we will need to insert the 'sub'
	 list. This is a desperate attempt to not segfault. */

#ifdef DEBUG_WIDGET
      printf("************** Relocating sub list. w=0x%08X\n",w);
#endif
      
      sub_end = *w->sub;
      while (sub_end->next) 
	sub_end = sub_end->next;

      //      if (w->where) 
      *w->where = *w->sub;

	//      if (w->sub && *w->sub && (*w->sub)->owner)
      (*w->sub)->owner->where = w->where;
	
      if (w->out) {
	sub_end->next = *w->out;
	if (*w->out && (*w->out)->owner)
	  (*w->out)->owner->where = &sub_end->next;
      }     
      else
	sub_end->next = NULL;

      //      if (w->out && *w->out && (*w->out)->owner)

    }
    else {
      if (w->out) {
	*w->where = *w->out;
       	if (*w->out && (*w->out)->owner)
	  (*w->out)->owner->where = w->where;
      }
      else
	*w->where = NULL;
    }
    
    /* If we don't break this link, then deleting
       the widget's divtree will keep on going
       and delete other widgets' divtrees */
    if (w->out) *w->out = NULL; 
    if (w->sub) *w->sub = NULL;
  }
  
  if (w->def->remove) (*w->def->remove)(w);

  if (!in_shutdown) {
    /* Set the flags for redraw */
    if (w->dt && w->dt->head) {
      w->dt->head->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
      w->dt->flags |= DIVTREE_NEED_RECALC;
    }   
  }

#ifdef DEBUG_KEYS
  num_widgets--;
#endif
  g_free(w);
}

g_error inline widget_set(struct widget *w, int property, glob data) {
   g_error e;
   
   if (!(w && w->def->set))
     return mkerror(PG_ERRT_INTERNAL,23);
   
   /* If the widget has a handler, go with that */
   e = (*w->def->set)(w,property,data);
   if (errtype(e)!=ERRT_PASS)
     return e;
   
   /* Otherwise provide some defaults */
   switch (property) {
    
      /* Set the size, assuming initial split at w->in.
       * Calls resize handler if it exists, and sets the
       * appropriate flags.
       */
    case PG_WP_SIDE:
      if (!VALID_SIDE(data)) return mkerror(PG_ERRT_BADPARAM,2);
      w->in->flags &= SIDEMASK;
      w->in->flags |= ((sidet)data) | DIVNODE_NEED_RECALC | 
	DIVNODE_PROPAGATE_RECALC;
      resizewidget(w);
      w->dt->flags |= DIVTREE_NEED_RECALC;
      redraw_bg(w);
      break;

    case PG_WP_SIZE:
      if (data<0) data = 0;
      w->in->split = data;
      w->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;     /* No auto resizing */
      w->in->flags |= DIVNODE_NEED_RECALC | DIVNODE_PROPAGATE_RECALC;
      w->dt->flags |= DIVTREE_NEED_RECALC;
      redraw_bg(w);
      break;

    case PG_WP_SIZEMODE:
      w->in->flags &= ~DIVNODE_SIZE_AUTOSPLIT;     /* No auto resizing */
      w->in->flags &= ~PG_SZMODEMASK;
      w->in->flags |= data & PG_SZMODEMASK;
      redraw_bg(w);
      break;
      
    case PG_WP_HOTKEY:
      install_hotkey(w,data);
      break;
      
    default:
      return mkerror(PG_ERRT_BADPARAM,6);   /* Unknown property */
   }
   return sucess;
}

glob inline widget_get(struct widget *w, int property) {
   if (!(w && w->def->get))
     return 0;
   
   /* handle some universal properties */
   switch (property) {
    
    case PG_WP_ABSOLUTEX:      /* Absolute coordinates */
      return w->in->div->x;
    case PG_WP_ABSOLUTEY:
      return w->in->div->y;

    default:
      return (*w->def->get)(w,property);
   }
}

/* This is used in transparent widgets - it propagates a redraw through
   the container the widget is in, in order to redraw the background
*/
void redraw_bg(struct widget *self) {
  struct widget *container;

  /* Dereference the handle */
  if (iserror(rdhandle((void **)&container,PG_TYPE_WIDGET,-1,
		       self->container)) || ! container) return;

  /* Flags! Redraws automatically propagate through all child nodes of the
     container's div.
  */
#ifndef CONFIG_NOPANELBAR
  if (container->type == PG_WIDGET_PANEL) /* Optimize for panels: don't redraw panelbar */
    container->in->div->next->flags |= DIVNODE_NEED_REDRAW;
  else
#endif
    container->in->flags |= DIVNODE_NEED_REDRAW;
  container->dt->flags |= DIVTREE_NEED_REDRAW;
}

/***** Trigger stuff **/

/* This is called to reinit the cursor handling when a layer is
   popped from the dtstack
*/
void reset_widget_pointers(void) {
  under = prev_under = capture = NULL;
}

/*
  FIXME: implement find_hotkey!
*/
long find_hotkey(void) {
}

/*
  Installs or updates the hotkey for a widget
*/
void install_hotkey(struct widget *self,long hotkey) {

  if ((!self->hotkey) && hotkey) {
    /* Add to the hotkey widget list if this is our first hotkey */
    self->hknext = self->dt->hkwidgets;
    self->dt->hkwidgets = self;
  }
  else if (self->hotkey && (!hotkey)) {
    /* Remove us from the list */
    if (self->dt->hkwidgets) {
      if (self==self->dt->hkwidgets) {
	self->dt->hkwidgets = self->hknext;
      }
      else {
	struct widget *p = self->dt->hkwidgets;
	while (p->hknext)
	  if (p->hknext == self) {
	    /* Take us out */
	    p->hknext = self->hknext;
	    break;
	  }
	  else
	    p = p->hknext;
      }
    }
  }
  
  self->hotkey = hotkey;
}

/*
   Set a timer.  At the time, in ticks, specified by 'time',
   the widget will recieve a TRIGGER_TIMER
*/
void install_timer(struct widget *self,unsigned long interval) {
  struct widget *w;

  /* Remove old links */
  remove_from_timerlist(self);

  self->time = getticks() + interval;

  /* Stick it in the sorted timerwidgets list */
  if (timerwidgets && (timerwidgets->time < self->time)) {
    /* Find a place to insert it */

    w = timerwidgets;
    while (w->tnext && (w->tnext->time < self->time)) 
      w = w->tnext;

    /* Stick it in! */
    self->tnext = w->tnext;
    w->tnext = self;
  }
  else {
    /* The list is empty, or the new timer needs to go
       before everything else in the list */

    self->tnext = timerwidgets;
    timerwidgets = self;
  }
}

/* Trigger and remove the next timer trigger */
void inline trigger_timer(void) {
  struct widget *w;


  /* Verify that the trigger is actually due.
   * The trigger might have been modified between
   * now and when it was set.
   */
  if (timerwidgets && getticks()>=timerwidgets->time) {
    /* Good. Take it off and trigger it */
    w = timerwidgets;
    timerwidgets = timerwidgets->tnext;

    send_trigger(w,TRIGGER_TIMER,NULL);
  };
}

/*
  A widget has requested focus... Send out the ACTIVATE and DEACTIVATE
  triggers, and update necessary vars
*/
void request_focus(struct widget *self) {
  if (kbdfocus==self) return;
  send_trigger(kbdfocus,TRIGGER_DEACTIVATE,NULL);
  kbdfocus = self;
  send_trigger(self,TRIGGER_ACTIVATE,NULL);
}

/*
   Finds the topmost interactive widget under the (x,y) coords.
   Recursive. On first call, div should be set
   to dts->top->head
   NULL if nothing found.
   div_under_crsr should be set to NULL ahead of time, afterwards it is the
   result.
*/
void widgetunder(int x,int y,struct divnode *div) {
  if (!div) return;
  if (div->x<=x && div->y<=y && (div->x+div->w)>x && (div->y+div->h)>y
      && div->owner && div->build && div->owner->trigger_mask)
    div_under_crsr = div;
  widgetunder(x,y,div->next);
  widgetunder(x,y,div->div);
}

/* Internal function that sends a trigger to a widget if it accepts it. */
int send_trigger(struct widget *w, long type,
			 union trigparam *param) {
  if (w && w->def && w->def->trigger &&
      (w->trigger_mask & type)) {
    (*w->def->trigger)(w,type,param);
    return 1;
  }
  else {
    
    /* Some default handlers */
    switch (type) {

    case TRIGGER_KEYDOWN:
    case TRIGGER_KEYUP:
    case TRIGGER_CHAR:
      global_hotkey(param->kbd.key,param->kbd.mods,type);
      return 1;

    }
  }
  return 0;
}

void dispatch_pointing(u32 type,s16 x,s16 y,s16 btn) {
  union trigparam param;
  memset(&param,0,sizeof(param));

  inactivity_reset();

  if (type == TRIGGER_DOWN &&
      get_param_int("sound","click",0))
    drivermessage(PGDM_SOUNDFX,PG_SND_KEYCLICK);

  /* Convert coordinates from physical to logical */
  VID(coord_logicalize) (&x,&y);
   
  param.mouse.x = x;
  param.mouse.y = y;
  param.mouse.btn = btn;
  param.mouse.chbtn = btn ^ prev_btn;
  prev_btn = btn;

#ifdef DEBUG_EVENT
  printf("Pointing event: 0x%08X (%d,%d) %d\n",type,x,y,btn);
#endif

  /* Update the cursor */
  cursor->x = x;
  cursor->y = y;
  VID(sprite_update) (cursor);

  /* Selfish apps that don't like using widgets can just steal the pointing
     device for themselves.  Probably not something a word processor would
     want to do.  Probably for games.
  */
  if (pointer_owner) {
    int evt=0;
    switch (type) {
    case TRIGGER_MOVE:
      evt = PG_NWE_PNTR_MOVE;
      break;
    case TRIGGER_UP:
      evt = PG_NWE_PNTR_UP;
      break;
    case TRIGGER_DOWN:
      evt = PG_NWE_PNTR_DOWN;
      break;
    }
    if (evt)
      /* Squeeze all the mouse params into a long, as follows.
	 Note that if PicoGUI is to support screens bigger than
	 4096x4096 this won't work!
	 
	 Bits 31-28:  buttons
	 Bits 27-24:  changed buttons
	 Bits 23-12:  Y
	 Bits 11-0 :  X
      */
      post_event(evt,NULL,
		 (param.mouse.btn << 28) |
		 (param.mouse.chbtn << 24) |
		 (param.mouse.y << 12) |
		 param.mouse.x,
		 keyboard_owner,NULL);
    return;
  }

  if (!(dts && dts->top && dts->top->head)) {
#ifdef DEBUG_EVENT
    printf("Pointer event with invalid tree\n");
#endif
    return;   /* Without a valid tree, pointer events are meaningless */
  }

  div_under_crsr = NULL;

  /* If we need to worry about toolbars under the popup boxes, pass events
     occurring in the toolbar area to the root divtree */
  if (popup_toolbar_passthrough()) {
    struct divnode *ntb = appmgr_nontoolbar_area();

    if (x < ntb->x ||
	y < ntb->y ||
	x >= ntb->x+ntb->w ||
	y >= ntb->y+ntb->h) {
      printf("Nyap!\n");
      widgetunder(x,y,dts->root->head);
    }
    else
      widgetunder(x,y,dts->top->head);
  }
  else
    widgetunder(x,y,dts->top->head);

  if (div_under_crsr) {
    under = div_under_crsr->owner;

    if ((!(btn & capturebtn)) && capture && (capture!=under)) {
      send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }

    /* First send the 'raw' event, then handle the cooked ones. */
    send_trigger(under,type,&param);

    if (type==TRIGGER_DOWN && !capture) {
      capture = under;
      capturebtn = param.mouse.chbtn;
    }

  }
  else {
    under = NULL;
    if ((!(btn & capturebtn)) && capture) {
      send_trigger(capture,TRIGGER_RELEASE,&param);
      capture = NULL;
    }
  }

  if (under!=prev_under) {
    /* Mouse has moved over a different widget */
    send_trigger(under,TRIGGER_ENTER,&param);
    send_trigger(prev_under,TRIGGER_LEAVE,&param);
    prev_under = under;
  }

  /* If a captured widget accepts TRIGGER_DRAG, send it even when the
     mouse is outside its divnodes. */
  if (type == TRIGGER_MOVE && capture)
    send_trigger(capture,TRIGGER_DRAG,&param);
}

#ifdef DEBUG_KEYS
   /* Utility function and vars to implement CTRL-ALT-P
    * bitmap debug hotkey */
   
int db_x,db_y,db_h;
   
g_error debug_bitmaps(void **pobj) {
   hwrbitmap bmp = (hwrbitmap) *pobj;
   s16 w,h;
   
   VID(bitmap_getsize) (bmp,&w,&h);
   if (db_x+10+w>vid->lxres) {
     db_x = 0;
     db_y += db_h+8;
     db_h = 0;
   }
   if (h>db_h)
     db_h = h;
   
   if (db_y+45+h>vid->lyres) {
      struct fontdesc *df=NULL;
      struct quad screenclip;
      screenclip.x1 = screenclip.y1 = 0;
      screenclip.x2 = vid->lxres-1;
      screenclip.y2 = vid->lyres-1;
      rdhandle((void**)&df,PG_TYPE_FONTDESC,-1,defaultfont);

      outtext(vid->display,df,10,vid->lyres-15,VID(color_pgtohwr) (0xFFFF00),
	      "Too many bitmaps for this screen. Change video mode and try again",
	      &screenclip,0,0,PG_LGOP_NONE,0);

      return sucess;   /* Lies! :) */
   }
   
   VID(rect) (vid->display,db_x+3,db_y+38,w+4,h+4,
	      VID(color_pgtohwr)(0xFFFFFF),PG_LGOP_NONE);
   VID(rect) (vid->display,db_x+4,db_y+39,w+2,h+2,
	      VID(color_pgtohwr)(0x000000),PG_LGOP_NONE);
   VID(blit) (vid->display,db_x+5,db_y+40,w,h,bmp,0,0,PG_LGOP_NONE);

   db_x += w+8;
   return sucess;
}
   
   /* Utility functions to implement CTRL-ALT-N gropnode dump */
void r_grop_dump(struct divnode *div) {
   struct gropnode *n;
   int i;
   
   if (!div) return;
   if (div->grop) {
      printf("Divnode 0x%08X: ",div);
      if (div->owner)
	printf("Owned by widget 0x%08X, type %d\n",div->owner,div->owner->type);
      else
	printf("Unowned\n");
      
      for (n=div->grop;n;n=n->next) {
	 printf("  Gropnode: type 0x%04X flags 0x%04X at (%d,%d,%d,%d) params: ",
		n->type,n->flags,n->r.x,n->r.y,n->r.w,n->r.h);
	 for (i=0;i<PG_GROPPARAMS(n->type);i++)
	   printf("%d ",n->param[i]);
	 printf("\n");
      }
   }
   r_grop_dump(div->div);
   r_grop_dump(div->next);
}
void grop_dump(void) {
   struct divtree *dt;
   printf("---------------- Begin grop tree dump\n");
   for (dt=dts->top;dt;dt=dt->next)
     r_grop_dump(dt->head);
   printf("---------------- End grop tree dump\n");
}
     
/* Utility functions to implement CTRL-ALT-D divnode dump */
void r_div_dump(struct divnode *div, const char *label, int level) {
   int i;
   
   if (!div)
     return;

   printf(label);
   for (i=0;i<level;i++)
     printf("\t");
   printf("Div 0x%08X: flags=0x%04X split=%d prefer=(%d,%d) child=(%d,%d)\n",
	  div,div->flags,div->split,div->pw,div->ph,div->cw,div->ch);

   r_div_dump(div->div," Div:",level+1);
   r_div_dump(div->next,"Next:",level+1);
}
void div_dump(void) {
   struct divtree *dt;
   printf("---------------- Begin div tree dump\n");
   for (dt=dts->top;dt;dt=dt->next)
       r_div_dump(dt->head,"Root:",0);
   printf("---------------- End div tree dump\n");
}
     
#endif
   
void dispatch_key(u32 type,s16 key,s16 mods) {
  struct widget *p;
  union trigparam param;
  int suppress = 0;    /* If a keycode is used by a hotkey, it is only passed
			  to the hotkey owner for KEYDOWNs but other events
			  for that keycode should not be sent to the focused
			  widget */

  long keycode = (mods<<16) | key;     /* Combines mods and the key */
  
  inactivity_reset();

  /* For rotating arrow keys along with the rest of PicoGUI */
  VID(coord_keyrotate)(&key);

  if (type == TRIGGER_KEYDOWN &&
      get_param_int("sound","keyclick",0))
    drivermessage(PGDM_SOUNDFX,PG_SND_KEYCLICK);

  /* First, process magic 'double bucky' keys */
  if (type==TRIGGER_KEYDOWN &&
      (mods & PGMOD_CTRL) &&
      (mods & PGMOD_ALT)) {

    switch (key) {

    case PGKEY_SLASH:       /* CTRL-ALT-SLASH exits */
      request_quit();
      return;
    
#ifdef DEBUG_KEYS           /* The rest only work in debug mode */

    case PGKEY_h:           /* CTRL-ALT-h dumps the handle tree */
      handle_dump();
      return;

    case PGKEY_s:           /* CTRL-ALT-s dumps all strings */
      string_dump();
      return;

    case PGKEY_n:           /* CTRL-ALT-n dumps all gropnodes */
      grop_dump();
      return;

    case PGKEY_d:           /* CTRL-ALT-d dumps all divnodes */
      div_dump();
      return;
       
    case PGKEY_g:           /* Just for fun :) */
      guru("GURU MEDITATION #%08X\n\nCongratulations!\n"
	   "    Either you have read the source code or\n"
	   "    you have very persistantly banged your\n"
	   "    head on the keyboard ;-)",div_under_crsr);
      return;

    case PGKEY_m:           /* CTRL-ALT-m displays a memory profile */
      guru("Memory Profile\n\n"
	   "Total memory use: %d bytes in %d allocations\n\n"
	   "%d bytes in %d gropnodes\n"
	   "%d bytes in %d zombie gropnodes\n"
	   "%d bytes in %d divnodes\n"
	   "%d bytes in %d widgets\n"
	   "%d bytes in %d handle nodes",
	   memamt,memref,
	   num_grops*sizeof(struct gropnode),num_grops,
	   grop_zombie_count*sizeof(struct gropnode),grop_zombie_count,
	   num_divs*sizeof(struct divnode),num_divs,
	   num_widgets*sizeof(struct widget),num_widgets,
	   num_handles*sizeof(struct handlenode),num_handles);
      return;

    case PGKEY_b:           /* CTRL-ALT-b blanks the screen */
      VID(rect)   (vid->display, 0,0,vid->lxres,vid->lyres, 
		   VID(color_pgtohwr) (0),PG_LGOP_NONE);
      VID(update) (0,0,vid->lxres,vid->lyres);
      return;

    case PGKEY_y:           /* CTRL-ALT-y unsynchronizes the screen buffers */
      {
	/* The buffers in PicoGUI normally like to be synchronized.
	 * Data flows from the divtree to the backbuffer to the screen.
	 * The purpose of this debugging key is to put a different
	 * image on the screen (a black rectangle) than is in the rest
	 * of the pipeline, so that by watching the data ooze out one
	 * can tell if the correct update regions are being used and
	 * in general prod at the video driver.
	 * This would be very simple if not for the fact that only the video
	 * driver has access to the screen's buffer. The procedure here is
	 * to pump the black screen all the way through, then reinitializing
	 * the backbuffer while being very carefull not to update right away
	 * or mess up the sprites.
	 */

	struct divtree *p;
	/* Push through the black screen */
	VID(rect)   (vid->display, 0,0,vid->lxres,vid->lyres, 
		     VID(color_pgtohwr) (0),PG_LGOP_NONE);
	VID(update) (0,0,vid->lxres,vid->lyres);
	/* Force redrawing everything to the backbuffer */
	p = dts->top;
	while (p) {
	  p->flags |= DIVTREE_ALL_REDRAW;
	  p = p->next;
	}
	update(NULL,0);  /* Note the zero flag! */
	/* Clear the update rectangle, breaking the
	   pipeline that usually works so well :) */
	upd_w = 0;
	/* The above zero flag left sprites off.
	   With sprites off it's tough to use the mouse! */
	VID(sprite_showall) ();
      }
      return;

    case PGKEY_u:           /* CTRL-ALT-u makes a blue screen */
      VID(rect) (vid->display,0,0,vid->lxres,vid->lyres,
		 VID(color_pgtohwr) (0x0000FF), PG_LGOP_NONE);
      VID(update) (0,0,vid->lxres,vid->lyres);
      return;

    case PGKEY_p:           /* CTRL-ALT-p shows all loaded bitmaps */
      guru("Table of loaded bitmaps:");
      /* Reset evil globals :) */
      db_x = db_y = db_h = 0;
      handle_iterate(PG_TYPE_BITMAP,&debug_bitmaps);
      VID(update) (0,0,vid->lxres,vid->lyres);
      return;
       
#endif /* DEBUG_KEYS */

    }
  }

#ifdef DEBUG_EVENT
  printf("Keyboard event: 0x%08X (#%d, '%c') mod:0x%08X\n",type,key,key,mods);
#endif

  /* If the keyboard has been captured by a client, redirect everything
     to them.  This would seem a bit selfish of the client to want ALL
     keyboard input, but programs like games and kiosks are traditionally
     very selfish and always dislike however the operating system or GUI
     might handle the hardware.
     Well, I guess this is for games, then.
  */
  if (keyboard_owner) {
    int evt=0;
    switch (type) {
    case TRIGGER_CHAR:
      evt = PG_NWE_KBD_CHAR;
      break;
    case TRIGGER_KEYUP:
      evt = PG_NWE_KBD_KEYUP;
      break;
    case TRIGGER_KEYDOWN:
      evt = PG_NWE_KBD_KEYDOWN;
      break;
    }
    if (evt)
      /* Pack the mods into the high word and the key into the
	 low word.
      */
      post_event(evt,NULL,(type==TRIGGER_CHAR) ? key : keycode,keyboard_owner,NULL);
    return;
  }

  /* Ignore CHAR events for keys modified by anything other than shift */
  if (type == TRIGGER_CHAR && (mods & ~PGMOD_SHIFT)) return;

  /* Iterate through the hotkey-owning widgets if there's a KEYDOWN */
  p = dts->top->hkwidgets;
  while (p) {
    if (p->hotkey == keycode) {
      suppress = 1;
      if (type == TRIGGER_KEYDOWN)
	send_trigger(p,TRIGGER_HOTKEY,NULL);
    }
    p = p->hknext;
  }
  if (suppress) return;

  /* All other keypresses go to the focused widget (if any) */
  if (kbdfocus) {
    param.kbd.key = key;
    param.kbd.mods = mods;
    send_trigger(kbdfocus,type,&param);
  }
  /* Otherwise process global hotkeys here */
  /* Process global hotkeys */
  else
    global_hotkey(key,mods,type);
}

void dispatch_direct(char *name,u32 param) {
#ifdef DEBUG_EVENT
  printf("Direct event: %s(0x%08X)\n",name,param);
#endif
}

void resizewidget(struct widget *w) {
  (*w->def->resize)(w);
  divresize_recursive(w->dt->head);
}
   
/* Iterator function used by resizeall() */
g_error resizeall_iterate(void **p) {
   (*((struct widget *) (*p))->def->resize)((struct widget *) (*p));
   return sucess;
}
/* Call the resize() function on all widgets with handles */
void resizeall(void) {
  struct divtree *tree;

  handle_iterate(PG_TYPE_WIDGET,&resizeall_iterate);
  
  for (tree=dts->top;tree;tree=tree->next)
    divresize_recursive(tree->head);
}

/* Check for global hotkeys, like those used to move between widgets.
 * Widgets that can be focused (this includes widgets with hotspots)
 * should send unused keys here.
 */
void global_hotkey(u16 key,u16 mods, u32 type) {
  if (type == TRIGGER_KEYDOWN) {
   
    if (!mods) {
      /* Key down, no modifiers */

      switch (key) {
      case PGKEY_TAB:
	hotspot_traverse(HOTSPOT_NEXT);
	break;
      case PGKEY_UP:
	hotspot_traverse(HOTSPOT_UP);
	break;
      case PGKEY_DOWN:
	hotspot_traverse(HOTSPOT_DOWN);
	break;
      case PGKEY_LEFT:
	hotspot_traverse(HOTSPOT_LEFT);
	break;
      case PGKEY_RIGHT:
	hotspot_traverse(HOTSPOT_RIGHT);
	break;
      }
    }
    
    else if (mods & PGMOD_SHIFT) {
      /* Key down with shift */

      switch (key) {
      case PGKEY_TAB:
	hotspot_traverse(HOTSPOT_PREV);
	break;
      }
    }
  } 
}

/* The End */








