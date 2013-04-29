/**
 * hnode-browser.c
 *
 * Implements a GObject for hnode discovery
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

#include "hnode-pktsrc.h"
#include "hnode-uid.h"
#include "hnode-provider.h"
#include "hnode-browser.h"


#define G_HNODE_BROWSER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_BROWSER, GHNodeBrowserPrivate))

G_DEFINE_TYPE (GHNodeBrowser, g_hnode_browser, G_TYPE_OBJECT);

typedef struct _GHNodeBrowserPrivate GHNodeBrowserPrivate;
struct _GHNodeBrowserPrivate
{
//    GHNodeBrowserSource *Source;
  
    AvahiClient     *AvahiClient;
    AvahiEntryGroup *ServiceGroup;

    guint32  ResolveCount;
    gboolean  avahi_done; 
  
    guint32 MaxObjectID;

    //struct sockaddr_in discaddr;
    //GPollFD discfd;  

    GList *MPointList;
    GList *HNodeList;

    GList *CurHNode;
    GList *CurMPoint;

    //GHNodeMgmtServPrivate  *CurMgmtSrv;
    //GPollFD HMfd;

    GList *ProfileFilter;
            
    //GList *EndPoints;
    GHashTable *EndPointTypeHash;

    gboolean  dispose_has_run;
};



/*
enum
{
	PROP_0
};
*/

/*
enum
{
	ADD,
	REMOVE,
	START,
	END,
	CONTACT,
    DEBUG,
    EVENT,
	LAST_SIGNAL
};
*/
//static guint g_hnode_browser_signals[LAST_SIGNAL] = { 0 };

/* GObject callbacks */
static void g_hnode_browser_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_browser_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);


static GObjectClass *parent_class = NULL;

