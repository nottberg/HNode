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
//#include "hnode-provider.h"
#include "hnode-nodeobj.h"
#include "hnode-cfginf.h"

#define G_HNODE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE, GHNodePrivate))

G_DEFINE_TYPE (GHNode, g_hnode, G_TYPE_OBJECT);

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

typedef struct _GHNodePrivate GHNodePrivate;
struct _GHNodePrivate
{
    //GHNodeServerSource *Source;
   
    AvahiClient     *AvahiClient;
    AvahiEntryGroup *ServiceGroup;
    char            *ServiceName;
    char            *ServicePrefix;

    GHNodePktSrc    *HNodeSource;
    GHNodePktSrc    *ConfigSource;

 	guint16   MicroVersion;
 	guint8    MinorVersion;
 	guint8    MajorVersion;
	guint8    NodeUID[16];
 	guint32   EndPointCount;
 	guint16   MgmtPort;

    gboolean  ConfigEnable;
 	guint16   ConfigPort;
 	
 	guint32   HostAddrType;
 	guint32   HostAddrLength;
 	guint8    HostAddr[128];
            
    GArray   *EndPointArray;
 
    GHNodeAddress  *DebugAddrObj;
    guint16         DebugPktSeqCnt;

    GHNodeAddress  *EventAddrObj;
    guint16         EventPktSeqCnt;

    gboolean        ServerValid;
    guint32         ServerObjID;

    gboolean        dispose_has_run;

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

static guint g_hnode_signals[LAST_SIGNAL] = { 0 };

enum HNodeAddressType
{
    HNADDR_NONE   =  0,
    HNADDR_IPV4   =  1,
	HNADDR_IPV6   =  2,
};

enum HNodePacketType
{
    HNPKT_NONE          =  0,
    HNPKT_PINFO_REQ     =  1,
	HNPKT_PINFO_RPY     =  2,
    HNPKT_EPINFO_REQ    =  3,
    HNPKT_EPINFO_RPY    =  4,
    HNPKT_SYSINFO_REQ   =  5,
    HNPKT_SYSINFO_ACK   =  6,        
};

/* GObject callbacks */
static void g_hnode_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);

extern void g_cclosure_user_marshal_VOID__UINT_UINT_UINT_UINT_POINTER (GClosure     *closure,
                                                                       GValue       *return_value,
                                                                       guint         n_param_values,
                                                                       const GValue *param_values,
                                                                       gpointer      invocation_hint,
                                                                       gpointer      marshal_data);

static GObjectClass *parent_class = NULL;

