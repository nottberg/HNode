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

#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

#include "hnode-pktsrc.h"
#include "hnode-browser.h"


static gint wait_time = 0;
static gboolean list  = FALSE;
static gboolean debug = FALSE;
static gboolean event = FALSE;

typedef struct hmcfg_context
{
   GMainLoop     *loop; 
   GHNodeBrowser *Browser;
}HMCFG_CONTEXT;

/* Callbacks for GLIB API Timeout Event */
static gboolean
hmcfg_list_timeout (void *userdata)
{
    HMCFG_CONTEXT *Context = userdata;

    GHNodeBrowserMPoint    *MPoint;
    GHNodeBrowserHNode     *Node;
    GHNodeAddress          *ObjAddr;
    gchar                  *AddrStr;
    guint32                 AddrStrLen;

//    GHNodeBrowserHNodeIter *Iter;
 
    // Cycle through the management servers starting at the first.
    MPoint = g_hnode_browser_first_mpoint(Context->Browser);
    while(MPoint)
    {
        // Dump Node Data
        g_hnode_browser_mpoint_get_address(MPoint, &ObjAddr);

        g_hnode_address_get_str_ptr( ObjAddr, &AddrStr, &AddrStrLen);   
        g_print("Management Server Address: %s\n", AddrStr);   

        // See if there are additional Node to process
        MPoint = g_hnode_browser_next_mpoint(Context->Browser);
    }
    
    // Cycle through the nodes starting at the first.
    Node = g_hnode_browser_first_hnode(Context->Browser);
    while(Node)
    {
        guint32 MajorVersion;
        guint32 MinorVersion;
        guint32 MicroVersion;
        guint8  UIDArray[16];
        guint32 EPCount;
        guint32 EPIndx;

        // Dump Node Data
        //g_hnode_browser_hnode_debug_print(Node);

        g_hnode_browser_hnode_get_address(Node, &ObjAddr);
        g_hnode_browser_hnode_get_version(Node, &MajorVersion, &MinorVersion, &MicroVersion);
        g_hnode_browser_hnode_get_uid(Node, UIDArray);
        g_hnode_browser_hnode_get_endpoint_count(Node, &EPCount);

        g_hnode_address_get_str_ptr( ObjAddr, &AddrStr, &AddrStrLen);   
        g_print("AddrStr: %s\n", AddrStr);   
        g_print("Version: %d.%d.%d\n", MajorVersion, MinorVersion, MicroVersion);
        g_print("NodeID: %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n", 
                UIDArray[0], UIDArray[1], UIDArray[2], UIDArray[3],
                UIDArray[4], UIDArray[5], UIDArray[6], UIDArray[7],
                UIDArray[8], UIDArray[9], UIDArray[10], UIDArray[11],
                UIDArray[12], UIDArray[13], UIDArray[14], UIDArray[15]);   

        g_print("End Point Count: %d\n", EPCount);   

        // Cycle through endpoints
        EPIndx = 0;
        while( EPIndx < EPCount )
        {
            GHNodeAddress *EPAddr;
            guint32  EPMajorVer;
            guint32  EPMinorVer;
            guint32  EPMicroVer;
            guint32  EPAssocIndex;
            gchar   *EPTypeStrPtr;

            g_hnode_browser_hnode_endpoint_get_address(Node, EPIndx, &EPAddr);
            g_hnode_browser_hnode_endpoint_get_version(Node, EPIndx, &EPMajorVer, &EPMinorVer, &EPMicroVer);
            g_hnode_browser_hnode_endpoint_get_associate_index(Node, EPIndx, &EPAssocIndex);   
            g_hnode_browser_hnode_endpoint_get_type_str(Node, EPIndx, &EPTypeStrPtr);

            g_hnode_address_get_str_ptr( EPAddr, &AddrStr, &AddrStrLen);   
            g_print("EP AddrStr: %s\n", AddrStr);      
            g_print("Version: %d.%d.%d\n", EPMajorVer, EPMinorVer, EPMicroVer);
            g_print("Associated End Point Index: %d\n", EPAssocIndex);
            g_print("Mime Type Str: %s\n", EPTypeStrPtr);   

            // Try the next endpoint.
            EPIndx += 1;
        }

        // See if there are additional Node to process
        Node = g_hnode_browser_next_hnode(Context->Browser);
    }

    /* Quit the application */
    g_main_loop_quit( Context->loop );
 
    return FALSE; /* Don't re-schedule timeout event */
}

static gboolean
hmcfg_exit_timeout (void *userdata)
{
    HMCFG_CONTEXT *Context = userdata;
 
    /* Quit the application */
    g_main_loop_quit( Context->loop );
 
    return FALSE; /* Don't re-schedule timeout event */
}

