/* $Id: request.c,v 1.29 2001/12/14 21:49:42 micahjd Exp $
 *
 * request.c - Sends and receives request packets. dispatch.c actually
 *             processes packets once they are received.
 *             Welcome to the UNIXy side of PicoGUI...
 *             strace is your friend!
 *
 * PicoGUI small and efficient client/server GUI
 * Copyright (C) 2000,2001 Micah Dowty <micahjd@users.sourceforge.net>
 *
 * Thread-safe code added by RidgeRun Inc.
 * Copyright (C) 2001 RidgeRun, Inc.  All rights reserved.
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
#include <pgserver/pgnet.h>
#include <pgserver/input.h>
#ifndef CONFIG_UNIX_SOCKET
#include <netinet/tcp.h>
#else
#include <sys/un.h>

/* Default server unix domain socket path */
#define PG_REQUEST_SERVER "/var/tmp/.pgui"

#endif

#ifndef CONFIG_MAXPACKETSZ
#define CONFIG_MAXPACKETSZ 6000000
#endif

/* Socket */
int s = 0;

/* Nonzero when the main program is waiting for network/user input 
   in a select() call */
unsigned char req_in_select;

/* File descriptors of all open connections */
fd_set con;
int    con_n;

/* The connections waiting for an event */
fd_set evtwait;

/* Linked list of connection buffers */
struct conbuf *conbufs = NULL;

/* If this is nonzero, the last client exiting will cause
   the server to exit
*/
extern int use_sessionmgmt;
int numclients = 0;

/************* Functions used only in this file **/
/* Stuff that actually does the work, and gets called by the select loop */

/* Close a connection and clean up */
void closefd(int fd) {
  struct conbuf *p,*condemn=NULL;

  /* Last client left */
  if (use_sessionmgmt && !(--numclients))
    request_quit();
  
  /* Give up captured input devices */
  if (keyboard_owner==fd)
    keyboard_owner = 0;
  if (pointer_owner==fd)
    pointer_owner = 0;
  if (sysevent_owner==fd)
    sysevent_owner = 0;
  if (display_owner==fd) {
    struct divtree *p;
    
    display_owner = 0;
    
    /* Force redraw everything */
    p = dts->top;
    while (p) {
      p->flags |= DIVTREE_ALL_REDRAW;
      p = p->next;
    }
    update(NULL,1);
  }
  
#ifdef DEBUG_NET 
  printf("Close. fd = %d\n",fd);
#endif
  handle_cleanup(fd,-1);
  close(fd);
  FD_CLR(fd,&con);
  appmgr_unregowner(fd);

  /* Free the connection buffers */
  if (conbufs && conbufs->owner==fd) {
    condemn = conbufs;
    conbufs = conbufs->next;
  }
  else if (conbufs) {
    p = conbufs;
    while (p->next) {
      if (p->next->owner==fd) {
	condemn = p->next;
	p->next = p->next->next;
	break;
      }
      else
	p = p->next;
    }
  }
  if (condemn) {
    int i;

    /* Delete data associated with ring buffer nodes */
    for (i=0;i<EVENTQ_LEN;i++)
      if (condemn->q[i].data)
	g_free(condemn->q[i].data);

    /* Delete dynamic packet buffer */
    if (condemn->data_dyn)
      g_free(condemn->data_dyn);

    /* Delete connection buffer */
    g_free(condemn);
  }

  if (!in_shutdown)
    update(NULL,1);
}

void newfd(int fd) {
  struct pghello hi;
  struct conbuf *mybuf;
  memset(&hi,0,sizeof(hi));

  numclients++;

  /* Allocate connection buffers */
  if (iserror(prerror(g_malloc((void **)&mybuf,
			       sizeof(struct conbuf))))) {
    closefd(fd);
    return;
  }
  memset(mybuf,0,sizeof(struct conbuf));
  mybuf->owner = fd;
  mybuf->in = mybuf->out = mybuf->q;
  mybuf->next = conbufs;
  conbufs = mybuf;

  /* Say Hi!  Send a structure with information about the server */
  hi.magic = htonl(PG_REQUEST_MAGIC);
  hi.protover = htons(PG_PROTOCOL_VER);
  if (send_response(fd,&hi,sizeof(hi))) closefd(fd);
}

