/* $Id: sdlgl_camera.c,v 1.11 2002/09/23 22:51:26 micahjd Exp $
 *
 * sdlgl_camera.c - OpenGL driver for picogui, using SDL for portability.
 *                  This is an input filter that traps keyboard and mouse
 *                  input for camera control and other on-the-fly settings.
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

#include <pgserver/common.h>
#include <pgserver/sdlgl.h>

/* We need access to sdlinput's cursor */
extern struct cursor *sdlinput_cursor;

void gl_camera_reset(void);

/********************************************** Input filter ****/

void infilter_sdlgl_handler(struct infilter *self, u32 trigger, union trigparam *param) {
  float dx,dy,dz=0;
  s16 cursorx,cursory;
  float scale;

  /* Entering/leaving modes 
   */
  if ((param->kbd.mods & PGMOD_CTRL) && (param->kbd.mods & PGMOD_ALT) && trigger==PG_TRIGGER_KEYDOWN)
    switch (param->kbd.key) {

      /* Camera modes */

    case PGKEY_q:
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_NONE)
	gl_global.grid = 1;
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_TRANSLATE)
	gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
      else
	gl_global.camera_mode = SDLGL_CAMERAMODE_TRANSLATE;
      gl_global.need_update++;
      return;

    case PGKEY_e:
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_NONE)
	gl_global.grid = 1;
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_ROTATE)
	gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
      else
	gl_global.camera_mode = SDLGL_CAMERAMODE_ROTATE;
      gl_global.need_update++;
      return;

    case PGKEY_z:
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_NONE)
	gl_global.grid = 1;
      if (gl_global.camera_mode == SDLGL_CAMERAMODE_FOLLOW_MOUSE)
	gl_camera_reset();
      else
	gl_global.camera_mode = SDLGL_CAMERAMODE_FOLLOW_MOUSE;
      gl_global.need_update++;
      return;

    case PGKEY_r:
      gl_camera_reset();
      return;

      /* Misc flags */

    case PGKEY_f:
      gl_global.showfps = !gl_global.showfps;
      gl_global.need_update++;
      return;

    case PGKEY_g:
      gl_global.grid = !gl_global.grid;
      gl_global.need_update++;
      return;

    case PGKEY_w:
      gl_global.wireframe = !gl_global.wireframe;
      gl_global.need_update++;
      return;
    }
  
  /* Keep track of pressed keys 
   */
  if (trigger == PG_TRIGGER_KEYDOWN)
    gl_global.pressed_keys[param->kbd.key] = 1;
  if (trigger == PG_TRIGGER_KEYUP)
    gl_global.pressed_keys[param->kbd.key] = 0;

  /* Follow-mouse mode doesn't absorb normal events like the other modes */
  if (gl_global.camera_mode == SDLGL_CAMERAMODE_FOLLOW_MOUSE) {
    switch (trigger) {
      
    case PG_TRIGGER_MOVE:
      /* We want smoothing for rotation and Z, but short-circuit the smoothing
       * for X and Y, it's more disorienting than helpful.
       */
      gl_global.camera.e.tx = vid->xres/2 - param->mouse.x;
      gl_global.camera.e.ty = vid->yres/2 - param->mouse.y;
      gl_global.smoothed_cam.e.tx = gl_global.camera.e.tx;
      gl_global.smoothed_cam.e.ty = gl_global.camera.e.ty;
      break;

    case PG_TRIGGER_DOWN:
      if (gl_global.pressed_keys[PGKEY_LSHIFT] || gl_global.pressed_keys[PGKEY_RSHIFT]) {
	if (param->mouse.btn & 8)
	  gl_global.camera.e.tz += 20;
	if (param->mouse.btn & 16)
	  gl_global.camera.e.tz -= 20;
	return;
      }
      break;

    }
  }
  /* In a camera mode other than follow-mouse?
   */
  else if (gl_global.camera_mode != SDLGL_CAMERAMODE_NONE) {
    
    /* A couple keys should exit camera mode... 
     */
    if (trigger == PG_TRIGGER_KEYDOWN)
      switch (param->kbd.key) {
      case PGKEY_ESCAPE:
      case PGKEY_SPACE:
      case PGKEY_RETURN:
	gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
	gl_global.need_update++;
      }

    /* Mouse click should also exit... 
     */
    if (trigger == PG_TRIGGER_DOWN && (param->mouse.btn & 1)) {
      gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
      gl_global.need_update++;
    }

    /* If this was a mouse movement from SDL, get the 
     * relative movement then warp the SDL cursor back to the center
     */
    if ((trigger & (PG_TRIGGER_MOVE | PG_TRIGGER_UP | PG_TRIGGER_DOWN)) && 
	param->mouse.cursor == sdlinput_cursor) {
      int oldx,oldy;

      /* Measure the relative motion */
      cursor_getposition(param->mouse.cursor,&oldx,&oldy);
      dx = param->mouse.x - oldx;
      dy = param->mouse.y - oldy;

      /* If there was no motion, it's likely that this was the event generated by
       * the SDL_WarpMouse below on a previous event. Throw away the event now
       * to avoid infinite recursion.
       */
      if (trigger==PG_TRIGGER_MOVE && !(dx || dy))
	return;  

      /* This will generate a new motion event! */
      SDL_WarpMouse(oldx,oldy);

      /* Translate the mouse wheel into Z motion. Note that this isn't really
       * the correct way to do it, but this works since we're interfacing almost
       * directly with the sdlinput driver. (This is loaded as the first input filter)
       */
      if (trigger == PG_TRIGGER_DOWN && (param->mouse.btn & 8))
	dz = 20;
      if (trigger == PG_TRIGGER_DOWN && (param->mouse.btn & 16))
	dz = -20;    

      scale = gl_get_key_scale();
      dx *= scale;
      dy *= scale;
      dz *= scale;
      
      switch (gl_global.camera_mode) {
	
      case SDLGL_CAMERAMODE_TRANSLATE:
	gl_global.camera.e.tx += dx;
	gl_global.camera.e.ty += dy;
	gl_global.camera.e.tz += dz;
	break;
	
      case SDLGL_CAMERAMODE_ROTATE:
	gl_global.camera.e.ry += dx * 0.1;
	gl_global.camera.e.rx -= dy * 0.1;
	gl_global.camera.e.rz += dz * 0.1;
	break;
      }
    }
    
    /* Trap events here */
    return;
  }

  /* Otherwise, pass the event */
  infilter_send(self,trigger,param);
}

