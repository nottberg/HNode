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
 
HMSRV_CONTEXT Context;

void 
hmsrv_server_change( void )
{
   g_message("hmsrv_server_change\n");
   g_main_loop_quit( Context.Loop );
}

static gboolean checkDaemon  = FALSE;
static gboolean reloadConfig = FALSE;
static gboolean asDaemon     = FALSE;
static gboolean stopDaemon   = FALSE;

static GOptionEntry entries[] = 
{
    { "check", 'c', 0, G_OPTION_ARG_NONE, &checkDaemon, "Check whether the daemon is already running.  Return 1 is it is.", NULL },
    { "reload", 'r', 0, G_OPTION_ARG_NONE, &reloadConfig, "Reload the daemon configuration from its config file.", NULL },
    { "daemonize", 'D', 0, G_OPTION_ARG_NONE, &asDaemon, "Fork the daemon into a child process, otherwise run in the forground.", NULL },
    { "stop", 's', 0, G_OPTION_ARG_NONE, &stopDaemon, "Stop the currently running daemon", NULL },
    { NULL }
};

int
main (AVAHI_GCC_UNUSED int argc, AVAHI_GCC_UNUSED char *argv[])
{
    GOptionContext *cmdline_context;
    GError *error = NULL;
    GHNodeServer  *Server;
    guint          result;
    gboolean       startResult;

    // Initialize the gobject type system
    g_type_init();

    // Parse any command line options.
    cmdline_context = g_option_context_new( "- the hnode server implementation." );
    g_option_context_add_main_entries( cmdline_context, entries, NULL );
    g_option_context_parse( cmdline_context, &argc, &argv, &error );

    // Allocate a server object
    Server = g_hnode_server_new();

    if( Server == NULL )
        exit(-1);

    // Check if the daemon is running already 
    if( checkDaemon ) 
    {
        g_print("Check if daemon is running.");
        return g_hnode_server_check_daemon( Server );
    }

    // Check if we are being asked to kill an already running daemon 
    if( stopDaemon ) 
    {
        g_print("Attempting to stop the daemon.");
        return g_hnode_server_stop_daemon( Server );
    }

    // Reload the running daemons config 
    if( reloadConfig ) 
    {
        g_print("Reload daemon configuration.");
        return g_hnode_server_reload_config( Server );
    }

    // Server intialization


    // Create the GLIB main loop
    Context.Loop = g_main_loop_new( NULL, FALSE );

    // Register the server event callback
    g_signal_connect( G_OBJECT( Server ), "state_change", G_CALLBACK( hmsrv_server_change ), NULL );

    if( asDaemon )
    {
        // Fork off the server process
        startResult = g_hnode_server_start_as_daemon( Server );
    }
    else
    {
        // Start the server in the foreground
        startResult = g_hnode_server_start( Server );
    }

    // If the start operation encountered an error then 
    // exit right here before starting the main loop
    if( startResult )
    {
        // Bad start
        return 1;
    }

    // Check if this is the parent and we are running as a daemon
    // Then wait here for the daemon to start, and then exit with the start code.
    // If this is the daemon then start up the main loop and handle requests
    if( g_hnode_server_is_parent( Server ) )
    {
        // Wait for the daemon to start up successfully
        result = g_hnode_server_parent_wait_for_daemon_start( Server );
        g_print( "Daemon start: %d\n", result );
        g_main_loop_unref( Context.Loop );
        return result;
    }

    // Running as a child or in the forground
    // Start the event loop.
    g_main_loop_run( Context.Loop );
    
    // Clean up
    g_main_loop_unref( Context.Loop );
//     avahi_client_free (Context.AvahiClient);
//     avahi_glib_poll_free (glib_poll);
 
    return 0;
}
