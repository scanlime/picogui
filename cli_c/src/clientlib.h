/* $Id: clientlib.h,v 1.5 2001/06/07 23:41:00 micahjd Exp $
 *
 * clientlib.h - definitions used only within the client library code itself
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * This library is free software; you can redistribute it and/or
 * modify it under the terms of the GNU Lesser General Public
 * License as published by the Free Software Foundation; either
 * version 2.1 of the License, or (at your option) any later version.
 * 
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
 * Lesser General Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with this library; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA  02111-1307  USA 
 * 
 * Contributors: 
 * Philippe Ney <philippe.ney@smartdata.ch>
 * Brandon Smith <lottabs2@yahoo.com>
 * 
 * 
 */

#ifndef _CLIENTLIB_H
#define _CLIENTLIB_H

/* System includes */
#include <netinet/tcp.h>
#include <sys/socket.h>
#include <sys/time.h>  /* for time_t type (used in timeval structure) */
#include <sys/types.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>    /* for fprintf() */
#include <malloc.h>
#include <signal.h>

#ifndef UCLINUX
#  include <alloca.h> /* for uClinux 'alloca' is in malloc.h */
#endif

#include <string.h>   /* for memcpy(), memset(), strcpy() */
#include <stdarg.h>   /* needed for pgRegisterApp and pgSetWidget */
#include <stdlib.h>   /* for getenv() */

/* PicoGUI */
#include <picogui.h>            /* Basic PicoGUI include */
#include <picogui/network.h>    /* Network interface to the server */

//#define DEBUG
//#define DEBUG_EVT

/* Default server */
#define PG_REQUEST_SERVER       "127.0.0.1"

/* Buffer size. When packets don't need to be sent immediately,
 * they accumulate in this buffer. It doesn't need to be very big
 * because most packets are quite small. Large packets like bitmaps
 * won't fit here anyway, so they are sent immediately.
 */
#define PG_REQBUFSIZE           512

/* A node in the list of event handlers set with pgBind */
struct _pghandlernode {
  pghandle widgetkey;
  short eventkey;
  pgevthandler handler;
  void *extra;
  struct _pghandlernode *next;
};

/* Structure for a retrieved and validated response code,
   the data collected by _pg_flushpackets is stored here. */
struct _pg_return_type {
  short type;
  union {

    /* if type == PG_RESPONSE_RET */
    unsigned long retdata;

    /* if type == PG_RESPONSE_EVENT */
    struct pgEvent event;

    /* if type == PG_RESPONSE_DATA */
    struct {
      unsigned long size;
      void *data;         /* Dynamically allocated - should be freed and
			     set to NULL when done, or it will be freed
			     next time flushpackets is called */
    } data;

  } e;  /* e for extra? ;-) */
};

/* Global vars for the client lib */
extern int _pgsockfd;                  /* Socket fd to the pgserver */
extern short _pgrequestid;             /* Request ID to detect errors */
extern short _pgdefault_rship;         /* Default relationship and widget */
extern pghandle _pgdefault_widget;        /*    when 0 is used */
extern unsigned char _pgeventloop_on;  /* Boolean - is event loop running? */
extern unsigned char _pgreqbuffer[PG_REQBUFSIZE];  /* Buffer of request packets */
extern short _pgreqbuffer_size;        /* # of bytes in reqbuffer */
extern short _pgreqbuffer_count;       /* # of packets in reqbuffer */
extern void (*_pgerrhandler)(unsigned short errortype,const char *msg); /* Error handler */
extern struct _pghandlernode *_pghandlerlist;  /* List of pgBind event handlers */

extern struct timeval _pgidle_period;  /* Period before calling idle handler */
extern pgidlehandler _pgidle_handler;  /* Idle handler */
extern unsigned char _pgidle_lock;     /* Already in idle handler? */
extern char *_pg_appname;             /* Name of the app's binary */
extern pgselecthandler _pgselect_handler;   /* Normally a pointer to select() */
extern struct _pg_return_type _pg_return; /* Response from _pg_flushpackets */

#define clienterr(msg)        (*_pgerrhandler)(PG_ERRT_CLIENT,msg)

/**** Internal functions (netcore.c) */

/* IO wrappers.  On error, they return nonzero and call clienterr() */
int _pg_send(void *data,unsigned long datasize);
int _pg_recv(void *data,unsigned long datasize);

/* Wait for a new event, recieves the type code. This is used 
 * when an idle handler or other interruption is needed */
int _pg_recvtimeout(short *rsptype);

/* Malloc wrapper. Reports errors */
void *_pg_malloc(size_t size);

/* Default error handler (this should never be called directly) */
void _pg_defaulterr(unsigned short errortype,const char *msg);

/* Put a request into the queue */
void _pg_add_request(short reqtype,void *data,unsigned long datasize);

/* Receive a response packet and store its contents in _pg_return
 * (handling errors if necessary)
 */
void _pg_getresponse(void);

/* Get rid of a pgmemdata structure when done with it */
void _pg_free_memdata(struct pgmemdata memdat);

/* Format a message in a dynamically allocated buffer */
char * _pg_dynformat(const char *fmt,va_list ap);

/* Idle handler */
void _pg_idle(void);

/**** Platform-dependant functions (platform.c) */

#ifdef UCLINUX
  int vsnprintf(char *buf, size_t size, const char *fmt, va_list args);
#endif

#endif /* _CLIENTLIB_H */

/* The End */