void readfd(int from) {
  struct conbuf *buf = find_conbuf(from);
  int r;
  
  /* Do we have a header yet? */
  if (buf->header_size < sizeof(buf->req)) {
    errno = 0;
    r = recv(from,((unsigned char*)(&buf->req))+buf->header_size,
	     sizeof(buf->req)-buf->header_size,0);
#ifdef DEBUG_NET
    printf("recv header = %d (have %d out of %d)\n",r,buf->header_size,
	   sizeof(buf->req));
#endif
    if (r<=0) {
      if (errno!=EAGAIN)
	/* Something bad happened, (the client probably disconnected)
	   close it down */
	closefd(from);
      return;
    }
    else {
      buf->header_size += r;
    }
  }
  
  /* Yep, get data */
  if (buf->header_size >= sizeof(buf->req)) {
    /* need prep to receive data? */
    if (!buf->data) {
      /* Reorder the bytes in the header */
      buf->req.type = ntohs(buf->req.type);
      buf->req.id = ntohl(buf->req.id);
      buf->req.size = ntohl(buf->req.size);

#ifdef DEBUG_NET
      printf("prep data (type %u, #%u, %lu bytes)\n",buf->req.type,
	     buf->req.id,buf->req.size);
#endif
      /* Will the data fit in the static buffer? */
      if (buf->req.size < PKTBUF_LEN) {
	buf->data = buf->data_stat;
      }
      else {
	/* Allocate and use a dynamic buffer (set the limit at 6 meg, this should be
	 * enough for even the most insanely large images (1280x1024x32) but will catch
	 * stuff before ElectricFence chokes on it. */
	if ((buf->req.size > CONFIG_MAXPACKETSZ) ||
	    iserror(prerror(g_malloc((void**)&buf->data_dyn,
				     buf->req.size+1)))) {

	  /* Oops, the client asked for too much memory!
	     Bad client!
	     Make them disappear.
	  */
	  closefd(from);
	  return;
	}
#ifdef DEBUG_NET
	printf("Using a dynamic packet buffer\n");
#endif
	buf->data = buf->data_dyn;
      }
      
      /* Null-terminate it. Only YOU can prevent runaway strings! */
      buf->data[buf->req.size] = 0;
    }

    if (buf->data_size < buf->req.size) {
      /* NOW we can get the packet data */
      
      errno = 0;
      r = recv(from,buf->data+buf->data_size,
	       buf->req.size-buf->data_size,0);      
#ifdef DEBUG_NET
      printf("recv data = %d\n",r);
#endif
      if (r<=0) {
	if (errno!=EAGAIN)
	  /* Something bad happened, (the client probably disconnected)
	     close it down */
	  closefd(from);
	return;
      }
      else {
	buf->data_size += r;
      }
    }

    /* Are we there yet? */
    if (buf->data_size >= buf->req.size) {
      /* Yahoo! */
      if (dispatch_packet(from,&buf->req,buf->data))
	closefd(from);
      else {
	/* Reset the structure for another packet */
	g_free(buf->data_dyn);
	buf->data_dyn = NULL;
	buf->data_size = buf->header_size = 0;
	buf->data = NULL;
      }
    }   
  }
}

/****************** 'public' functions */

/* Retrieves the buffers associated with a connection */
struct conbuf *find_conbuf(int fd) {
  struct conbuf *p=conbufs;
  while (p) {
    if (p->owner==fd)
      return p;
    p = p->next;
  }
  return NULL;
}

/* Like send, but with some error checking stuff.  Returns nonzero
   on error.
*/
int send_response(int to,const void *data,size_t len) {
  if (send(to,data,len,0)!=len) {
#ifdef DEBUG_NET
    printf("Error in send()\n");
#endif
    closefd(to);
    return 1;
  }
  return 0;
}

/* Bind the socket and start listening */
g_error net_init(void) {
#ifdef CONFIG_UNIX_SOCKET
  struct sockaddr_un server_sockaddr;
#else
  struct sockaddr_in server_sockaddr;
#endif
  volatile int tmp;
#ifdef WINDOWS
  WSADATA wsad;
#endif

  if (s) return;

#ifndef WINDOWS
  signal(SIGCHLD, SIG_IGN);
  signal(SIGPIPE, SIG_IGN);
#endif

#ifdef WINDOWS    /* Windows - compatible enough with BSD sockets that
		     programmers would accept WinSock, but incompatible enough
		     to break everything.  Another Microsoft 'innovation' */
  if (WSAStartup(MAKEWORD(2,0),&wsad))
    return mkerror(PG_ERRT_NETWORK,49);
#endif

#ifndef CONFIG_UNIX_SOCKET
  if((s = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP)) == -1)
#else
  if((s = socket(AF_UNIX, SOCK_STREAM, 0)) == -1)
#endif
    return mkerror(PG_ERRT_NETWORK,50);

#ifndef CONFIG_UNIX_SOCKET
  /* Setup TCP socket */

  /* Try to avoid blocking the port up... */
  tmp = 1;
  setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (void *)&tmp,sizeof(tmp));

  /* Try disabling the "Nagle algorithm" or "tinygram prevention" */
  tmp = 1;
  setsockopt(s,6 /*PROTO_TCP*/,TCP_NODELAY,(void *)&tmp,sizeof(tmp));
   
  server_sockaddr.sin_family = AF_INET;
  server_sockaddr.sin_port = htons(PG_REQUEST_PORT + 
				   get_param_int("pgserver","display",0));
  server_sockaddr.sin_addr.s_addr = htonl(INADDR_ANY);

  if(bind(s, (struct sockaddr *)&server_sockaddr, 
     sizeof(server_sockaddr)) == -1)
    return mkerror(PG_ERRT_NETWORK,52);