static void
hmcfg_hnode_debugrx(GHNodeBrowserHNode *hnode, 
                    guint   DbgType,
                    guint   DbgSeqCnt,
                    guint   EPIndex,
                    guint   DDLength,
                    guint8 *DDPtr,
                    void   *userdata)
{
    HMCFG_CONTEXT *Context = userdata;
 
    g_message ("hmcfg_hnode_debugrx: %d %d %d %d!", DbgType, DbgSeqCnt, EPIndex, DDLength);

}

static void
hmcfg_hnode_add (GHNodeBrowser *Browser, GHNodeBrowserHNode *hnode, void *userdata)
{
    HMCFG_CONTEXT *Context = userdata;
 
    g_signal_connect(hnode, "debug-rx", G_CALLBACK(hmcfg_hnode_debugrx), &Context);

}

static void
hmcfg_mgmt_add (GHNodeBrowser *Browser, GHNodeBrowserMPoint *MPoint, void *userdata)
{
    HMCFG_CONTEXT *Context = userdata;
 
    // If debug is turned on then enable debug packet forwarding.
    if( debug )
    {
        g_hnode_browser_mpoint_enable_debug(MPoint);
    }

}


static GOptionEntry entries[] = 
{
  { "wait", 't', 0, G_OPTION_ARG_INT, &wait_time, "The number of seconds to wait before exiting.", "N" },
  { "list", 'l', 0, G_OPTION_ARG_NONE, &list, "Discover management servers and associated hnodes.  Output a list upon exit.", NULL },
//  { "list-xml", 'x', 0, G_OPTION_ARG_NONE, &listxml, "Discover management servers and associated hnodes.  Output an xml list upon exit.", NULL },
//  { "list-intf", 'i', 0, G_OPTION_ARG_STR, &listintf, "Discover hnodes that export a specific interface.  Output an xml list upon exit.", NULL },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Decode and output debug packets from discovered hnodes.", NULL },
  { "event", 'e', 0, G_OPTION_ARG_NONE, &event, "Decode and output event packets from discovered hnodes.", NULL },
  { NULL }
};
 
int
main (AVAHI_GCC_UNUSED int argc, AVAHI_GCC_UNUSED char *argv[])
{
    GMainLoop *loop = NULL;
    GHNodeBrowser *Browser;
    GError *error = NULL;
    GOptionContext *cmdline_context;
    HMCFG_CONTEXT   Context;

    // Initialize the gobject type system
    g_type_init();

    // Parse any command line options.
    cmdline_context = g_option_context_new ("- command line interface to hnode management servers.");
    g_option_context_add_main_entries (cmdline_context, entries, NULL); // GETTEXT_PACKAGE);
    g_option_context_parse (cmdline_context, &argc, &argv, &error);

    // Allocate the main loop and a browser object
    Context.Browser = g_hnode_browser_new();

    if( Context.Browser == NULL )
        exit(-1);

    Context.loop    = g_main_loop_new (NULL, FALSE);

    printf("Context: 0x%x\n", &Context);

    // Process the command line options
    if( list )
    {
        if( event | debug )
        {
            g_error("The event/debug options cannot be used in combination with the list option.");
            goto fail;
        }

        // default the wait_time for a list operation to 1 second.
        if( wait_time == 0 )
            wait_time = 1;

        // Setup a timeout to end the list process
        g_timeout_add( (wait_time * 1000), hmcfg_list_timeout, &Context);

        g_signal_connect(Context.Browser, "hnode-add", G_CALLBACK(hmcfg_hnode_add), &Context);
        g_signal_connect(Context.Browser, "mgmt-add", G_CALLBACK(hmcfg_mgmt_add), &Context);

        // Start up the server object and main loop
        g_hnode_browser_start(Context.Browser);

        /* Start the GLIB Main Loop */
        g_main_loop_run (Context.loop);
        
    }
    else if( event | debug )
    {
        // If there is a wait time then set that up here.
        if( wait_time )
        {
            g_timeout_add( (wait_time * 1000), hmcfg_exit_timeout, &Context);
        }

        g_signal_connect(Context.Browser, "hnode-add", G_CALLBACK(hmcfg_hnode_add), &Context);
        g_signal_connect(Context.Browser, "mgmt-add", G_CALLBACK(hmcfg_mgmt_add), &Context);

        // Start up the server object and main loop
        g_hnode_browser_start(Context.Browser);

        /* Start the GLIB Main Loop */
        g_main_loop_run (Context.loop);

    }
       


    // Register the server event callback
    //g_signal_connect (G_OBJECT (HNode), "state_change",
	//	      G_CALLBACK (hnode_change), NULL);

    // Setup the HNode

     

    fail:
    /* Clean up */
    g_main_loop_unref (Context.loop);
    g_object_unref(Context.Browser);

    return 0;
}