/********************************************** Utilities ****/

void gl_camera_reset(void) {
  gl_global.camera_mode = SDLGL_CAMERAMODE_NONE;
  gl_global.camera.e.tx = 0;
  gl_global.camera.e.ty = 0;
  gl_global.camera.e.tz = 0;
  gl_global.camera.e.rx = 0;
  gl_global.camera.e.ry = 0;
  gl_global.camera.e.rz = 0;
  gl_global.resetting = 1;
  gl_global.need_update++;
}

/* Per-frame processing for motion keys
 */
void gl_process_camera_keys(void) {
  float scale = gl_get_key_scale();

  /* Process camera movement keys */
  switch (gl_global.camera_mode) {
    
  case SDLGL_CAMERAMODE_TRANSLATE:
    if (gl_global.pressed_keys[PGKEY_w])     gl_global.camera.e.tz += 5.0 * scale;
    if (gl_global.pressed_keys[PGKEY_s])     gl_global.camera.e.tz -= 5.0 * scale;
    if (gl_global.pressed_keys[PGKEY_DOWN])  gl_global.camera.e.ty += 5.0 * scale;
    if (gl_global.pressed_keys[PGKEY_UP])    gl_global.camera.e.ty -= 5.0 * scale;
    if (gl_global.pressed_keys[PGKEY_RIGHT]) gl_global.camera.e.tx += 5.0 * scale;
    if (gl_global.pressed_keys[PGKEY_LEFT])  gl_global.camera.e.tx -= 5.0 * scale;
    break;

  case SDLGL_CAMERAMODE_ROTATE:
    if (gl_global.pressed_keys[PGKEY_w])     gl_global.camera.e.rz += 0.4 * scale;
    if (gl_global.pressed_keys[PGKEY_s])     gl_global.camera.e.rz -= 0.4 * scale;
    if (gl_global.pressed_keys[PGKEY_UP])    gl_global.camera.e.rx += 0.4 * scale;
    if (gl_global.pressed_keys[PGKEY_DOWN])  gl_global.camera.e.rx -= 0.4 * scale;
    if (gl_global.pressed_keys[PGKEY_LEFT])  gl_global.camera.e.ry += 0.4 * scale;
    if (gl_global.pressed_keys[PGKEY_RIGHT]) gl_global.camera.e.ry -= 0.4 * scale;
    break; 
  }
}