static void
g_hnode_dispose (GObject *obj)
{
    GHNode        *self = (GHNode *)obj;
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE(self);

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
g_hnode_finalize (GObject *obj)
{
    GHNode *self = (GHNode *)obj;

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_class_init (GHNodeClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_dispose;
    o_class->finalize = g_hnode_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	class->state_change_id = g_signal_new (
			"state_change",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeClass, state_change),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->config_rx_id = g_signal_new (
			"config-rx",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeClass, config_rx),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

    parent_class = g_type_class_peek_parent (class);      
	g_type_class_add_private (o_class, sizeof (GHNodePrivate));
}



static void
g_hnode_init (GHNode *sb)
{
	GHNodePrivate *priv;
    //GHNodeServerSource  *NodeSource;

	priv = G_HNODE_GET_PRIVATE (sb);
        
    // Initilize the ObjID value
    priv->ServiceName   = NULL;
    priv->ServicePrefix = NULL;

    priv->ServiceGroup = NULL;
    priv->AvahiClient  = NULL;
 
    priv->EndPointArray = NULL;

 	priv->MicroVersion  = 0;
 	priv->MinorVersion  = 0;
 	priv->MajorVersion  = 1;

	memset(priv->NodeUID, 0, 16);

 	priv->EndPointCount = 0;
 	priv->MgmtPort      = 0;
 	
    priv->ConfigEnable  = FALSE;
 	priv->ConfigPort    = 0;

 	priv->HostAddrType   = 0;
 	priv->HostAddrLength = 0;
 	priv->HostAddr[0]    = '\0';

    priv->DebugAddrObj   = NULL;
    priv->DebugPktSeqCnt = 0;
    priv->EventAddrObj   = NULL;
    priv->ServerValid    = FALSE;

    priv->ServerObjID    = 0;

    priv->dispose_has_run = FALSE;
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

static void g_hnode_create_service( GHNode *HNode );

static void 
g_hnode_entry_group_callback(AvahiEntryGroup *g,
                     AvahiEntryGroupState state,
                     void* userdata) 
{
    GHNodeClass   *class;
    GHNodePrivate *priv;
    GHNodeEvent    hbevent;

    GHNode *HNode = userdata;
	
    class = G_HNODE_GET_CLASS (HNode);
    priv = G_HNODE_GET_PRIVATE (HNode);

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
             
            g_warning("Service name collision, renaming service to '%s'\n", priv->ServiceName);
             
            /* And recreate the services */
            g_hnode_create_service(HNode);
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
g_hnode_create_service( GHNode *HNode ) 
{
    char r[128];
    int ret;
    GHNodeAddress *AddrObj;    
    GHNodeClass   *class;
    GHNodePrivate *priv;
    GHNodeEvent    hbevent;
	
    class = G_HNODE_GET_CLASS (HNode);
    priv = G_HNODE_GET_PRIVATE (HNode);

    /* If this is the first time we're called, let's create a new entry group */
    if (!priv->ServiceGroup)
    {
        priv->ServiceGroup = avahi_entry_group_new(priv->AvahiClient, g_hnode_entry_group_callback, HNode);
        if(!priv->ServiceGroup)        
        {
            g_error("avahi_entry_group_new() failed: %s\n", avahi_strerror(avahi_client_errno(priv->AvahiClient)));
            goto fail;
        }
    }
          
    /* Create some random TXT data */
    g_snprintf(r, sizeof(r), "random=%i", rand());

    AddrObj = g_hnode_pktsrc_get_address_object(priv->HNodeSource);

    /* Add the service for _hmnode._tcp */
    if ((ret = avahi_entry_group_add_service(priv->ServiceGroup, 
                                             AVAHI_IF_UNSPEC, 
                                             AVAHI_PROTO_INET, 
                                             0, priv->ServiceName, 
                                             "_hnode._udp", 
                                             NULL, NULL, 
                                             g_hnode_address_GetPort(AddrObj),
                                             "test=blah", r, NULL)) < 0) 
    {
        g_error("Failed to add _hnode._udp service: %s\n", avahi_strerror(ret));
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

/* Callback for state changes on the Client */
static void
g_hnode_avahi_client_callback (AVAHI_GCC_UNUSED AvahiClient *client, AvahiClientState state, void *userdata)
{
   GHNodeClass   *class;
   GHNodePrivate *priv;
   GHNodeEvent    hbevent;

   GHNode *HNode = userdata;
	
   class = G_HNODE_GET_CLASS (HNode);
   priv = G_HNODE_GET_PRIVATE (HNode);

   switch( state )
   {
       case AVAHI_CLIENT_S_RUNNING:  
           /* The server has startup successfully and registered its host
            * name on the network, so it's time to create our services */
           if (!priv->ServiceGroup)
           {
               priv->AvahiClient = client;
               g_hnode_create_service( HNode );
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

static void
g_hnode_set_base_name(GHNode  *HNode)
{
    GHNodePrivate *priv;
    gchar TmpNameBuf[512];

	priv = G_HNODE_GET_PRIVATE (HNode);

    // Check if a previous string needs to be freed.
    if( priv->ServiceName )
    {
        avahi_free(priv->ServiceName);
        priv->ServiceName = NULL;
    }    

    // Create a unique service name based on user prefix, interface, uid.
    if( priv->ServicePrefix )
    {
        g_sprintf(TmpNameBuf, "%s_eth_%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", priv->ServicePrefix, 
                          priv->NodeUID[0], priv->NodeUID[1], priv->NodeUID[2], priv->NodeUID[3], priv->NodeUID[4], priv->NodeUID[5], priv->NodeUID[6], priv->NodeUID[7]);
    }
    else
    {
        g_sprintf(TmpNameBuf, "eth_%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x%2.2x", priv->NodeUID[0], priv->NodeUID[1], priv->NodeUID[2], priv->NodeUID[3], priv->NodeUID[4], priv->NodeUID[5], priv->NodeUID[6], priv->NodeUID[7]);
    }

    // Create a unique base name.
    priv->ServiceName = avahi_strdup(TmpNameBuf);

}

void 
g_hnode_server_hnoderx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    guint DataLength;
    guint PktType;
    guint ReqTag;

    GHNode        *HNode = (GHNode *) data;
	GHNodeClass   *class;
	GHNodePrivate *priv;

    GHNodePacket *TxPacket;

	class = G_HNODE_GET_CLASS (HNode);
	priv = G_HNODE_GET_PRIVATE (HNode);

    DataLength = g_hnode_packet_get_data_length(Packet);
    PktType    = g_hnode_packet_get_uint(Packet);
    ReqTag     = g_hnode_packet_get_uint(Packet);

	switch( PktType )
    {
        // Request for general info about this hnode.
        case HNPKT_PINFO_REQ:
            printf("Provider Info Request -- Tag: 0x%x\n", ReqTag);

            TxPacket = g_hnode_packet_new();						
    
            // Set the TX address
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Check to see if this is the packet we are interested in.
            g_hnode_packet_set_uint(TxPacket, HNPKT_PINFO_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

    		g_hnode_packet_set_short(TxPacket, priv->MicroVersion); // Micro Version
    		g_hnode_packet_set_char(TxPacket, priv->MinorVersion);  // Minor Version
    		g_hnode_packet_set_char(TxPacket, priv->MajorVersion);  // Major Version

    		// Get the unique ID for the node.
            g_hnode_packet_set_bytes(TxPacket, priv->NodeUID, 16);   // NodeUID

        	g_hnode_packet_set_uint(TxPacket, priv->EndPointCount);  // End Point Count
         	g_hnode_packet_set_short(TxPacket, priv->MgmtPort);      // Mgmt Port
						
            g_hnode_packet_set_short(TxPacket, priv->ConfigPort);   // Configuration Port - 0 == configuration not supportted

    		// Save away the addressing information.
        	g_hnode_packet_set_uint(TxPacket, priv->HostAddrType);  // Address Type
    		g_hnode_packet_set_uint(TxPacket, priv->HostAddrLength);   // Address Length
            
            g_hnode_packet_set_bytes(TxPacket, priv->HostAddr, 128);  // Address string

			// Send a request for info about the provider.
            g_hnode_pktsrc_send_packet(priv->HNodeSource, TxPacket);
		break;

        case HNPKT_EPINFO_REQ:
        { 
            guint32 ReqEPIndex;
            GHNodeEndPoint *EndPoint;

            // The third word in the packet is the requested endpoint.
            ReqEPIndex = g_hnode_packet_get_uint(Packet);

            // Get a pointer to the endpoint index.
            EndPoint = &g_array_index(priv->EndPointArray, GHNodeEndPoint, ReqEPIndex);

            // Set-up a TX Packet for the reply
            TxPacket = g_hnode_packet_new();						  
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Check to see if this is the packet we are interested in.
            g_hnode_packet_set_uint(TxPacket, HNPKT_EPINFO_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);
            g_hnode_packet_set_uint(TxPacket, ReqEPIndex);


         	g_hnode_packet_set_short(TxPacket, EndPoint->Port);   // Mgmt Port
            g_hnode_packet_skip_bytes(TxPacket, 2);      // Reserved Bytes

    		g_hnode_packet_set_short(TxPacket, EndPoint->MicroVersion); // Micro Version
    		g_hnode_packet_set_char(TxPacket, EndPoint->MinorVersion);  // Minor Version
    		g_hnode_packet_set_char(TxPacket, EndPoint->MajorVersion);  // Major Version

    		// Get the unique ID for the node.
        	g_hnode_packet_set_uint(TxPacket, EndPoint->AssociateEPIndex);  // AssociatedEP
						

    		// Save away the addressing information.
        	g_hnode_packet_set_uint(TxPacket, EndPoint->MimeTypeLength);  // MimeStrLength            
            g_hnode_packet_set_bytes(TxPacket, EndPoint->MimeTypeStr, 128);  // MimeTypeStr

			// Send a request for info about the provider.
            g_hnode_pktsrc_send_packet(priv->HNodeSource, TxPacket);
        }
        break;

        case HNPKT_SYSINFO_REQ: 
        {
            guint8   TmpTime;
            guint16  DebugPort;
            guint16  EventPort;
            gchar   *AddrStr;
            guint32  AddrStrLen;
            GHNodeAddress *PktAddrObj;

            PktAddrObj = g_hnode_packet_get_address_object(Packet);

            // Duplicate the node address.
            if( priv->DebugAddrObj == NULL )
            {
                priv->DebugAddrObj = g_hnode_address_new();
            }

            // Duplicate the node address.
            if( priv->EventAddrObj == NULL )
            {
                priv->EventAddrObj = g_hnode_address_new();
            }

            g_hnode_address_get_str_ptr(PktAddrObj, &AddrStr, &AddrStrLen);
            g_hnode_address_set_str(priv->DebugAddrObj, AddrStr );

            g_hnode_address_get_str_ptr(PktAddrObj, &AddrStr, &AddrStrLen);
            g_hnode_address_set_str(priv->EventAddrObj, AddrStr );

            // Pull out the time values
            TmpTime = g_hnode_packet_get_char(Packet);
            TmpTime = g_hnode_packet_get_char(Packet);
            TmpTime = g_hnode_packet_get_char(Packet);
            TmpTime = g_hnode_packet_get_char(Packet);
         
            // Get the Debug Port
            DebugPort = g_hnode_packet_get_short(Packet);
            g_hnode_address_set_port(priv->DebugAddrObj, DebugPort);

            EventPort = g_hnode_packet_get_short(Packet);
            g_hnode_address_set_port(priv->EventAddrObj, EventPort);

            // Get our assigned objectID which will be used when sending
            // debug and event data to the management server.
            priv->ServerObjID = g_hnode_packet_get_uint(Packet);

            // The server information is now valid
            priv->ServerValid = TRUE;

            // Set-up a TX Packet for the reply
            TxPacket = g_hnode_packet_new();						  
            g_hnode_packet_clone_address(TxPacket, Packet);

            g_hnode_packet_set_uint(TxPacket, HNPKT_SYSINFO_ACK);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

			// Send a request for info about the provider.
            g_hnode_pktsrc_send_packet(priv->HNodeSource, TxPacket);
        }
        break;
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


void 
g_hnode_config_rx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{

    GHNode        *HNode = (GHNode *) data;
	GHNodeClass   *class;
	GHNodePrivate *priv;

	class = G_HNODE_GET_CLASS (HNode);
	priv = G_HNODE_GET_PRIVATE (HNode);

    // Tell the upper layer about the new node record.
    g_signal_emit(HNode, class->config_rx_id, 0, Packet);

    // Free the RX frame
    g_object_unref(Packet);

	return;
}
 
void 
g_hnode_config_tx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    // Free the TX frame
    g_object_unref(Packet);

	return;
}



  
GHNode*
g_hnode_new (void)
{
	return g_object_new (G_TYPE_HNODE, NULL);
}

void
g_hnode_set_name_prefix(GHNode  *HNode, guint8  *NamePrefix)
{
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE (HNode);

    // Check if a previous string needs to be freed.
    if( priv->ServicePrefix )
    {
        avahi_free(priv->ServicePrefix);
        priv->ServicePrefix = NULL;
    }    

    // Create a unique base name.
    priv->ServicePrefix = avahi_strdup(NamePrefix);

    // Rebuild the service name based on this new info.
    g_hnode_set_base_name(HNode);
}

void
g_hnode_set_version(GHNode *HNode, guint8 MajorVersion, guint8 MinorVersion, guint16 MicroVersion )
{
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE (HNode);

    priv->MajorVersion = MajorVersion;
    priv->MinorVersion = MinorVersion;
    priv->MicroVersion = MicroVersion;

}

void
g_hnode_set_uid(GHNode *HNode, guint8 *UIDBuffer)
{
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE (HNode);

    memcpy(priv->NodeUID, UIDBuffer, sizeof(priv->NodeUID));

    // Rebuild the service name based on this new info.
    g_hnode_set_base_name(HNode);

}

void
g_hnode_enable_config_support(GHNode *HNode)
{
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE (HNode);

    priv->ConfigEnable = TRUE;

}

void
g_hnode_set_endpoint_count(GHNode *HNode, guint16 EndPointCount)
{
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE (HNode);

    // Store the number of endpoints
    priv->EndPointCount = EndPointCount;

    // Allocate an array for the endpoint data.
    priv->EndPointArray = g_array_sized_new(FALSE, TRUE, sizeof(GHNodeEndPoint), EndPointCount);

}

void
g_hnode_set_endpoint(GHNode  *HNode, 
                     guint16  EndPointIndex,
                     guint16  AssociateEPIndex,
	                 guint8  *MimeTypeStr,	
	                 guint16  Port,
	                 guint8   MajorVersion,
	                 guint8   MinorVersion,
	                 guint16  MicroVersion)
{

    GHNodePrivate  *priv;
    GHNodeEndPoint *EndPoint;

	priv = G_HNODE_GET_PRIVATE (HNode);

    // Get a pointer to the endpoint index.
    EndPoint = &g_array_index(priv->EndPointArray, GHNodeEndPoint, EndPointIndex);

    EndPoint->MimeTypeLength = g_strlcpy(EndPoint->MimeTypeStr, MimeTypeStr, sizeof(EndPoint->MimeTypeStr));

    EndPoint->Port              = Port;
    EndPoint->AssociateEPIndex  = AssociateEPIndex;

    EndPoint->MajorVersion      = MajorVersion;
    EndPoint->MinorVersion      = MinorVersion;
    EndPoint->MicroVersion      = MicroVersion;
}

/**
 * Kicks off managment server discovery
 */
void
g_hnode_start(GHNode *HNode)
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

    GHNodeAddress *AddrObj;    

	GHNodeClass   *class;
	GHNodePrivate *priv;
	GHNodeEvent    hbevent;
	
	class = G_HNODE_GET_CLASS (HNode);
	priv = G_HNODE_GET_PRIVATE (HNode);

 
    /* Optional: Tell avahi to use g_malloc and g_free */
    avahi_set_allocator (avahi_glib_allocator ());
 
    /* Create the GLIB main loop */
    //priv->Loop = g_main_loop_new (NULL, FALSE);

    // Give this service a name
    priv->ServiceGroup = NULL;

    /* Create the GLIB Adaptor */
    glib_poll = avahi_glib_poll_new (NULL, G_PRIORITY_DEFAULT);
    poll_api = avahi_glib_poll_get (glib_poll);

    // Create the socket that will be used for interacting with hnodes
    priv->HNodeSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
    g_signal_connect(priv->HNodeSource, "rx_packet", G_CALLBACK(g_hnode_server_hnoderx), HNode);
    g_signal_connect(priv->HNodeSource, "tx_packet", G_CALLBACK(g_hnode_server_hnodetx), HNode);
    g_hnode_pktsrc_start( priv->HNodeSource );

    AddrObj = g_hnode_pktsrc_get_address_object(priv->HNodeSource);
    priv->MgmtPort = g_hnode_address_GetPort(AddrObj);

    if( priv->ConfigEnable )
    {
        priv->ConfigSource = g_hnode_pktsrc_new(PST_HNODE_UDP_ASYNCH);
        g_signal_connect(priv->ConfigSource, "rx_packet", G_CALLBACK(g_hnode_config_rx), HNode);
        g_signal_connect(priv->ConfigSource, "tx_packet", G_CALLBACK(g_hnode_config_tx), HNode);
        g_hnode_pktsrc_start( priv->ConfigSource );

        AddrObj = g_hnode_pktsrc_get_address_object(priv->ConfigSource);
        priv->ConfigPort = g_hnode_address_GetPort(AddrObj); 
    }

    // Make sure the ServiceName is up to date with the latest settings.
    g_hnode_set_base_name(HNode);

    /* Create a new AvahiClient instance */
    priv->AvahiClient = avahi_client_new (poll_api,            /* AvahiPoll object from above */
                               0,
                               g_hnode_avahi_client_callback,  /* Callback function for Client state changes */
                               HNode,                         /* User data */
                               &error);                          /* Error return */
 
     /* Check the error return code */
     if (priv->AvahiClient == NULL)
     {
         /* Print out the error string */
         g_error ("Error initializing Avahi: %s", avahi_strerror (error));
 
         goto fail;
     }
    
     /* Make a call to get the version string from the daemon */
     version = avahi_client_get_version_string (priv->AvahiClient);
 
     /* Check if the call suceeded */
     if (version == NULL)
     {
         g_error ("Error getting version string: %s", avahi_strerror (avahi_client_errno (priv->AvahiClient)));
 
         goto fail;
     }
         
     return;

     fail:
     /* Clean up */
     avahi_client_free (priv->AvahiClient);
     avahi_glib_poll_free (glib_poll);

}

/**
 * Sends a debug frame to the management server
 */
void
g_hnode_send_debug_frame(GHNode *HNode, guint32 Type, guint32 EPIndex, guint32 DebugDataLength, guint8 *DebugDataPtr)
{
    GHNodePrivate  *priv;
    GHNodePacket   *TxPacket;

	priv = G_HNODE_GET_PRIVATE (HNode);

    if( priv->ServerValid == FALSE )
        return;

    TxPacket = g_hnode_packet_new();						
    
    // Set the TX address
    g_hnode_packet_assign_addrobj(TxPacket, priv->DebugAddrObj);
    //g_hnode_packet_clone_address(TxPacket, priv->DebugAddrObj);

    // Indicate this is a debug frame.
    g_hnode_packet_set_short(TxPacket, Type); 

    // Fill in an incrementing sequence count that can be used to identify
    // dropped frames, etc.
    g_hnode_packet_set_short(TxPacket, priv->DebugPktSeqCnt);
    priv->DebugPktSeqCnt += 1;

    // Fill in the Server Object ID, so the server knows who sent this frame.
    g_hnode_packet_set_uint(TxPacket, priv->ServerObjID);

    // Indicate the endpoint which generated this debug packet.
    g_hnode_packet_set_short(TxPacket, EPIndex);

    // Fill in a data length paremeter and then copy the data into the packet.
    g_hnode_packet_set_short(TxPacket, DebugDataLength);
    g_hnode_packet_set_bytes(TxPacket, DebugDataPtr, DebugDataLength);

	// Send the packet to the management server.
    g_hnode_pktsrc_send_packet(priv->HNodeSource, TxPacket);

}

/**
 * Sends an event frame to the management server
 */
void
g_hnode_send_event_frame(GHNode *HNode, guint32 EventID, guint32 EPIndex, guint32 EventParamLength, guint8 *EventParamDataPtr)
{
    GHNodePrivate  *priv;
    GHNodePacket   *TxPacket;

	priv = G_HNODE_GET_PRIVATE (HNode);

    if( priv->ServerValid == FALSE )
        return;

    TxPacket = g_hnode_packet_new();						
    
    // Set the TX address
    g_hnode_packet_assign_addrobj(TxPacket, priv->EventAddrObj);

    // Indicate this is a debug frame.
    g_hnode_packet_set_short(TxPacket, EventID); 

    // Fill in an incrementing sequence count that can be used to identify
    // dropped frames, etc.
    g_hnode_packet_set_short(TxPacket, priv->EventPktSeqCnt);
    priv->EventPktSeqCnt += 1;

    // Fill in the Server Object ID, so the server knows who sent this frame.
    g_hnode_packet_set_uint(TxPacket, priv->ServerObjID);

    // Indicate the endpoint which generated this debug packet.
    g_hnode_packet_set_short(TxPacket, EPIndex);

    // Fill in a data length paremeter and then copy the data into the packet.
    g_hnode_packet_set_short(TxPacket, EventParamLength);
    g_hnode_packet_set_bytes(TxPacket, EventParamDataPtr, EventParamLength);

	// Send the packet to the management server.
    g_hnode_pktsrc_send_packet(priv->HNodeSource, TxPacket);

}

void 
g_hnode_send_config_frame(GHNode  *HNode, GHNodePacket *TxPacket)
{
    GHNodePrivate *priv;

	priv = G_HNODE_GET_PRIVATE (HNode);

    g_hnode_pktsrc_send_packet(priv->ConfigSource, TxPacket);

	return;
}

#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer

void
g_cclosure_user_marshal_VOID__UINT_UINT_UINT_UINT_POINTER (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_UINT_UINT_UINT_POINTER) (gpointer     data1,
                                                                  guint        arg_1,
                                                                  guint        arg_2,
                                                                  guint        arg_3,
                                                                  guint        arg_4,
                                                                  gpointer     arg_5,
                                                                  gpointer     data2);
  register GMarshalFunc_VOID__UINT_UINT_UINT_UINT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_UINT_UINT_UINT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            g_marshal_value_peek_uint (param_values + 4),
            g_marshal_value_peek_pointer (param_values + 5),
            data2);
}