static void
g_hnode_browser_dispose (GObject *obj)
{
    GHNodeBrowser        *self = (GHNodeBrowser *)obj;
    GHNodeBrowserPrivate *priv;

	priv = G_HNODE_BROWSER_GET_PRIVATE(self);

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
g_hnode_browser_finalize (GObject *obj)
{
    GHNodeBrowser *self = (GHNodeBrowser *)obj;

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_browser_class_init (GHNodeBrowserClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_browser_dispose;
    o_class->finalize = g_hnode_browser_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	class->hnode_add_id = g_signal_new (
			"hnode-add",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserClass, hnode_add),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->mgmt_add_id = g_signal_new (
			"mgmt-add",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserClass, mgmt_add),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->enum_cmplt_id = g_signal_new (
			"enumeration-complete",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserClass, enum_cmplt),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

    parent_class = g_type_class_peek_parent (class);   
	g_type_class_add_private (o_class, sizeof (GHNodeBrowserPrivate));
}

/*
        // Check if devices are being filtered
        if( priv->ProfileFilter )
        {
            // Check if the newly found device meets the filter criteria.
            // Start out assuming the device should be filtered.
            FilterMatch = FALSE;
            
            Element = g_list_first(priv->ProfileFilter);
            while(Element)
            {                 
                Profile = (GHNodeEPTypeProfilePrivate *)Element->data;
                
                ProfileMatch = TRUE;
                 
                for(Indx = 0; Indx < Profile->TypeStrs->len; Indx++)
                {
                    TypeStr = g_ptr_array_index(Profile->TypeStrs, Indx);
                    
                    TypeMatch = FALSE;
                    
                    // Search for a successful match
                    Element2 = g_list_first(PRecord->EndPoints);
                    while( Element2 )
                    {
                        EPRecord = Element2->data;
                        g_print("Checking Strings: %s == %s\n", TypeStr, EPRecord->MimeTypeStr); 
                        if(g_ascii_strcasecmp(TypeStr, (char *)EPRecord->MimeTypeStr) == 0)
                        {
                           TypeMatch = TRUE;
                           break;
                        }
                        Element2 = g_list_next(Element2);
                    }

                    g_print("TypeMatch: %d\n", TypeMatch); 
       
                    if(TypeMatch == FALSE)
                    {
                        ProfileMatch = FALSE;
                        break;
                    }

                }

                g_print("ProfileMatch: %d\n", ProfileMatch); 
         
                if( ProfileMatch == TRUE )
                {
                    FilterMatch = TRUE;
                }
                
                Element = g_list_next(Element);
            }

            g_print("FilterMatch: %d\n", FilterMatch); 
            
            // Check if the provider passed the check
            if( FilterMatch == FALSE )
            {
                // Remove the provider
                g_print("Removing Provider: %s due to Profile Filter\n", PRecord->HostAddr);
                
                //
                Element2 = g_list_first(PRecord->EndPoints);
                while( Element2 )
                {
                  // Get the pointer record.
                  EPRecord = Element2->data;
                  
                  // Clean the endpoint information from the private data.
                  priv->EndPoints = g_list_remove(priv->EndPoints, EPRecord);
                  g_hash_table_remove(priv->EndPointTypeHash, EPRecord);
                  
                  // Next endpoint
                  Element2 = g_list_next(Element2);
                  
                  // Free the endpoint data structure
                  g_free(EPRecord);
                }

                // Free the provider list of endpoints
                g_list_free(PRecord->EndPoints);
                     
                // Remove the provider from the private data.     
                priv->Providers = g_list_remove(priv->Providers, PRecord);
                
                // Free the provider record.
                g_free(PRecord); // = g_new(GHNodeProviderPrivate, 1);
       
                // Nothing to see here, move along.         
                continue;
            } 
        }
        
        // Generate an event for the new device. 
        hbevent.eventtype = G_HNODE_ADD;
        hbevent.ObjID     = PRecord->ObjID;

	    g_signal_emit(sb, g_hnode_browser_signals[ADD], 0, &hbevent);
    }

}
*/

static gboolean g_hnode_find_objid_by_uid(GHNodeBrowser *sb,
                                         guint8  *UIDArray,
                                         guint32 *ObjID)
{               
    GList *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
//    GHNodeProviderPrivate *PRecord;

	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // Find the referenced Provider.
//    Element = g_list_first(priv->Providers);
//    while(Element)
//    {
//        PRecord = (GHNodeProviderPrivate *)Element->data; 
//        if( memcmp(PRecord->NodeUID, UIDArray, 16) == 0 )
//            break;
            
//        Element = g_list_next(Element);
//    }
     
    // Failed to find a Provider match. 
//    if( Element == NULL )    
//        return TRUE;

    // Provider Found
//    *ObjID = PRecord->ObjID;
    return FALSE;
}

/*
static void g_hnode_parse_hmnode_debug(GHNodeBrowserSource  *NodeSource, GHNodeBrowser *sb, GHNodeBrowserPrivate *priv)
{
    guint32 PCount;
    guint32 ObjID;
	GHNodeEvent hbevent;    
    guint8  DbgPkt[2048];
    guint8  OriginUID[16];
/*
    // Read the UID information    
    SockWaitForData( priv->HMfd.fd, OriginUID, 16);

    printf("Debug RX - OriginUID: 0x%8.8x 0x%8.8x 0x%8.8x 0x%8.8x\n",
                *((guint32 *)&OriginUID[0]), *((guint32 *)&OriginUID[4]),
                *((guint32 *)&OriginUID[8]), *((guint32 *)&OriginUID[12]));

    // Pull the debug packet from the socket.
    PCount = SockWaitForU32( priv->HMfd.fd );
    SockWaitForData( priv->HMfd.fd, DbgPkt, PCount);

    printf("Debug RX - %d bytes\n", PCount);

    // Find originator ObjID based on UID
    // If the object can't be resolved then don't emit the debug
    if( g_hnode_find_objid_by_uid(sb, OriginUID, &ObjID) )
        return;

    printf("Debug RX - ObjID=%d\n", ObjID);
   
    // Generate an event for the debug packet. 
    hbevent.eventtype = G_HNODE_DEBUG;
    hbevent.ObjID     = ObjID;
    hbevent.PktPtr    = DbgPkt;
    hbevent.PktLength = PCount;
    
    g_signal_emit(sb, g_hnode_browser_signals[DEBUG], 0, &hbevent);

}
*/


static void
g_hnode_browser_init (GHNodeBrowser *sb)
{
	GHNodeBrowserPrivate *priv;
//    GHNodeBrowserSource  *NodeSource;

	priv = G_HNODE_BROWSER_GET_PRIVATE (sb);

    // Init
    //priv->HMfd.fd = 0;
    //priv->HMfd.revents = 0;
    
    // Setup the socket to listen for debug info   
    //priv->discfd.fd = socket(AF_INET, SOCK_DGRAM, 0);
    //if (priv->discfd.fd < 0) exit(1);
   
    // Setup socket address info.
    //bzero(&priv->discaddr, sizeof(struct sockaddr_in));
    //priv->discaddr.sin_family      = AF_INET;
    //priv->discaddr.sin_addr.s_addr = INADDR_ANY;
    //priv->discaddr.sin_port        = htons(0xDEAD);
   
    // Bind the socket to the address
    //if( bind(priv->discfd.fd, (struct sockaddr *)&priv->discaddr, sizeof(struct sockaddr_in)) < 0 ) 
    //    exit(1);
          
    // Initilize the ObjID value
    priv->MaxObjectID = 0;

    priv->MPointList  = NULL;
 
    // Clear out provider profiles
    //priv->ProfileFilter = NULL;
    
    // References to discovered providers.
    priv->HNodeList = NULL;
    //priv->EndPoints = NULL;
	priv->EndPointTypeHash = g_hash_table_new(g_str_hash, g_str_equal);

    priv->dispose_has_run = FALSE;
   
    priv->CurHNode  = NULL;
    priv->CurMPoint = NULL;
           
    priv->ResolveCount = 0;
    priv->avahi_done   = FALSE; 
 
}

/*
static void
g_hnode_browser_set_property (GObject 	*object,
				guint		 prop_id,
				const GValue	*value,
				GParamSpec	*pspec)
{
	GHNodeBrowser *sb;
	GHNodeBrowserPrivate *priv;

	sb = G_HNODE_BROWSER (object);
	priv = G_HNODE_BROWSER_GET_PRIVATE (object);

	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
					pspec);
			break;
	}
}

static void
g_hnode_browser_get_property (GObject		*object,
				guint		 prop_id,
				GValue		*value,
				GParamSpec	*pspec)
{
	GHNodeBrowser *sb;
	GHNodeBrowserPrivate *priv;

	sb = G_HNODE_BROWSER (object);
	priv = G_HNODE_BROWSER_GET_PRIVATE (object);
	
	switch (prop_id)
	{
		default:
			G_OBJECT_WARN_INVALID_PROPERTY_ID (object, prop_id,
					pspec);
			break;
	}
}
*/


GHNodeBrowser *
g_hnode_browser_new (void)
{
	return g_object_new (G_TYPE_HNODE_BROWSER, NULL);
}

static void
g_hnode_browser_mpoint_hnode_new(GHNodeBrowserMPoint *sb, gpointer nodeptr, gpointer data)
{
    GHNodeBrowserClass   *class;
    GHNodeBrowserPrivate *priv;

    GHNodeBrowser *Browser = data;
	
    class = G_HNODE_BROWSER_GET_CLASS (Browser);
    priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

    GHNodeBrowserHNode *Node = (GHNodeBrowserHNode *)nodeptr;

    // Add the New node to our internal list.
    priv->HNodeList = g_list_append(priv->HNodeList, Node);

    // Tell the upper layer about the new node record.
    g_signal_emit(Browser, class->hnode_add_id, 0, Node);

}

static void
g_hnode_browser_mpoint_cmnd_done(GHNodeBrowserMPoint *sb, gpointer event)
{
    g_message("g_hnode_browser_mpoint_cmnd_done\n");
}

static void
g_hnode_browser_mpoint_remove(GHNodeBrowserMPoint *sb, gpointer event)
{
    g_message("g_hnode_browser_mpoint_remove\n");
}

/*
static void
g_hnode_browser_mpoint_hnode_debug_rx(GHNodeBrowserMPoint *sb, gpointer event)
{
    g_message("g_hnode_browser_mpoint_debug_rx\n");
}

static void
g_hnode_browser_mpoint_hnode_event_rx(GHNodeBrowserMPoint *sb, gpointer event)
{
    g_message("g_hnode_browser_mpoint_event_rx\n");
}
*/
         
static void 
g_hnode_browser_resolve_callback(
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
    GHNodeBrowserClass   *class;
    GHNodeBrowserPrivate *priv;
    GHNodeBrowserMPoint  *MPtr;

    GHNodeBrowser *Browser = userdata;
	
    class = G_HNODE_BROWSER_GET_CLASS (Browser);
    priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

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
//	                 "\tTXT=%s\n"
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

            if( strcmp("_hmnode._tcp", type) == 0 )
            {
                gchar AddrStr[128];

                MPtr = g_hnode_browser_mpoint_new();

                g_snprintf(AddrStr, sizeof(AddrStr), "hmnode://%s:%u", a, port);
                g_hnode_browser_mpoint_set_address(MPtr, AddrStr);

                g_signal_connect(MPtr, "hnode-add", G_CALLBACK(g_hnode_browser_mpoint_hnode_new), Browser);
                g_signal_connect(MPtr, "cmnd-done", G_CALLBACK(g_hnode_browser_mpoint_cmnd_done), Browser);
                g_signal_connect(MPtr, "remove",    G_CALLBACK(g_hnode_browser_mpoint_remove), Browser);
                //g_signal_connect(MPtr, "debug-rx", G_CALLBACK(g_hnode_browser_mpoint_hnode_debug_rx), Browser);
                //g_signal_connect(MPtr, "event-rx", G_CALLBACK(g_hnode_browser_mpoint_hnode_event_rx), Browser);

                // Keep track of servers in a list.
                priv->MPointList = g_list_append(priv->MPointList, MPtr);

                g_hnode_browser_mpoint_start(MPtr);

                // Tell the upper layer about the new management point.
                g_signal_emit(Browser, class->mgmt_add_id, 0, MPtr);

            }

 	        avahi_free(t);
 	    }
 	}

    // Check if the enumeration of nodes is complete.
    g_print("Resolve complete\n");
    priv->ResolveCount -= 1; 	
    if( (priv->ResolveCount == 0) && (priv->avahi_done == TRUE) )
    {
        g_signal_emit(Browser, class->enum_cmplt_id, 0, NULL);        
    }

 	avahi_service_resolver_free(r);
}

