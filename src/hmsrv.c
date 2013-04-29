/* $Id: glib-integration.c 937 2005-11-08 21:56:28Z lennart $ */
 
/***
   This file is part of avahi.
  
   avahi is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.
  
   avahi is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
   Public License for more details.
  
   You should have received a copy of the GNU Lesser General Public
   License along with avahi; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.
***/
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <glib.h>
 
#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

#include "hnode-pktsrc.h"
#include "hnode-srvobj.h"

typedef struct HMSrvProgramContextStruct
{
   GMainLoop       *Loop;
//   AvahiClient     *AvahiClient;
//   AvahiEntryGroup *ServiceGroup;
//   char            *ServiceName;

//   PacketSource    *ListenSource;
//   PacketSource    *DebugSource;
//   PacketSource    *EventSource;

}HMSRV_CONTEXT;
 
void 
hmsrv_server_change( void )
{
   g_message("hmsrv_server_change\n");
}

int
main (AVAHI_GCC_UNUSED int argc, AVAHI_GCC_UNUSED char *argv[])
{

    GHNodeServer  *Server;
    HMSRV_CONTEXT Context;

    // Initialize the gobject type system
    g_type_init();

    // Allocate a server object
    Server = g_hnode_server_new();

    if( Server == NULL )
        exit(-1);

    // Server intialization

    /* Create the GLIB main loop */
    Context.Loop = g_main_loop_new (NULL, FALSE);

    // Register the server event callback
    g_signal_connect (G_OBJECT (Server), "state_change",
		      G_CALLBACK (hmsrv_server_change), NULL);

    // Start up the server object
    g_hnode_server_start(Server);

    /* Start the GLIB Main Loop */
    g_main_loop_run (Context.Loop);

    fail:
    /* Clean up */
    g_main_loop_unref (Context.Loop);
//     avahi_client_free (Context.AvahiClient);
//     avahi_glib_poll_free (glib_poll);
 
    return 0;
}
