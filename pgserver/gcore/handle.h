/*
 * handle.h - Functions and data structures for allocating handles to
 *            represent objects, converting between handles and pointers,
 *            and deleting handles.
 * 
 * Micah Dowty <micah@homesoftware.com>
 * 
 * This file is released under the GPL. Please see the file COPYING that
 * came with this distribution.
 */

#ifndef __HANDLE_H
#define __HANDLE_H

#include <g_error.h>

/* Flags for the handle's type */
#define HFLAG_RED   0x80     /* This is a red-black tree */
#define HFLAG_NFREE 0x40     /* The pointer should _not_ be free'd when the
			        handle is destroyed (free'd in a method
			        depending on the data type */

/* Data types */
#define TYPE_BITMAP     1
#define TYPE_WIDGET     2
#define TYPE_FONTDESC   3
#define TYPE_STRING     4

/* Data type of a handle ID */
typedef unsigned short handle;
#define HANDLE_BITS     16
#define HANDLE_SIZE (1<<HANDLE_BITS)

struct handlenode {
  handle id;
  int owner;        /* the connection that owns this handle */
  unsigned char type;          /* Most of this represents the data type
				  that this handle points to. Upper 2 bits
				  are for HFLAGs */
  void *obj;
  struct handlenode *left,*right,*parent;
};

/* Allocates a new handle for obj 
 * Owner = -1, system owns it
 */
g_error mkhandle(handle *h,unsigned char type,int owner,void *obj);

/* Reads the handle, returns NULL if handle is invalid or if it
   doesn't match the required type and user. If owner is -1, we don't care */
g_error rdhandle(void **p,unsigned char reqtype,int owner,handle h);

/* Deletes the handle, and if HFLAG_NFREE is not set it frees the object.
 * Owner = -1, don't care
 */
g_error handle_free(int owner,handle h);

/* Deletes all handles from a specified owner (-1 for all handles) */
void handle_cleanup(int owner);

/* A fairly interesting function.  Destroys any data referenced by
   the destination handle, and transfers the data from the source
   handle to the destination handle.  The destination's ownership is
   retained, and the source becomes invalid */
g_error handle_bequeath(handle dest, handle src, int srcowner);

#endif /* __HANDLE_H */
/* The End */
