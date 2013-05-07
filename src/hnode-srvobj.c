/**
 * hnode_srvobj.c
 *
 * Implements a GObject providing the basics for an HNode Server
 *
 * This is free software; you can redistribute it and/or modify it
 * under the terms of the GNU Lesser General Public License as
 * published by the Free Software Foundation; either version 2.1 of the
 * License, or (at your option) any later version.
 * 
 * It is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
 * Public License for more details.
 * 
 * You should have received a copy of the GNU Lesser General Public
 * License along with avahi; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
 * USA.
 * 
 * (c) 2005, Curtis Nottberg
 *
 * Authors:
 *   Curtis Nottberg
 */

#include <glib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <errno.h>

// These functions should probably be replaced with glib equivalents.
#include <stdio.h>
#include <stdlib.h>
#include <strings.h>
#include <string.h>

#include <glib.h>

#include <avahi-client/client.h>
#include <avahi-client/lookup.h>
#include <avahi-client/publish.h>

#include <avahi-common/alternative.h>
#include <avahi-common/error.h>
#include <avahi-common/timeval.h>

#include <avahi-glib/glib-watch.h>
#include <avahi-glib/glib-malloc.h>

#include <libdaemon/dfork.h>
#include <libdaemon/dsignal.h>
#include <libdaemon/dlog.h>
#include <libdaemon/dpid.h>
#include <libdaemon/dexec.h>

//#include <slp.h>

//#include "hmpacket.h"
#include "hnode-logging.h"
#include "hnode-pktsrc.h"
#include "hnode-provider.h"
#include "hnode-srvobj.h"





#define G_HNODE_SERVER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_SERVER, GHNodeServerPrivate))

G_DEFINE_TYPE (GHNodeServer, g_hnode_server, G_TYPE_OBJECT);

/*
typedef struct _GHNodeServerPriv GHNodeServerPrivate;
struct _GHNodeManagementServerPriv
{
    guint32 ObjID;

    guint16  URLLifetime;
    gchar    URLStr[256];
    
    gchar   HMAddrStr[128];
    guint16 HMPort;    
    struct sockaddr_in HMAddr;    
};

typedef struct _GHNodeProviderRecordPriv GHNodeProviderPrivate;
struct _GHNodeProviderRecordPriv
{
    guint32 ObjID;
    
    guint16 PortNumber;
    guint16 MicroVersion;
    guint8  MinorVersion;
    guint8  MajorVersion;
    guint32 EndPointCount;
    guint32 HostAddrType;
    guint32 HostAddrLength;
    guint8  HostAddr[128];
	guint8  NodeUID[16];
    
    GList *EndPoints;
};
 
typedef struct _GHNodeEndPointRecordPriv GHNodeEndPointPrivate; 
struct _GHNodeEndPointRecordPriv
{
    guint32 ObjID;
    
    guint16 PortNumber;
    guint16 MicroVersion;
    guint8  MinorVersion;
    guint8  MajorVersion;
    guint32 MimeStrLength;
    guint8  MimeTypeStr[256];
    
    GHNodeProviderPrivate *Parent;
};

typedef struct _GHNodeEPTypeProfilePriv GHNodeEPTypeProfilePrivate; 
struct _GHNodeEPTypeProfilePriv
{
    GPtrArray     *TypeStrs;
    GStringChunk  *TypeStrStorage;    
};

typedef struct _GHNodeBrowserSourceStruct GHNodeBrowserSource;
struct _GHNodeBrowserSourceStruct
{
    GSource source;
    GHNodeBrowser *obj;
};
*/

/*
typedef struct HNodeEndPointRecord
{
	guint16   MimeTypeLength;
	guint8    MimeTypeStr[128];	
	guint16   Port;

	guint16   AssociateEPIndex;

	guint8    MajorVersion;
	guint8    MinorVersion;
	guint16   MicroVersion;

}GHNodeEndPoint;

typedef struct HNodeRecord
{
	guint8    NodeUID[16];

	guint32   HostAddrLength;
    guint8    HostAddr[256];

	guint8    MajorVersion;
	guint8    MinorVersion;
	guint16   MicroVersion;

    //guint32 			EndPointCount;
	//HNODE_ENDPOINT *EndPointArray;
    GArray   *EndPointList;

    guint32   TimeToLive;  // The number of discovery cycles to wait before removing this entry
    guint32   Flags;
    guint32   Status;

    GHNodePktSrc *HNodeSource;

}GHNodeRecord;
*/
typedef struct _GHNodeServerPrivate GHNodeServerPrivate;
struct _GHNodeServerPrivate
{
    //GHNodeServerSource *Source;
    
    guint32 MaxObjectID;

    AvahiClient     *AvahiClient;
    AvahiEntryGroup *ServiceGroup;
    char            *ServiceName;

    GHNodePktSrc    *ListenSource;

    GHNodePktSrc    *DebugSource;
    GHNodePktSrc    *EventSource;

    GHNodePktSrc    *HNodeSource;

    //struct sockaddr_in discaddr;
    //GPollFD discfd;  

    //GList *MgmtServers;

    //GHNodeMgmtServPrivate  *CurMgmtSrv;
    //GPollFD HMfd;

    //GList *ProfileFilter;
            
    guint32  NextProviderID;
    GList   *HNodeList;
    //GList *EndPoints;
    //GHashTable *EndPointTypeHash;
  
    GList *ClientList;

    gchar    daemonIDStr[64];
    pid_t    daemonPID;
    gboolean isDaemon;
    gboolean isParent;

    gboolean                   dispose_has_run;

};



/*
enum
{
	PROP_0
};
*/

enum
{
    STATE_EVENT,
	LAST_SIGNAL
};

static guint g_hnode_server_signals[LAST_SIGNAL] = { 0 };

/* GObject callbacks */
static void g_hnode_server_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_server_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);


static GObjectClass *parent_class = NULL;

