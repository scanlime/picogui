/* $Id: requests.h,v 1.2 2002/11/20 22:23:27 micahjd Exp $
 *
 * requests.h - The interface to request packet handlers, used to
 *              implement the client/server protocol, themes, and WTs
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

#ifndef _H_REQUESTS
#define _H_REQUESTS

#include <picogui/network.h>

/* Structure passed through the request handlers */
struct request_data {
  struct {
    /* Request header and data, in network byte order */
    struct pgrequest *req;
    const void *data;
    /* Client ID of the sender */
    int owner;
  } in;

  struct {

    /* Response header and data in network byte order, used if has_response is 1 */
    union {
      u16 type;
      struct pgresponse_err err;
      struct pgresponse_ret ret;
      struct pgresponse_event event;
      struct pgresponse_data data;
    } response;
    u32 response_len;
    const void *response_data;
    u32 response_data_len;

    /* Return value, sent if there is no error or other response */
    u32 ret;
    
    unsigned int has_response : 1;

    /* Don't send a response yet, block this client until an event is queued */
    unsigned int block : 1;

    /* Free the response_data after this is processed */
    unsigned int free_response_data : 1;
  } out;
};
  
/* Process a request. On entry, all unused fields
 * are expected to be zero. On exit, applicable return values
 * will be filled in. If r->out.free_response_data is set, 
 * the caller must call g_free on r->out.data at some point.
 */
g_error request_exec(struct request_data *r);

#endif /* __H_REQUESTS */
/* The End */