#else
  /* Setup unix domain socket */

  server_sockaddr.sun_family = AF_UNIX;
  strcpy(server_sockaddr.sun_path,PG_REQUEST_SERVER);
  /* Remove possible previous sockets */
  unlink(server_sockaddr.sun_path);
  tmp = strlen(server_sockaddr.sun_path) + sizeof(server_sockaddr.sun_family);

  if(bind(s, (struct sockaddr *)&server_sockaddr, tmp) == -1)
  {
    perror ("bind");
    return mkerror(PG_ERRT_NETWORK,52);
  }

#endif

  if(listen(s, REQUEST_BACKLOG) == -1)
    return mkerror(PG_ERRT_NETWORK,53);

  con_n = s+1;
  FD_ZERO(&con);
  FD_ZERO(&evtwait);

  return sucess;
}

void net_release(void) {
  int i;
  struct conbuf *p,*condemn=NULL;

  /* Don't clean up handles yet, handle cleanup will be done sometime
     in the shutdown process... */
  if (!s) return;
  for (i=0;i<con_n;i++)
    if (FD_ISSET(i,&con)) close(i);
  close(s);
  s = 0;

  /* Free the list! */
  p = conbufs;
  while (p) {
    condemn = p;
    p = p->next;
    g_free(condemn->data_dyn);
    g_free(condemn);
  }

  #ifdef WINDOWS
  WSACleanup();
  #endif
}

/* Yay, a big select loop! */
void net_iteration(void) {
  int fd;
  int len;
  struct sockaddr_in ec;
  int i;
  int argh=1;
  fd_set rfds;
  struct timeval tv;
  struct inlib *n;   /* For iterating input drivers */
#ifndef WINDOWS
  sigset_t sigmask;
#endif

  /* Get ready to select() the socket itself and all open connections */
  FD_ZERO(&rfds);
  FD_SET(s,&rfds);
  for (i=0;i<con_n;i++)     /* con stores all the active connections */
    if (FD_ISSET(i,&con)) FD_SET(i,&rfds);

  /* Default timeout */
  tv.tv_sec = 5;
  tv.tv_usec = 0;

  /* Give the input driver(s) a chance to modify things */
  n = inlib_list;
  while (n) {
    if (n->fd_init)
      (*n->fd_init)(&con_n,&rfds,&tv);
    n = n->next;
  }

  /* The only time we allow interruptions is during a select.
     I used to try to make PicoGUI reentrant, but everything boiled down
     to drawing operations which neither needed to be nor could be reentrant.
  */
#ifndef WINDOWS
  sigemptyset(&sigmask);
  sigprocmask(SIG_SETMASK,&sigmask,NULL);
#endif
   
  req_in_select = 1;

  i = select(con_n,&rfds,NULL,NULL,&tv);

#ifndef WINDOWS
  sigaddset(&sigmask,SIGPIPE);
  sigaddset(&sigmask,SIGCHLD);
  sigaddset(&sigmask,SIGALRM);

  sigprocmask(SIG_SETMASK,&sigmask,NULL);
#endif
   
  req_in_select = 0;

#ifdef DEBUG_NET
  /* For some reason, extra signals interrupt select() before it's done.
     FIXME.
     Uncomment this guru event for more information:
  */
  /*
  if (i<0)
    guru("Return from select()\ni = %d\ntv.tv_sec = %d\ntv.tv_usec = %d\nerrno = %d",
	 i,tv.tv_sec,tv.tv_usec,errno);
  */
#endif

  if (i>0) {
    /* Something is active */

    if (FD_ISSET(s,&rfds)) {
      /* New connection */

      len = sizeof(struct sockaddr_in);
      if((fd = accept(s, (void *)&ec, &len)) == -1) {
#ifdef DEBUG_NET
	printf("accept() returned -1\n");
#ifdef WINDOWS
	printf("WSAGetLastError() = %d\n",WSAGetLastError());
#endif
#endif
	goto requestloop_finish;
      }

      /* Make it non-blocking */
      ioctl(fd,FIONBIO,&argh);
      
      /* Save it for later */
      FD_SET(fd,&con);
      if ((fd+1)>con_n) con_n = fd+1;

#ifdef DEBUG_NET
      printf("Accepted. fd = %d, con_n = %d\n",fd,con_n);
#endif
      
      newfd(fd);

      goto requestloop_finish;
    }
    else {
      /* An existing connection needs attention */
      for (fd=0;fd<con_n;fd++)
	if (FD_ISSET(fd,&rfds)) {
	  if (FD_ISSET(fd,&con)) {
	    
	    /* Well, we're not waiting now! */
	    FD_CLR(fd,&evtwait);
	    
#ifdef DEBUG_NET
	    printf("Incoming. fd = %d\n",fd);
#endif
	    
	    readfd(fd);
	    goto requestloop_finish;
	  }
	  else {
	    /* This was not from a connection, but
	       from an input driver */
	    
	    n = inlib_list;
	    while (n) {
	      if (n->fd_activate && (*n->fd_activate)(fd))
		goto requestloop_finish;
	      n = n->next;
	    }
	    goto requestloop_finish;
	  }
	}
    }
  }
  
 requestloop_finish:
  
  /* Poll the input drivers */
  n = inlib_list;
  while (n) {
    if (n->poll)
      (*n->poll)();
    n = n->next;
  }

  return;
}

/* The End */