static void 
g_hnode_browser_browse_callback(
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
   GHNodeBrowserClass   *class;
   GHNodeBrowserPrivate *priv;

   GHNodeBrowser *Browser = userdata;
	
   class = G_HNODE_BROWSER_GET_CLASS (Browser);
   priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);
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
            priv->ResolveCount += 1;
            if (!(avahi_service_resolver_new(priv->AvahiClient, interface, protocol, name, type, domain, AVAHI_PROTO_INET, 0, g_hnode_browser_resolve_callback, Browser)))
            {
                 priv->ResolveCount -= 1;
                 g_error ("Failed to resolve service '%s': %s\n", name, avahi_strerror(avahi_client_errno(priv->AvahiClient)));
 	        }  
 	    break;
 	
 	    case AVAHI_BROWSER_REMOVE:
 	    break;
 	
 	    case AVAHI_BROWSER_ALL_FOR_NOW:
            g_print("Browser all-for-now\n");
            priv->avahi_done = TRUE; 	
            if( (priv->ResolveCount == 0) && (priv->avahi_done == TRUE) )
            {
                g_signal_emit(Browser, class->enum_cmplt_id, 0, NULL);        
            }

 	    case AVAHI_BROWSER_CACHE_EXHAUSTED:
 	    break;
    }
} 

