/*
 * input.h - Abstract input driver interface
 * $Revision: 1.2 $
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#ifndef __H_INPUT
#define __H_INPUT

g_error input_init(void (*request_quit)(void));
void input_release();

#endif /* __H_INPUT */
/* The End */