/* Handle smoothly moving our smoothed_cam to the camera position. This
 * eliminates the jerkiness of the mouse wheel and keyboard, and in general
 * increases this drivers niftiness :)
 */
void gl_process_camera_smoothing(void) {
  int i;
  float diff;
  int done_resetting = 0;
  int num_axes = (sizeof(gl_global.camera.array)/sizeof(double));

  /* Loop through each element in the camera coords... */
  for (i=0; i<num_axes; i++) {
    diff = gl_global.camera.array[i] - gl_global.smoothed_cam.array[i];

    /* Above some threshold, move smoothly to the position and cause a frame
     * to be rendered. Below that threshlod, snap to the correct position and render
     * one more frame. If it's zero, don't render anything.
     */
    if (fabs(diff) > 0.01) {
      gl_global.smoothed_cam.array[i] += diff * 0.1;
      gl_global.need_update++;
    }
    else if (diff) {
      gl_global.smoothed_cam.array[i] = gl_global.camera.array[i];
      gl_global.need_update++;
    }
    else if (gl_global.resetting) {
      /* We just finished smoothing out the camera motion from a camera reset,
       * Make sure the camera is snapped to 0 so we don't get rounding errors.
       */
      gl_global.smoothed_cam.array[i] = 0;
      done_resetting++;
    }
  }  

  /* If every axis is done resetting, turn off the grid and resetting flags */
  if (done_resetting == num_axes) {
    gl_global.resetting = 0;
    gl_global.grid = 0;
  }
}

/* Set up the current OpenGL matrix to view from our camera 
 */
void gl_matrix_camera(void) {
  glLoadIdentity();
  gl_matrix_pixelcoord();

  glTranslatef(0,0,gl_global.smoothed_cam.e.tz);

  /* Rotate from the center of the screen */
  glTranslatef(vid->xres/2,  vid->yres/2, 0);
  glRotatef(gl_global.smoothed_cam.e.rx,1,0,0);
  glRotatef(gl_global.smoothed_cam.e.ry,0,1,0);
  glRotatef(gl_global.smoothed_cam.e.rz,0,0,1);
  glTranslatef(-vid->xres/2, -vid->yres/2, 0);

  glTranslatef(gl_global.smoothed_cam.e.tx,
	       gl_global.smoothed_cam.e.ty,0);
}

/* Allow modifier keys to scale movements 
 */
float gl_get_key_scale(void) {
  if (gl_global.pressed_keys[PGKEY_LSHIFT] || gl_global.pressed_keys[PGKEY_RSHIFT])
    return 0.1;
  if (gl_global.pressed_keys[PGKEY_LCTRL] || gl_global.pressed_keys[PGKEY_RCTRL])
    return 10.0;

  return 1;
}

/********************************************** Input filter registration ****/

struct infilter infilter_sdlgl = {
  accept_trigs: PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP | PG_TRIGGER_CHAR |
                PG_TRIGGER_MOVE | PG_TRIGGER_DOWN | PG_TRIGGER_UP,
  absorb_trigs: PG_TRIGGER_KEYDOWN | PG_TRIGGER_KEYUP | PG_TRIGGER_CHAR |
                PG_TRIGGER_MOVE | PG_TRIGGER_DOWN | PG_TRIGGER_UP,
  handler: &infilter_sdlgl_handler,
};

/* The End */