/* Callback for state changes on the Client */
static void
g_hnode_browser_avahi_client_callback (AVAHI_GCC_UNUSED AvahiClient *client, AvahiClientState state, void *userdata)
{
   GHNodeBrowserClass   *class;
   GHNodeBrowserPrivate *priv;

   GHNodeBrowser *Browser = userdata;
	
   class = G_HNODE_BROWSER_GET_CLASS (Browser);
   priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

   switch( state )
   {
       case AVAHI_CLIENT_S_RUNNING:  
           /* The server has startup successfully and registered its host
            * name on the network, so it's time to create our services */
           if (!priv->ServiceGroup)
           {
               priv->AvahiClient = client;
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

/**
 * Kicks off managment server discovery
 */
void
g_hnode_browser_start(GHNodeBrowser *Browser)
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

	GHNodeBrowserClass   *class;
	GHNodeBrowserPrivate *priv;
	
	class = G_HNODE_BROWSER_GET_CLASS (Browser);
	priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

 
    /* Optional: Tell avahi to use g_malloc and g_free */
    avahi_set_allocator (avahi_glib_allocator ());
 
    /* Create the GLIB main loop */
    //priv->Loop = g_main_loop_new (NULL, FALSE);

    // Give this service a name
    //priv->ServiceName = avahi_strdup("HnodeManager");
    priv->ServiceGroup = NULL;

    /* Create the GLIB Adaptor */
    glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
    poll_api = avahi_glib_poll_get (glib_poll);

    //g_message("Listen\n");

    // Open up a listener socket source
    //priv->ListenSource = g_hnode_pktsrc_new(PST_HNODE_TCP_HMNODE_LISTEN);    
    //priv->ListenSource = pktsrc_SourceCreate( PST_HNODE_TCP_HMNODE_LISTEN, NULL, g_hnode_server_RxCallback, g_hnode_server_TxCallback, g_hnode_server_StateCallback );
    //g_signal_connect(priv->ListenSource, "new_source", G_CALLBACK(g_hnode_server_newsrc), Server);
    //g_hnode_pktsrc_start( priv->ListenSource );

    //g_message("Debug\n");

    // Setup some sockets to rx debug and event frames
    //priv->DebugSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    //g_signal_connect(priv->DebugSource, "rx_packet", G_CALLBACK(g_hnode_server_debugrx), Server);
    //g_signal_connect(priv->DebugSource, "tx_packet", g_hnode_server_debugtx, Server);
    //g_hnode_pktsrc_start( priv->DebugSource );

    //g_message("Event\n");

    //priv->EventSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    //g_signal_connect(priv->EventSource, "rx_packet", G_CALLBACK(g_hnode_server_eventrx), Server);
    //g_signal_connect(priv->EventSource, "tx_packet", g_hnode_server_eventtx, Server);
    //g_hnode_pktsrc_start( priv->EventSource );

    //g_message("HNode\n");

    // Create the socket that will be used for interacting with hnodes
    //priv->HNodeSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    //g_signal_connect(priv->HNodeSource, "rx_packet", G_CALLBACK(g_hnode_server_hnoderx), Server);
    //g_signal_connect(priv->HNodeSource, "tx_packet", G_CALLBACK(g_hnode_server_hnodetx), Server);
    //g_hnode_pktsrc_start( priv->HNodeSource );

//    priv->DebugSource = pktsrc_SourceCreate( PST_HNODE_UDP_ASYNCH, NULL, g_hnode_server_RxCallback, g_hnode_server_TxCallback, g_hnode_server_StateCallback );
//    priv->EventSource = pktsrc_SourceCreate( PST_HNODE_UDP_ASYNCH, NULL, g_hnode_server_RxCallback, g_hnode_server_TxCallback, g_hnode_server_StateCallback );

    //AddrObj = g_hnode_pktsrc_get_address_object(priv->ListenSource);
    //g_message("Inbound Ports -- HMPort: %d\n", g_hnode_address_GetPort(AddrObj)); 

    //AddrObj = g_hnode_pktsrc_get_address_object(priv->DebugSource);
    //g_message("Inbound Ports -- Debug: %d\n", g_hnode_address_GetPort(AddrObj)); 

    //AddrObj = g_hnode_pktsrc_get_address_object(priv->EventSource);
    //g_message("Inbound Ports -- Event: %d\n", g_hnode_address_GetPort(AddrObj)); 

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
                               g_hnode_browser_avahi_client_callback,  /* Callback function for Client state changes */
                               Browser,                         /* User data */
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
                                          "_hmnode._tcp", 
                                          NULL, 0, g_hnode_browser_browse_callback, Browser))) 
     {
 	     g_error("Failed to create service browser: %s\n", avahi_strerror(avahi_client_errno(priv->AvahiClient)));
 	     goto fail;
     }

     return;

     fail:
     /* Clean up */
     avahi_client_free (priv->AvahiClient);
     avahi_glib_poll_free (glib_poll);

}