static void
g_hnode_server_dispose (GObject *obj)
{
    GHNodeServer          *self = (GHNodeServer *)obj;
    GHNodeServerPrivate   *priv;

	priv = G_HNODE_SERVER_GET_PRIVATE(self);

    if(priv->dispose_has_run)
    {
        /* If dispose did already run, return. */
        return;
    }

    /* Make sure dispose does not run twice. */
    priv->dispose_has_run = TRUE;

    /* 
    * In dispose, you are supposed to free all types referenced from this
    * object which might themselves hold a reference to self. Generally,
    * the most simple solution is to unref all members on which you own a 
    * reference.
    */


    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
g_hnode_server_finalize (GObject *obj)
{
    GHNodeServer *self = (GHNodeServer *)obj;
    
    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_server_class_init (GHNodeServerClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_server_dispose;
    o_class->finalize = g_hnode_server_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	g_hnode_server_signals[STATE_EVENT] = g_signal_new (
			"state_change",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeServerClass, hnode_found),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

    parent_class = g_type_class_peek_parent (class);   
	g_type_class_add_private (o_class, sizeof (GHNodeServerPrivate));
}

static void
g_hnode_server_init (GHNodeServer *sb)
{
	GHNodeServerPrivate *priv;
    //GHNodeServerSource  *NodeSource;

	priv = G_HNODE_SERVER_GET_PRIVATE (sb);
         
    // Initilize the ObjID value
    priv->MaxObjectID  = 0;
    priv->ServiceName  = NULL;
    priv->ServiceGroup = NULL;
    priv->ListenSource = NULL;
    priv->DebugSource  = NULL;
    priv->EventSource  = NULL;
    priv->AvahiClient  = NULL;
 
    priv->HNodeList    = NULL;

    priv->ClientList   = NULL;

    g_stpcpy( priv->daemonIDStr, "hmnoded" );
    priv->daemonPID    = 0;
    priv->isDaemon     = FALSE;
    priv->isParent     = FALSE;

    priv->dispose_has_run = FALSE;

    priv->NextProviderID = 0;
}

/*
static void
g_hnode_server_set_property (GObject 	*object,
				guint		 prop_id,
				const GValue	*value,
				GParamSpec	*pspec)
{
	GHNodeServer *sb;
	GHNodeServerPrivate *priv;

	sb = G_HNODE_SERVER (object);
	priv = G_HNODE_SERVER_GET_PRIVATE (object);

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
					pspec);
			break;
	}
}

static void
g_hnode_server_get_property (GObject		*object,
				guint		 prop_id,
				GValue		*value,
				GParamSpec	*pspec)
{
	GHNodeServer *sb;
	GHNodeServerPrivate *priv;

	sb = G_HNODE_SERVER (object);
	priv = G_HNODE_SERVER_GET_PRIVATE (object);
	
	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
					pspec);
			break;
	}
}
*/

void 
g_hnode_server_provider_state(GHNodeProvider *sb, guint32 newstate, gpointer data)
{
    GList *CurElem;
    GHNodeClient *Client;

    GHNodeServerPrivate *priv;
    GHNodeServer        *Server = data;
	
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);
    
    switch( newstate )
    {
        // Provider Info is complete; determine whether the node should be claimed.
        case GHNP_STATE_INFO_COMPLETE:
        {
            GHNodeAddress *DebugAddrObj;
            GHNodeAddress *EventAddrObj;

            DebugAddrObj = g_hnode_pktsrc_get_address_object(priv->DebugSource);
            EventAddrObj = g_hnode_pktsrc_get_address_object(priv->EventSource);

            g_hnode_provider_claim(sb, priv->NextProviderID, g_hnode_address_GetPort(DebugAddrObj), g_hnode_address_GetPort(EventAddrObj));
            priv->NextProviderID += 1;
        }
        break;

        // Provider has been claimed and is now ready for transactions.
        case GHNP_STATE_READY:
        {
            // Notify all client nodes of the new hnode.
            CurElem = g_list_first(priv->ClientList);
            while( CurElem )
            {
                Client = CurElem->data;

                // Notify this client node of the new hnode.
                g_hnode_client_add_node(Client, sb);

                // Next client node
                CurElem = g_list_next(CurElem);
            }
        }
    }

	return;
}

void 
g_hnode_server_provider_txreq(GHNodeProvider *sb, gpointer data)
{
    GHNodePacket        *Packet;
    GHNodeServerPrivate *priv;
    GHNodeServer        *Server = data;
	
//    class = G_HNODE_SERVER_GET_CLASS (Server);
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    Packet = g_hnode_packet_new();

    g_hnode_provider_generate_tx_request(sb, Packet);

    g_hnode_pktsrc_send_packet(priv->HNodeSource, Packet);

	return;
}

/* Callback for Avahi API Timeout Event */
static void
g_hnode_server_avahi_timeout_event (AVAHI_GCC_UNUSED AvahiTimeout *timeout, AVAHI_GCC_UNUSED void *userdata)
{
}
 
/* Callback for GLIB API Timeout Event */
static gboolean
g_hnode_server_avahi_timeout_event_glib (void *userdata)
{
    GMainLoop *loop = userdata;
 
    /* Quit the application */
    g_main_loop_quit (loop);
 
    return FALSE; /* Don't re-schedule timeout event */
}
 
static void g_hnode_server_create_services( GHNodeServer *Server );

static void 
g_hnode_server_entry_group_callback(AvahiEntryGroup *g,
                     AvahiEntryGroupState state,
                     void* userdata) 
{
    GHNodeServerClass   *class;
    GHNodeServerPrivate *priv;
    GHNodeServerEvent    hbevent;

    GHNodeServer *Server = userdata;
	
    class = G_HNODE_SERVER_GET_CLASS (Server);
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    //assert(g == Context->ServiceGroup);
 
    /* Called whenever the entry group state changes */ 
    switch (state) 
    {
        case AVAHI_ENTRY_GROUP_ESTABLISHED :
            /* The entry group has been established successfully */
        break;
 
        case AVAHI_ENTRY_GROUP_COLLISION : 
        {
            char *n;
             
            /* A service name collision happened. Let's pick a new name */
            n = avahi_alternative_service_name(priv->ServiceName);
            avahi_free(priv->ServiceName);
            priv->ServiceName = n;
             
            g_error("Service name collision, renaming service to '%s'\n", priv->ServiceName);
             
            /* And recreate the services */
            g_hnode_server_create_services(Server);
            break;
        }
 
        case AVAHI_ENTRY_GROUP_FAILURE :
 
            g_error("Entry group failure: %s\n", avahi_strerror(avahi_client_errno(avahi_entry_group_get_client(g))));
 
            /* Some kind of failure happened while we were registering our services */
            //avahi_simple_poll_quit(simple_poll);
        break;
 
        case AVAHI_ENTRY_GROUP_UNCOMMITED:
        case AVAHI_ENTRY_GROUP_REGISTERING:
        break;
    }
}
 
static void 
g_hnode_server_create_services( GHNodeServer *Server ) 
{
    char r[128];
    int ret;
    GHNodeAddress       *AddrObj;
    GHNodeServerClass   *class;
    GHNodeServerPrivate *priv;
    GHNodeServerEvent    hbevent;
	
    class = G_HNODE_SERVER_GET_CLASS (Server);
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    /* If this is the first time we're called, let's create a new entry group */
    if (!priv->ServiceGroup)
    {
        priv->ServiceGroup = avahi_entry_group_new(priv->AvahiClient, g_hnode_server_entry_group_callback, Server);
        if(!priv->ServiceGroup)        
        {
            g_error("avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(priv->AvahiClient)));
            goto fail;
        }
    }
          
    /* Create some random TXT data */
    g_snprintf(r, sizeof(r), "random=%i", rand());

    AddrObj = g_hnode_pktsrc_get_address_object(priv->ListenSource);

    /* Add the service for _hmnode._tcp */
    if ((ret = avahi_entry_group_add_service(priv->ServiceGroup, 
                                             AVAHI_IF_UNSPEC, 
                                             AVAHI_PROTO_INET, 
                                             0, priv->ServiceName, 
                                             "_hmnode._tcp", 
                                             NULL, NULL, 
                                             g_hnode_address_GetPort(AddrObj),
                                             "test=blah", r, NULL)) < 0) 
    {
        g_error("Failed to add _hmnode._tcp service: %s\n", avahi_strerror(ret));
        goto fail;
    }
 
     
    /* Tell the server to register the service */
    if ((ret = avahi_entry_group_commit(priv->ServiceGroup)) < 0) 
    {
         g_error("Failed to commit entry_group: %s\n", avahi_strerror(ret));
         goto fail;
    }
 

    return;
 
fail:
    g_error("Fail...\n");

    //avahi_simple_poll_quit(simple_poll);
}

static void 
g_hnode_server_resolve_callback(
 	    AvahiServiceResolver *r,
 	    AVAHI_GCC_UNUSED AvahiIfIndex interface,
 	    AVAHI_GCC_UNUSED AvahiProtocol protocol,
 	    AvahiResolverEvent event,
 	    const char *name,
 	    const char *type,
 	    const char *domain,
 	    const char *host_name,
 	    const AvahiAddress *address,
 	    uint16_t port,
 	    AvahiStringList *txt,
 	    AvahiLookupResultFlags flags,
 	    AVAHI_GCC_UNUSED void* userdata) 
{ 	
    GHNodeProvider      *HnodePtr;
    GHNodeServerClass   *class;
    GHNodeServerPrivate *priv;
    GHNodeServerEvent    hbevent;

    GHNodeServer *Server = userdata;
	
    class = G_HNODE_SERVER_GET_CLASS (Server);
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    assert(r);
 	

    /* Called whenever a service has been resolved successfully or timed out */
 	
    switch (event) 
    {
        case AVAHI_RESOLVER_FAILURE:
 	        g_error ("(Resolver) Failed to resolve service '%s' of type '%s' in domain '%s': %s\n", name, type, domain, avahi_strerror(avahi_client_errno(avahi_service_resolver_get_client(r))));
 	    break;
 	
 	    case AVAHI_RESOLVER_FOUND: {
 	        char a[AVAHI_ADDRESS_STR_MAX], *t;
 	           
 	        //g_message ("Service '%s' of type '%s' in domain '%s':\n", name, type, domain);
 	           
 	        avahi_address_snprint(a, sizeof(a), address);
 	        t = avahi_string_list_to_string(txt);
// 	        g_message("\t%s:%u (%s)\n"
// 	                 "\tTXT=%s\n"
// 	                 "\tcookie is %u\n"
// 	                 "\tis_local: %i\n"
// 	                 "\tour_own: %i\n"
// 	                 "\twide_area: %i\n"
// 	                 "\tmulticast: %i\n"
// 	                 "\tcached: %i\n",
// 	                 host_name, port, a,
// 	                 t,
// 	                 avahi_string_list_get_service_cookie(txt),
// 	                 !!(flags & AVAHI_LOOKUP_RESULT_LOCAL),
// 	                 !!(flags & AVAHI_LOOKUP_RESULT_OUR_OWN),
// 	                 !!(flags & AVAHI_LOOKUP_RESULT_WIDE_AREA),
// 	                 !!(flags & AVAHI_LOOKUP_RESULT_MULTICAST),
// 	                 !!(flags & AVAHI_LOOKUP_RESULT_CACHED));

            if( strcmp("_hnode._udp", type) == 0 )
            {
                gchar AddrStr[128];

                HnodePtr = g_hnode_provider_new();

                g_snprintf(AddrStr, sizeof(AddrStr), "hnode://%s:%u", a, port);
                g_hnode_provider_set_address(HnodePtr, AddrStr);

                g_signal_connect(HnodePtr, "state_change", G_CALLBACK(g_hnode_server_provider_state), Server);
                g_signal_connect(HnodePtr, "tx_request", G_CALLBACK(g_hnode_server_provider_txreq), Server);

                // Keep track of servers in a list.
                priv->HNodeList = g_list_append(priv->HNodeList, HnodePtr);

                g_hnode_provider_start(HnodePtr);

                // Notify interested parties about the new service.
                //hbevent.eventtype = G_HNODE_ADD;
                //hbevent.ObjID     = PRecord->ObjID;

        	    //g_signal_emit(Server, g_hnode_server_signals[STATE_EVENT], 0, NULL); //&hbevent);
            }

 	        avahi_free(t);
 	    }
 	}
 	
 	avahi_service_resolver_free(r);
}
 	
static void 
g_hnode_server_browse_callback(
 	    AvahiServiceBrowser *b,
 	    AvahiIfIndex interface,
 	    AvahiProtocol protocol,
 	    AvahiBrowserEvent event,
 	    const char *name,
 	    const char *type,
 	    const char *domain,
 	    AVAHI_GCC_UNUSED AvahiLookupResultFlags flags,
 	    void* userdata) 
{
   GHNodeServerClass   *class;
   GHNodeServerPrivate *priv;
   GHNodeServerEvent    hbevent;

   GHNodeServer *Server = userdata;
	
   class = G_HNODE_SERVER_GET_CLASS (Server);
   priv = G_HNODE_SERVER_GET_PRIVATE (Server);
   assert(b);
 	
    /* Called whenever a new services becomes available on the LAN or is removed from the LAN */
 	
    switch (event)
    {
        case AVAHI_BROWSER_FAILURE:       
 	        g_error ("(Browser) %s\n", avahi_strerror(avahi_client_errno(avahi_service_browser_get_client(b))));
 	        //avahi_simple_poll_quit(simple_poll);
 	    return;
 	
 	    case AVAHI_BROWSER_NEW: 	
            /* We ignore the returned resolver object. In the callback
               function we free it. If the server is terminated before
               the callback function is called the server will free
               the resolver for us. */
            if (!(avahi_service_resolver_new(priv->AvahiClient, interface, protocol, name, type, domain, AVAHI_PROTO_INET, 0, g_hnode_server_resolve_callback, Server)))
                 g_error ("Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(priv->AvahiClient)));
 	           
 	    break;
 	
 	    case AVAHI_BROWSER_REMOVE:
 	    break;
 	
 	    case AVAHI_BROWSER_ALL_FOR_NOW:
 	    case AVAHI_BROWSER_CACHE_EXHAUSTED:
 	    break;
    }
} 

/* Callback for state changes on the Client */
static void
g_hnode_server_avahi_client_callback (AVAHI_GCC_UNUSED AvahiClient *client, AvahiClientState state, void *userdata)
{
   GHNodeServerClass   *class;
   GHNodeServerPrivate *priv;
   GHNodeServerEvent    hbevent;

   GHNodeServer *Server = userdata;
	
   class = G_HNODE_SERVER_GET_CLASS (Server);
   priv = G_HNODE_SERVER_GET_PRIVATE (Server);

   switch( state )
   {
       case AVAHI_CLIENT_S_RUNNING:  
           /* The server has startup successfully and registered its host
            * name on the network, so it's time to create our services */
           if (!priv->ServiceGroup)
           {
               priv->AvahiClient = client;
               g_hnode_server_create_services( Server );
           }
       break;
 
       case AVAHI_CLIENT_FAILURE:        
           /* We we're disconnected from the Daemon */
           g_error ("Disconnected from the Avahi Daemon: %s", avahi_strerror(avahi_client_errno(client)));

           /* Quit the application */
           //g_main_loop_quit (Context->Loop);
             
       break;
 
       case AVAHI_CLIENT_S_COLLISION:
           /* Let's drop our registered services. When the server is back
            * in AVAHI_SERVER_RUNNING state we will register them
            * again with the new host name. */
             
       case AVAHI_CLIENT_S_REGISTERING:
           /* The server records are now being established. This
            * might be caused by a host name change. We need to wait
            * for our own records to register until the host name is
            * properly esatblished. */
             
           if (priv->ServiceGroup)
               avahi_entry_group_reset(priv->ServiceGroup);
     
       break;
 
       case AVAHI_CLIENT_CONNECTING:
       break;
   }
}

void 
g_hnode_server_client_state(GHNodeClient *sb, guint NewState, gpointer data)
{
    GHNodeServerPrivate *priv;

    GHNodeServer *Server = data;
	
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    // This indicates that a client has terminated it's connection
    // Clean-up the client list and free the client object.
    priv->ClientList = g_list_remove(priv->ClientList, sb);

    // Free the client block
    g_object_unref(sb);

}

void 
g_hnode_server_newsrc(GHNodePktSrc *sb, GHNodePktSrc *newsrc, gpointer data)
{
    GHNodeClient *Client;
    GHNodeServerClass   *class;
    GHNodeServerPrivate *priv;

    GList *CurElem;
    GHNodeProvider *ProviderNode;

    GHNodeServer *Server = data;
	
    class = G_HNODE_SERVER_GET_CLASS (Server);
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    // Setup a client record to track the new client.
    Client = g_hnode_client_new();

    // Notify us when things shutdown, etc.
    g_signal_connect(Client, "state_change", G_CALLBACK(g_hnode_server_client_state), Server);

    // Associate the TCP connection with the client object
    g_hnode_client_set_client_source(Client, newsrc);

    // Have the server remember this client.
    priv->ClientList  = g_list_append(priv->ClientList, Client);

    // Scan the list looking for a client node that needs to be updated.
    CurElem = g_list_first(priv->HNodeList);
    while( CurElem )
    {
        ProviderNode = CurElem->data;

        if( g_hnode_provider_is_ready( ProviderNode ) )
        {
            g_hnode_client_add_node(Client, ProviderNode);
        }

        // Check the next client node
        CurElem = g_list_next(CurElem);
    }

    // turn on the client processing
    g_hnode_client_start(Client);

	return;
}

void 
g_hnode_server_debugrx(GHNodePktSrc *sb, GHNodePacket *RxPacket, gpointer data)
{
    GHNodeServer *Server = data;
    GHNodeServerPrivate *priv;

    guint32 RxType;      
    guint32 RxSeqCnt;   
    guint32 RxSvrObjID; 
    guint32 RxEPIndex;  
    guint32 RxDataLength; 
    guint8  RxDataBuffer[1500];

    GHNodeProvider *Provider;
    GHNodeAddress  *RxAddr;

    GList *CurElem;
    GHNodeClient *Client;
    GHNodePacket *TxPacket;

    g_print("g_hnode_server_debugrx -- PktSrc: 0x%x, Packet: 0x%x, Server: 0x%x\n", sb, RxPacket, Server);

    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    // Extract the data from the RX Frame.
    RxType     = g_hnode_packet_get_short(RxPacket); 
    RxSeqCnt   = g_hnode_packet_get_short(RxPacket);
    RxSvrObjID = g_hnode_packet_get_uint(RxPacket);
    RxEPIndex  = g_hnode_packet_get_short(RxPacket);
    RxDataLength = g_hnode_packet_get_short(RxPacket);

    g_print("g_hnode_server_debugrx -- Packet: 0x%x, RxType: 0x%x, RxEPIndex: 0x%x, RxDataLength: 0x%x\n", RxPacket, RxType, RxEPIndex, RxDataLength);

    g_hnode_packet_get_bytes(RxPacket, RxDataBuffer, RxDataLength);


    // Validate the debug frame data.
    // Build a Debug Frame to alert the clients with.
    TxPacket = g_hnode_packet_new();

    // Fill in some header information.                
    // Write the request length
    g_hnode_packet_set_uint(TxPacket, ((8*4)+RxDataLength) );
    g_hnode_packet_set_uint(TxPacket, HNODE_MGMT_DEBUG_PKT);
    g_hnode_packet_set_uint(TxPacket, 0); // No Req tag since there is no response.

    // Write the Debug headers
    g_hnode_packet_set_uint(TxPacket, RxType);
    g_hnode_packet_set_uint(TxPacket, RxSeqCnt);
    g_hnode_packet_set_uint(TxPacket, RxSvrObjID);
    g_hnode_packet_set_uint(TxPacket, RxEPIndex);

    // Write the Debug Data
    g_hnode_packet_set_uint(TxPacket, RxDataLength);
    g_hnode_packet_set_bytes(TxPacket, RxDataBuffer, RxDataLength);

    // Notify interested client nodes of the debug packet.
    CurElem = g_list_first(priv->ClientList);
    while( CurElem )
    {
        Client = CurElem->data;

        // Notify this client node of the new hnode.
        g_hnode_client_forward_debug_packet(Client, TxPacket);

        // Next client node
        CurElem = g_list_next(CurElem);
    }

    // Unref our copy, the seperate tx routines take
    // out there own ref, until tx is complete.
    g_object_unref(TxPacket);

	return;

/*
    U8_T                Buf[2048];
    U32_T               TLength;
    int                 Indx;
    U8_T                TxBuf[2048];
    U8_T               *PktPos;
    SOCK_HDLR          *Client;
    HNODE_RECORD       *CurProvider = NULL;
    struct sockaddr_in  RxAddr;
    socklen_t           RxAddrLen;

    printf("\nProcessDebugRx - start\n");

    // Init the address length field.
    RxAddrLen = sizeof(RxAddr);    
  
    // Pull in the data.
    TLength = recvfrom(gHnodeDebugSock, Buf, sizeof(Buf), 0,
                             (struct sockaddr *)&RxAddr, &RxAddrLen);

	// If the receive Length is zero then 
	// the other end has closed the socket 
    // return this info to the caller.    
	if( TLength == 0 )
		return;

    // Look up the provider this came from
    CurProvider = ProviderLookupProviderByAddr(&Providers, 
                                                (struct sockaddr *)&RxAddr, RxAddrLen);

    if(CurProvider == NULL)
        return;

    // Build the client debug packet 
    PktPos = TxBuf;
    PktPos = PacketFormatU32(PktPos, HNODE_MGMT_DEBUG_PKT);
    PktPos = PacketFormatData(PktPos, CurProvider->NodeUID, 16);
    PktPos = PacketFormatU32(PktPos, TLength);
    PktPos = PacketFormatData(PktPos, Buf, TLength);
   
    // Distribute the Debug packet to interested listners.
    for( Indx = 3; Indx < FileDescCount; Indx++)
    {
        Client = &FileHdlrArray[Indx];

        printf("\nClient Check (0x%x) - enable: 0x%x  State: 0x%x\n",
                    Client, Client->Debug.Enable, Client->State);
        
        // Check if this client wants to receive debug packets.
        if( Client->Debug.Enable == TRUE && Client->State == SHDLR_IDLE )
        {
            printf("\nClient Send (0x%x) - length: %d bytes\n", Client, (PktPos - TxBuf));

            // Put the packet in the socket.
			send(Client->curSock, TxBuf, (PktPos - TxBuf), 0);
        }
             
    }
*/
}

void 
g_hnode_server_eventrx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{

	return;
}

void 
g_hnode_server_hnoderx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    GList *CurProvider;
    //GHNodePacket        *Packet;
    GHNodeServerPrivate *priv;
    GHNodeServer        *Server = data;
	
//    class = G_HNODE_SERVER_GET_CLASS (Server);
    priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    // Loop through the provider's looking for someone to claim this packet.
    CurProvider = g_list_first(priv->HNodeList);
    while( CurProvider )
    {
        if( g_hnode_provider_claim_rx_packet(CurProvider->data, Packet) )
        {
            g_hnode_provider_process_rx_reply(CurProvider->data, Packet);
            break;
        }

        CurProvider = g_list_next(CurProvider);  
    }

    // Free the RX frame
    g_object_unref(Packet);

	return;
}
 
void 
g_hnode_server_hnodetx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    // Free the TX frame
    g_object_unref(Packet);

	return;
}
  
GHNodeServer *
g_hnode_server_new (void)
{
	return g_object_new (G_TYPE_HNODE_SERVER, NULL);
}
   

/**
 * Kicks off managment server discovery
 */
gboolean
g_hnode_server_start(GHNodeServer *Server)
{
    guint8 PktBuf[512];
    gint   PktSize;
    struct sockaddr_in toAddr;

    const AvahiPoll *poll_api;
    AvahiGLibPoll *glib_poll;
    //AvahiClient *client;
    AvahiServiceBrowser *sb = NULL;
    struct timeval tv;
    const char *version;
    int error;

    GHNodeAddress       *AddrObj;

	GHNodeServerClass   *class;
	GHNodeServerPrivate *priv;
	GHNodeServerEvent    hbevent;
	
	class = G_HNODE_SERVER_GET_CLASS (Server);
	priv = G_HNODE_SERVER_GET_PRIVATE (Server);

 
    /* Optional: Tell avahi to use g_malloc and g_free */
    avahi_set_allocator (avahi_glib_allocator ());
 
    /* Create the GLIB main loop */
    //priv->Loop = g_main_loop_new (NULL, FALSE);

    // Give this service a name
    priv->ServiceName = avahi_strdup("HnodeManager");
    priv->ServiceGroup = NULL;

    /* Create the GLIB Adaptor */
    glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
    poll_api = avahi_glib_poll_get (glib_poll);

    // Open up a listener socket source
    priv->ListenSource = g_hnode_pktsrc_new(PST_HNODE_TCP_HMNODE_LISTEN);    
    //priv->ListenSource = pktsrc_SourceCreate( PST_HNODE_TCP_HMNODE_LISTEN, NULL, g_hnode_server_RxCallback, g_hnode_server_TxCallback, g_hnode_server_StateCallback );
    g_signal_connect(priv->ListenSource, "new_source", G_CALLBACK(g_hnode_server_newsrc), Server);
    g_hnode_pktsrc_start( priv->ListenSource );

    // Setup some sockets to rx debug and event frames
    priv->DebugSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    g_signal_connect(priv->DebugSource, "rx_packet", G_CALLBACK(g_hnode_server_debugrx), Server);

    printf("Debug PktSrc: 0x%x\n", priv->DebugSource);

    //g_signal_connect(priv->DebugSource, "tx_packet", g_hnode_server_debugtx, Server);
    g_hnode_pktsrc_start( priv->DebugSource );

    priv->EventSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    g_signal_connect(priv->EventSource, "rx_packet", G_CALLBACK(g_hnode_server_eventrx), Server);
    //g_signal_connect(priv->EventSource, "tx_packet", g_hnode_server_eventtx, Server);
    g_hnode_pktsrc_start( priv->EventSource );

     // Create the socket that will be used for interacting with hnodes
    priv->HNodeSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    g_signal_connect(priv->HNodeSource, "rx_packet", G_CALLBACK(g_hnode_server_hnoderx), Server);
    g_signal_connect(priv->HNodeSource, "tx_packet", G_CALLBACK(g_hnode_server_hnodetx), Server);
    g_hnode_pktsrc_start( priv->HNodeSource );

//    priv->DebugSource = pktsrc_SourceCreate( PST_HNODE_UDP_ASYNCH, NULL, g_hnode_server_RxCallback, g_hnode_server_TxCallback, g_hnode_server_StateCallback );
//    priv->EventSource = pktsrc_SourceCreate( PST_HNODE_UDP_ASYNCH, NULL, g_hnode_server_RxCallback, g_hnode_server_TxCallback, g_hnode_server_StateCallback );

    AddrObj = g_hnode_pktsrc_get_address_object(priv->ListenSource);
    AddrObj = g_hnode_pktsrc_get_address_object(priv->DebugSource);
    AddrObj = g_hnode_pktsrc_get_address_object(priv->EventSource);

    /* Example, schedule a timeout event with the Avahi API */
    //avahi_elapse_time (&tv,                         /* timeval structure */
    //                   1000,                                   /* 1 second */
    //                   0);                                     /* "jitter" - Random additional delay from 0 to this value */
 
    //poll_api->timeout_new (poll_api,                /* The AvahiPoll object */
    //                      &tv,                          /* struct timeval indicating when to go activate */
    //                       avahi_timeout_event,          /* Pointer to function to call */
    //                       NULL);                        /* User data to pass to function */
 
    /* Schedule a timeout event with the glib api */
    //g_timeout_add (5000,                            /* 5 seconds */
    //                 avahi_timeout_event_glib,               /* Pointer to function callback */
    //                 loop);                                  /* User data to pass to function */
 
    /* Create a new AvahiClient instance */
    priv->AvahiClient = avahi_client_new (poll_api,            /* AvahiPoll object from above */
                               0,
                               g_hnode_server_avahi_client_callback,  /* Callback function for Client state changes */
                               Server,                         /* User data */
                               &error);                          /* Error return */
 
     /* Check the error return code */
     if (priv->AvahiClient == NULL)
     {
         /* Print out the error string */
         g_warning ("Error initializing Avahi: %s", avahi_strerror (error));
 
         goto fail;
     }
    
     /* Make a call to get the version string from the daemon */
     version = avahi_client_get_version_string (priv->AvahiClient);
 
     /* Check if the call suceeded */
     if (version == NULL)
     {
         g_warning ("Error getting version string: %s", avahi_strerror (avahi_client_errno (priv->AvahiClient)));
 
         goto fail;
     }
         
     /* Create the service browser */
     if (!(sb = avahi_service_browser_new(priv->AvahiClient, 
                                          AVAHI_IF_UNSPEC, 
                                          AVAHI_PROTO_INET, 
                                          "_hnode._udp", 
                                          NULL, 0, g_hnode_server_browse_callback, Server))) 
     {
 	     g_warning("Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(priv->AvahiClient)));
 	     goto fail;
     }

     return FALSE;

     fail:
     /* Clean up */
     avahi_client_free (priv->AvahiClient);
     avahi_glib_poll_free (glib_poll);
     return TRUE;

/*
    // Format a SLP request to discover the 
    // management server.  This is sent
    // multicast to 239.255.255.253, port 427
    // Replies will be unicast back to us.
    PktSize = sizeof(gSLPSrvReqTemplate);
    memcpy( PktBuf, gSLPSrvReqTemplate, PktSize );
    PktBuf[4] = PktSize;
    PktBuf[11] = 2;
    
    // Setup socket address info.
    bzero(&toAddr, sizeof(struct sockaddr_in));
    toAddr.sin_family      = AF_INET;
    inet_aton("239.255.255.253", &toAddr.sin_addr);
    toAddr.sin_port        = htons(427);
     
    sendto(priv->discfd.fd, PktBuf, PktSize, 0,
                 (struct sockaddr *)&toAddr, sizeof(toAddr));
*/
}

static gboolean 
g_hnode_server_signal_source_callback( gint signal, gpointer data ) 
{
    GHNodeServer *Server = data;

    switch( signal ) 
    {
        case SIGINT:
            hnode_log_msg(LOG_NOTICE, "Received SIGINT; exiting");
            g_signal_emit( Server, g_hnode_server_signals[STATE_EVENT], 0, NULL ); 
            //g_main_loop_quit(main_loop);
        break;

        case SIGTERM:
            hnode_log_msg(LOG_NOTICE, "Received SIGTERM; exiting");
            g_signal_emit( Server, g_hnode_server_signals[STATE_EVENT], 0, NULL );
        break;

        default:
            hnode_log_msg(LOG_WARNING, "Received unexpected signal %u; ignoring", signal);
        break;
    }

    return TRUE;  // Continue checking this source
}

gboolean
g_hnode_server_init_daemon( GHNodeServer *Server )
{
	GHNodeServerClass   *class;
	GHNodeServerPrivate *priv;
	
	class = G_HNODE_SERVER_GET_CLASS (Server);
	priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    // Reset signal handlers
    if( daemon_reset_sigs(-1) < 0 ) 
    {
        daemon_log( LOG_ERR, "Failed to reset all signal handlers: %s", strerror(errno) );
        return TRUE;
    }

    // Unblock signals
    if( daemon_unblock_sigs(-1) < 0 ) 
    {
        daemon_log( LOG_ERR, "Failed to unblock all signals: %s", strerror(errno) );
        return TRUE;
    }

    // Set identification string for the daemon for both syslog and PID file
    daemon_pid_file_ident = daemon_log_ident = daemon_ident_from_argv0( priv->daemonIDStr );

    // Success
    return FALSE;
}

gboolean
g_hnode_server_stop_daemon( GHNodeServer *Server )
{
	GHNodeServerClass   *class;
	GHNodeServerPrivate *priv;
	
	class = G_HNODE_SERVER_GET_CLASS (Server);
	priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    gint result;

    printf("Trying to stop daemon.\n");

    // Initialize the daemon
    if( g_hnode_server_init_daemon( Server ) )
    {
        g_print("Bad init.\n");
        return TRUE;
    }

    // Send the daemon a SIGTERM to kill it.
    result = daemon_pid_file_kill_wait( SIGTERM, 5 );

    g_print("Wait Kill Response: %d\n", result);

    // See if it worked.
    if( result < 0 )
    {
        daemon_log( LOG_WARNING, "Failed to kill daemon: %s", strerror(errno) );
        return TRUE;
    }

    g_print("Daemon kill done.\n");

    // Success
    return FALSE;
}

gboolean
g_hnode_server_parent_wait_for_daemon_start( GHNodeServer *Server )
{
    /* The parent */
    int ret;

    // Wait for 20 seconds for a signal from the daemon.
    ret = daemon_retval_wait(20);

    // Wait for 20 seconds for the return value passed from the daemon process.
    if( ret < 0 )
    {
        daemon_log( LOG_ERR, "Could not recieve return value from daemon process: %s", strerror( errno ) );
        return 255;
    }

    daemon_log( ret != 0 ? LOG_ERR : LOG_INFO, "Daemon returned %i as return value.", ret );
    return ret;
}

gboolean
g_hnode_server_is_parent( GHNodeServer *Server )
{
	GHNodeServerPrivate *priv;

	priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    return priv->isParent;
}

gboolean
g_hnode_server_start_as_daemon( GHNodeServer *Server )
{
	GHNodeServerClass   *class;
	GHNodeServerPrivate *priv;
	gint                 result;

	class = G_HNODE_SERVER_GET_CLASS (Server);
	priv = G_HNODE_SERVER_GET_PRIVATE (Server);

    // Initialize the daemon
    if( g_hnode_server_init_daemon( Server ) )
        return TRUE;

    // Check if the daemon is already running, and get its pid if it is.
    priv->daemonPID = daemon_pid_file_is_running();

    // Was it running.  Only allow one copy.
    if( priv->daemonPID >= 0 ) 
    {
        daemon_log( LOG_ERR, "Daemon already running on PID file %u", priv->daemonPID );
        return TRUE;
    }

    // Indicate that we are going to start up as a daemon process
    priv->isDaemon = TRUE;

    // Setup for a return value from the child to the parent.
    if( daemon_retval_init() < 0 ) 
    {
        daemon_log( LOG_ERR, "Failed to create pipe." );
        return TRUE;
    }

    // Perform the fork operation.
    priv->daemonPID = daemon_fork(); 

    // Perform the fork operation.
    if( priv->daemonPID < 0 ) 
    {
        // Fork operation failed
        daemon_retval_done();
        return TRUE;
    } 
    else if( priv->daemonPID ) 
    { 
        // Remember that this version is the parent.
        priv->isParent = TRUE;

        // Success
        return FALSE;
    } 
    else 
    { 
        // Remember that this version is the child.
        priv->isParent = FALSE;

        // Running in the deamon process
        // Close stdin, stdout, stderr
        result = daemon_close_all( -1 ); 

        // Close stdin, stdout, stderr
        if( result < 0 ) 
        {
            daemon_log( LOG_ERR, "Failed to close all file descriptors: %s", strerror( errno ) );

            // Tell the parent process about the error
            daemon_retval_send( 1 );

            // Clean up
            daemon_log(LOG_INFO, "Exiting...");
            daemon_retval_send(255);
            daemon_signal_done();
            daemon_pid_file_remove();
            return TRUE;
        }

        // Create the pid file for tracking daemon state
        result = daemon_pid_file_create();

        if( result < 0 ) 
        {
            daemon_log( LOG_ERR, "Could not create PID file (%s).", strerror(errno) );
            daemon_retval_send( 2 );

            // Clean up
            daemon_log(LOG_INFO, "Exiting...");
            daemon_retval_send(255);
            daemon_signal_done();
            daemon_pid_file_remove();
            return TRUE;
        }

        // Setup the signals that the daemon will listern for
        result = daemon_signal_init( SIGINT, SIGTERM, SIGQUIT, SIGHUP, 0 ); 

        // Setup the signals that the daemon will listern for
        if( result < 0 ) 
        {
            daemon_log( LOG_ERR, "Could not register signal handlers (%s).", strerror(errno) );
            daemon_retval_send( 3 );

            // Clean up
            daemon_log(LOG_INFO, "Exiting...");
            daemon_retval_send(255);
            daemon_signal_done();
            daemon_pid_file_remove();
            return TRUE;
        }

        // Tell the parent that things have started 
        daemon_retval_send( 0 );

        daemon_log( LOG_INFO, "Daemon started" );

        // Set up handling of interesting signals
        if( (g_hnode_signal_source_handle_signal( SIGINT ) < 0) || (g_hnode_signal_source_handle_signal( SIGTERM ) < 0)) 
        {
            hnode_log_perror(LOG_WARNING, "Failed to set up signal handling");
        }
        g_hnode_signal_source_add( g_hnode_server_signal_source_callback, Server );

        // Fire up the deamon process
        return g_hnode_server_start( Server );
    }
}


/*
gboolean
g_hnode_browser_open_hmnode( GHNodeBrowser *sb, guint32 MgmtServObjID )
{
    char PktBuf[512];    
    HNODE_MGMT_PKT *HMPktPtr;
    GList *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
	GHNodeEvent hbevent;
	
	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // If we are currently connected, error
    if( priv->CurMgmtSrv != NULL )
        return TRUE;
    
    // Find the referenced management server.
    Element = g_list_first(priv->MgmtServers);
    while(Element)
    {
        priv->CurMgmtSrv = (GHNodeMgmtServPrivate *)Element->data; 
        if( priv->CurMgmtSrv->ObjID == MgmtServObjID )
        {
            break;
        }
        Element = g_list_next(Element);
    }
     
    // Failed to find managment server with ObjID. 
    if( Element == NULL )
    {
        priv->CurMgmtSrv = NULL;     
        return TRUE;
    }
    
    // Attempt to make a connection to this manager.
    // Open up a socket.            
    if ( (priv->HMfd.fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
    {    
        g_printf("Error creating socket.\n");
        return TRUE;
    }
            
    // Try to connect to the management server.
    if ( connect(priv->HMfd.fd, (struct sockaddr *)&priv->CurMgmtSrv->HMAddr, sizeof(priv->CurMgmtSrv->HMAddr)) < 0 )
    {
        g_printf("Error connecting socket.\n");
        return TRUE;
    }
                               
    priv->HMfd.events  = G_IO_IN | G_IO_ERR | G_IO_HUP; // G_IO_OUT
    priv->HMfd.revents = 0;
    
    g_source_add_poll(&priv->Source->source, &priv->HMfd);
                                  
    HMPktPtr = (HNODE_MGMT_PKT *) PktBuf;       

	// Send the request
    HMPktPtr->PktType = HNODE_MGMT_REQ_PINFO;
    send(priv->HMfd.fd, PktBuf, 4, 0);
    
    // Successfully connected.
    return FALSE;
                                            

}

gboolean
g_hnode_browser_set_debug( GHNodeBrowser *sb )
{
    char PktBuf[512];    
    HNODE_MGMT_PKT *HMPktPtr;
    GList *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
	GHNodeEvent hbevent;
	
	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // If not currently connected then nothing to do.
    if( priv->CurMgmtSrv == NULL )
        return TRUE;
                                          
    HMPktPtr = (HNODE_MGMT_PKT *) PktBuf;       

	// Send the debug request
    HMPktPtr->PktType = HNODE_MGMT_DEBUG_CTRL;
    send(priv->HMfd.fd, PktBuf, 4, 0);
    
    // Successfully connected.
    return FALSE;
                                            
}

gboolean
g_hnode_browser_get_provider_info( GHNodeBrowser *sb, 
                                   guint32 ProviderObjID,
                                   GHNodeProvider *Provider )
{
    GList *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;

    GHNodeProviderPrivate *PRecord;
	
	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // Find the referenced Provider.
    Element = g_list_first(priv->Providers);
    while(Element)
    {
        PRecord = (GHNodeProviderPrivate *)Element->data; 
        if( PRecord->ObjID == ProviderObjID )
            break;
            
        Element = g_list_next(Element);
    }
     
    // Failed to find a Provider with ObjID. 
    if( Element == NULL )    
        return TRUE;

    Provider->PortNumber     = PRecord->PortNumber;
    Provider->MicroVersion   = PRecord->MicroVersion;
    Provider->MinorVersion   = PRecord->MinorVersion;
    Provider->MajorVersion   = PRecord->MajorVersion;
    Provider->EndPointCount  = PRecord->EndPointCount;
    Provider->HostAddrType   = PRecord->HostAddrType;
    Provider->HostAddrLength = PRecord->HostAddrLength;

    memcpy(Provider->HostAddr, PRecord->HostAddr, PRecord->HostAddrLength+1); 

    return FALSE;
}

gboolean
g_hnode_browser_get_endpoint_info( GHNodeBrowser *sb,
                                   guint32 ProviderObjID,
                                   guint32 EPIndex,
                                   GHNodeEndPoint *EndPoint )
{
    guint32  Index;
    GList   *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;

    GHNodeProviderPrivate *PRecord;    
    GHNodeEndPointPrivate *EPRecord;

	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // Find the referenced management server.
    Element = g_list_first(priv->Providers);
    while(Element)
    {
        PRecord = (GHNodeProviderPrivate *)Element->data; 
        if( PRecord->ObjID == ProviderObjID )
            break;
            
        Element = g_list_next(Element);
    }
     
    // Failed to find a Provider with ObjID. 
    if( Element == NULL )    
        return TRUE;

    // Make sure the requested endpoint is within bounds
    if( EPIndex >= PRecord->EndPointCount )
        return TRUE;
        
    // Got the provider now find the endpoint
    Element = g_list_first(PRecord->EndPoints);
    for(Index = 0; Index < EPIndex; Index++)
        Element = g_list_next(Element);
        
    EPRecord = (GHNodeEndPointPrivate *)Element->data;

    EndPoint->PortNumber     = EPRecord->PortNumber;
    EndPoint->MicroVersion   = EPRecord->MicroVersion;
    EndPoint->MinorVersion   = EPRecord->MinorVersion;
    EndPoint->MajorVersion   = EPRecord->MajorVersion;
    EndPoint->MimeStrLength  = EPRecord->MimeStrLength;

    memcpy(EndPoint->MimeTypeStr, EPRecord->MimeTypeStr, EPRecord->MimeStrLength+1); 

    return FALSE;
}

gboolean 
g_hnode_browser_build_ep_address( GHNodeBrowser *sb,
                                  guint32 ProviderObjID,
                                  guint32 EPIndex,
                                  struct sockaddr *AddrStruct,
                                  guint32 *AddrLength)
{
    struct sockaddr_in *INAddr = (struct sockaddr_in *)AddrStruct;
    
    guint32  Index;
    GList   *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;

    GHNodeProviderPrivate *PRecord;    
    GHNodeEndPointPrivate *EPRecord;

	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // Find the referenced management server.
    Element = g_list_first(priv->Providers);
    while(Element)
    {
        PRecord = (GHNodeProviderPrivate *)Element->data; 
        if( PRecord->ObjID == ProviderObjID )
            break;
            
        Element = g_list_next(Element);
    }
     
    // Failed to find a Provider with ObjID. 
    if( Element == NULL )    
        return TRUE;

    // Make sure the requested endpoint is within bounds
    if( EPIndex >= PRecord->EndPointCount )
        return TRUE;
        
    // Got the provider now find the endpoint
    Element = g_list_first(PRecord->EndPoints);
    for(Index = 0; Index < EPIndex; Index++)
        Element = g_list_next(Element);
        
    EPRecord = (GHNodeEndPointPrivate *)Element->data;

	// Setup an address
	INAddr->sin_family = AF_INET;
	INAddr->sin_port   = htons(EPRecord->PortNumber);
	inet_aton((char *)PRecord->HostAddr, &INAddr->sin_addr); 

    // Adjust the length
    *AddrLength = sizeof(struct sockaddr_in);
    
    return FALSE;
}

void
g_hnode_browser_add_device_filter(GHNodeBrowser *sb, gchar **RequiredDeviceMimeStrs)
{
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
	GHNodeEPTypeProfilePrivate *Profile;
    gchar    *tmpStr;
    guint32  i;
    
	class = G_HNODE_BROWSER_GET_CLASS (sb);
	priv = G_HNODE_BROWSER_GET_PRIVATE (sb);

    // Create a profile record to remeber this stuff
    Profile = g_new(GHNodeEPTypeProfilePrivate, 1);
    
    Profile->TypeStrs = g_ptr_array_new();
    Profile->TypeStrStorage = g_string_chunk_new(64);
    
    // Add the filter strings
    for(i = 0; RequiredDeviceMimeStrs[i]; i++)
    {
        tmpStr = g_string_chunk_insert_const(Profile->TypeStrStorage, RequiredDeviceMimeStrs[i]);
        g_ptr_array_add(Profile->TypeStrs, (gpointer) tmpStr);
    }
    
    // Add to list of filters.
    priv->ProfileFilter = g_list_append(priv->ProfileFilter, Profile);
}

void
g_hnode_browser_reset_device_filter(GHNodeBrowser *sb)
{
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
	GHNodeEPTypeProfilePrivate *Profile;
	GList *Element;
    
	class = G_HNODE_BROWSER_GET_CLASS (sb);
	priv = G_HNODE_BROWSER_GET_PRIVATE (sb);

    // Pull all filter elements and get rid of them
    Element = g_list_first(priv->ProfileFilter);
    while(Profile)
    {                 
        Profile = (GHNodeEPTypeProfilePrivate *)Element->data;
        g_ptr_array_free(Profile->TypeStrs, TRUE);
        g_string_chunk_free(Profile->TypeStrStorage);
        Element = g_list_next(Element);
    }
        
    // Free up the main list
    g_list_free(priv->ProfileFilter);
    priv->ProfileFilter = NULL;
}
*/