gboolean
g_hnode_browser_set_debug( GHNodeBrowser *sb )
{
/*
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
*/                                            
}

gboolean
g_hnode_browser_get_provider_info( GHNodeBrowser *sb, 
                                   guint32 ProviderObjID,
                                   GHNodeProvider *Provider )
{
    GList *Element;
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;

    //GHNodeProviderPrivate *PRecord;
	
	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
    
    // Find the referenced Provider.
    //Element = g_list_first(priv->Providers);
    //while(Element)
    //{
    //    PRecord = (GHNodeProviderPrivate *)Element->data; 
    //    if( PRecord->ObjID == ProviderObjID )
    //        break;
            
    //    Element = g_list_next(Element);
    //}
     
    // Failed to find a Provider with ObjID. 
    //if( Element == NULL )    
    //    return TRUE;

    //Provider->PortNumber     = PRecord->PortNumber;
    //Provider->MicroVersion   = PRecord->MicroVersion;
    //Provider->MinorVersion   = PRecord->MinorVersion;
    //Provider->MajorVersion   = PRecord->MajorVersion;
    //Provider->EndPointCount  = PRecord->EndPointCount;
    //Provider->HostAddrType   = PRecord->HostAddrType;
    //Provider->HostAddrLength = PRecord->HostAddrLength;

    //memcpy(Provider->HostAddr, PRecord->HostAddr, PRecord->HostAddrLength+1); 

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

    //GHNodeProviderPrivate *PRecord;    
    //GHNodeEndPointPrivate *EPRecord;

	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
 /*   
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
*/
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

    //GHNodeProviderPrivate *PRecord;    
    //GHNodeEndPointPrivate *EPRecord;

	class = G_HNODE_BROWSER_GET_CLASS(sb);
	priv  = G_HNODE_BROWSER_GET_PRIVATE(sb);   
/*    
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
*/    
    return FALSE;
}

void
g_hnode_browser_add_device_filter(GHNodeBrowser *sb, gchar **RequiredDeviceMimeStrs)
{
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
	//GHNodeEPTypeProfilePrivate *Profile;
    gchar    *tmpStr;
    guint32  i;
    
	class = G_HNODE_BROWSER_GET_CLASS (sb);
	priv = G_HNODE_BROWSER_GET_PRIVATE (sb);

    // Create a profile record to remeber this stuff
//    Profile = g_new(GHNodeEPTypeProfilePrivate, 1);
    
//    Profile->TypeStrs = g_ptr_array_new();
//    Profile->TypeStrStorage = g_string_chunk_new(64);
    
    // Add the filter strings
//    for(i = 0; RequiredDeviceMimeStrs[i]; i++)
//    {
//        tmpStr = g_string_chunk_insert_const(Profile->TypeStrStorage, RequiredDeviceMimeStrs[i]);
//        g_ptr_array_add(Profile->TypeStrs, (gpointer) tmpStr);
//    }
    
    // Add to list of filters.
//    priv->ProfileFilter = g_list_append(priv->ProfileFilter, Profile);

}

void
g_hnode_browser_reset_device_filter(GHNodeBrowser *sb)
{
	GHNodeBrowserClass *class;
	GHNodeBrowserPrivate *priv;
//	GHNodeEPTypeProfilePrivate *Profile;
	GList *Element;
    
	class = G_HNODE_BROWSER_GET_CLASS (sb);
	priv = G_HNODE_BROWSER_GET_PRIVATE (sb);

    // Pull all filter elements and get rid of them
    Element = g_list_first(priv->ProfileFilter);
//    while(Profile)
//    {                 
//        Profile = (GHNodeEPTypeProfilePrivate *)Element->data;
//        g_ptr_array_free(Profile->TypeStrs, TRUE);
//        g_string_chunk_free(Profile->TypeStrStorage);
//        Element = g_list_next(Element);
//    }
        
    // Free up the main list
    g_list_free(priv->ProfileFilter);
    priv->ProfileFilter = NULL;
}

GHNodeBrowserMPoint *
g_hnode_browser_first_mpoint(GHNodeBrowser *Browser)
{
	GHNodeBrowserPrivate *priv;
    
	priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

    priv->CurMPoint = g_list_first(priv->MPointList);

    if( priv->CurMPoint )
        return priv->CurMPoint->data;
    return NULL; 
}

GHNodeBrowserMPoint *
g_hnode_browser_next_mpoint(GHNodeBrowser *Browser)
{
	GHNodeBrowserPrivate *priv;
    
	priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

    if( priv->CurMPoint == NULL )
        return NULL;
    
    priv->CurMPoint = g_list_next(priv->CurMPoint);

    if( priv->CurMPoint )
        return priv->CurMPoint->data;
    return NULL; 
}

GHNodeBrowserHNode *
g_hnode_browser_first_hnode(GHNodeBrowser *Browser)
{
	GHNodeBrowserPrivate *priv;
    
	priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

    priv->CurHNode = g_list_first(priv->HNodeList);

    if( priv->CurHNode )
        return priv->CurHNode->data;
    return NULL; 
}

GHNodeBrowserHNode *
g_hnode_browser_next_hnode(GHNodeBrowser *Browser)
{
	GHNodeBrowserPrivate *priv;
    
	priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

    if( priv->CurHNode == NULL )
        return NULL;
    
    priv->CurHNode = g_list_next(priv->CurHNode);

    if( priv->CurHNode )
        return priv->CurHNode->data;
    return NULL; 
}

GHNodeBrowserHNode *
g_hnode_browser_find_hnode_by_uid(GHNodeBrowser *Browser, GHNodeUID *TargetUID)
{	
    GList                *CurElem;
    GHNodeBrowserHNode   *Node;
	GHNodeBrowserPrivate *priv;
    
	priv = G_HNODE_BROWSER_GET_PRIVATE (Browser);

    // Cycle through the nodes starting at the first.
    CurElem = g_list_first(priv->HNodeList);
    while(CurElem)
    {
        // Get the node pointer
        Node = CurElem->data;

        // Check UID equality
        if( g_hnode_browser_hnode_is_uid_equal( Node, TargetUID ) )
        {
            return Node;
        }

        // See if there are additional Node to process
        CurElem = g_list_next(CurElem);
    }

    // Failed to find the node.
    return NULL;     
}



