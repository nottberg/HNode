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

//#include <slp.h>

//#include "hmpacket.h"
#include "hnode-pktsrc.h"
#include "hnode-provider.h"
#include "hnode-srvobj.h"

enum HNnodeNodeState
{
    HNODE_NODE_STATE_NEW,
    HNODE_NODE_STATE_EXPOSING,
    HNODE_NODE_STATE_ESTABLISHED,
    HNODE_NODE_FILTERED
};

typedef struct _GHNodeClientNode
{
    guint32 NodeID;

    guint32 State;    
    
    GHNodeProvider *Node;
}GHNodeClientNode;

enum HNodeClientEnableFlagEnum
{
    HNODE_ENABLE_REPORT_NODES = 0x00000001,
    HNODE_ENABLE_DEBUG        = 0x00000002,
    HNODE_ENABLE_EVENT        = 0x00000004,
};

enum HNodeClientStateEnum
{
    HNODE_CLIENT_STATE_STOPPED,
    HNODE_CLIENT_STATE_NO_CONNECT,
    HNODE_CLIENT_STATE_IDLE,
    HNODE_CLIENT_STATE_UPDATING,
};

#define G_HNODE_CLIENT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_CLIENT, GHNodeClientPrivate))

G_DEFINE_TYPE (GHNodeClient, g_hnode_client, G_TYPE_OBJECT);

typedef struct _GHNodeClientPrivate GHNodeClientPrivate;
struct _GHNodeClientPrivate
{
    // The link to the client.
    GHNodePktSrc *ClientSource;  

    guint32       State;
    guint32       EnableFlags;

    guint32       TotalNodeCount;
    guint32       FilteredNodeCount;
    guint32       UpdateNodeCount;

    guint32       NextNodeID;
    GList        *NodeList;
    
    GHNodeClientNode *UpdatingNode;

    guint32       NextIOTag;

    gboolean                   dispose_has_run;

};

/* GObject callbacks */
static void g_hnode_client_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_client_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);

void g_hnode_client_build_update_packet(GHNodeClient *sb);

static GObjectClass *parent_class = NULL;

static void
g_hnode_client_dispose (GObject *obj)
{
    GList               *CurElem;
    GHNodeClientNode    *ClientNode;
    GHNodeClient        *self = (GHNodeClient *)obj;
    GHNodeClientPrivate *priv;

	priv = G_HNODE_CLIENT_GET_PRIVATE(self);

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

    // Free the client packet source
    g_object_unref(priv->ClientSource);

    // Scan the list looking for a client node that needs to be updated.
    CurElem = g_list_first(priv->NodeList);
    while( CurElem )
    {
       ClientNode = CurElem->data;
   
       // Un reference the provider record.
       g_object_ref(ClientNode->Node);

       // Remove this element.
       priv->NodeList = g_list_remove(priv->NodeList, ClientNode);

       // Free the node data structure 
       g_free(ClientNode);

       // Check the next client node
       CurElem = g_list_first(priv->NodeList);
    }


    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
g_hnode_client_finalize (GObject *obj)
{
    GHNodeClient *self = (GHNodeClient *)obj;

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_client_class_init (GHNodeClientClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_client_dispose;
    o_class->finalize = g_hnode_client_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	class->state_id = g_signal_new (
			"state_change",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeClientClass, state_change),
			NULL, NULL,
			g_cclosure_marshal_VOID__UINT,
			G_TYPE_NONE,
			1, G_TYPE_UINT);

	class->txreq_id = g_signal_new (
			"tx_request",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeClientClass, tx_request),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
            NULL);

    parent_class = g_type_class_peek_parent (class);
	g_type_class_add_private (o_class, sizeof (GHNodeClientPrivate));
}

static void
g_hnode_client_init (GHNodeClient *sb)
{
	GHNodeClientPrivate *priv;
    //GHNodeServerSource  *NodeSource;

	priv = G_HNODE_CLIENT_GET_PRIVATE (sb);
          
    // Initilize the ObjID value
    priv->ClientSource   = NULL;
    
    priv->State       = HNODE_CLIENT_STATE_STOPPED; 
    priv->EnableFlags = 0;  

    priv->TotalNodeCount    = 0;
    priv->FilteredNodeCount = 0;
    priv->UpdateNodeCount   = 0;

    priv->NextNodeID = 0; 
    priv->NodeList   = NULL;  

    priv->NextIOTag  = 0;

    priv->dispose_has_run = FALSE;

}

/*
static void
g_hnode_client_set_property (GObject 	*object,
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
g_hnode_client_get_property (GObject		*object,
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

GHNodeClient *
g_hnode_client_new (void)
{
	return g_object_new (G_TYPE_HNODE_CLIENT, NULL);
}

void 
g_hnode_client_client_rx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    guint32       ReqLength;
    guint32       ReqType;
    guint32       ReqIOTag;

    GHNodePacket *TxPacket;

    GHNodeClient        *Client = data;
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE(Client);

    // Get the header stuff
    ReqLength = g_hnode_packet_get_uint(Packet);
    ReqType   = g_hnode_packet_get_uint(Packet);
    ReqIOTag  = g_hnode_packet_get_uint(Packet);

    switch( ReqType )
    {
		// Shutdown the server.
        case HNODE_MGMT_SHUTDOWN:
//            Context.RunServer = 0;
        break;

		// Enabling the reporting of nodes.
        case HNODE_MGMT_REQ_PINFO:
              // Indicate we should inform the client of node updates
              priv->EnableFlags |= HNODE_ENABLE_REPORT_NODES;

              // Send an acknoledgement packet
              TxPacket = g_hnode_packet_new();

              // Fill in some header information.                
              // Write the request length - 12 bytes
              g_hnode_packet_set_uint(TxPacket, 12);

              // Write the reply type and reqid.
              g_hnode_packet_set_uint(TxPacket, HNODE_MGMT_ACK);
              g_hnode_packet_set_uint(TxPacket, ReqIOTag);

              // Send the packet
              g_hnode_pktsrc_send_packet(priv->ClientSource, TxPacket);  
        break;
        
		// Control the debug information relayed to the client.
        case HNODE_MGMT_DEBUG_CTRL:
              priv->EnableFlags |= HNODE_ENABLE_DEBUG;
        break;

		// Control the device events that will be relayed to the client.
        case HNODE_MGMT_EVENT_CTRL:
              priv->EnableFlags |= HNODE_ENABLE_EVENT;
        break;
        
    }

    // Free the packet.
    g_object_unref(Packet);

/*   
     
		case SHDLR_PROVIDER_INFO_HEADER:
            Hdlr->proto.pinfo.ProviderCount = ProviderGetCount(&Providers);
            
            PktPos = PacketFormatU32(PktPos, HNODE_MGMT_RSP_PINFO);
            PktPos = PacketFormatU32(PktPos, Hdlr->proto.pinfo.ProviderCount);

		    printf("MgmtClientDataOut - PInfo Header: ProviderCount=%d\n",
					 Hdlr->proto.pinfo.ProviderCount);
		
		    printf("Sending %d bytes:\n", (PktPos - Buf));
		    printf("   0x%x 0x%x 0x%x 0x%x\n", Buf[0], Buf[1], Buf[2], Buf[3]);
		    printf("   0x%x 0x%x 0x%x 0x%x\n", Buf[4], Buf[5], Buf[6], Buf[7]);

			send(fd, Buf, (PktPos - Buf), 0);
            
            if( Hdlr->proto.pinfo.ProviderCount )
            {
                // Get a pointer to the starting provider.
                Hdlr->proto.pinfo.CurProvider = ProviderGetFirstProviderInfo(&Providers); 
                
                // Move to the next state to start dumping provider info
                Hdlr->State = SHDLR_PROVIDER_INFO_RECORD;
                return 1;
            }
            
            // Request Completed.
            Hdlr->State = SHDLR_IDLE;
		break;
        
        // Followed by n provider records, where each record has the following format.
        // U16_T PortNumber;
        // U16_T MicroVersion;
        // U8_T  MinorVersion;
        // U8_T  MajorVersion;
        // U32_T EndPointCount;
        // U32_T HostAddrType;
        // U32_T HostAddrLength;
        // U8_T  HostAddr[HostAddrLength];   
		case SHDLR_PROVIDER_INFO_RECORD:
            // Dump the info for the current provider.
            PktPos = PacketFormatU16(PktPos, Hdlr->proto.pinfo.CurProvider->Port);
            PktPos = PacketFormatU16(PktPos, Hdlr->proto.pinfo.CurProvider->MicroVersion);
            PktPos = PacketFormatU8(PktPos, Hdlr->proto.pinfo.CurProvider->MinorVersion);
            PktPos = PacketFormatU8(PktPos, Hdlr->proto.pinfo.CurProvider->MajorVersion);

            PktPos = PacketFormatData(PktPos, Hdlr->proto.pinfo.CurProvider->NodeUID, 16);

            PktPos = PacketFormatU32(PktPos, Hdlr->proto.pinfo.CurProvider->EndPointCount);
            PktPos = PacketFormatU32(PktPos, Hdlr->proto.pinfo.CurProvider->HostAddrType);
            PktPos = PacketFormatU32(PktPos, Hdlr->proto.pinfo.CurProvider->HostAddrLength);
            PktPos = PacketFormatData(PktPos, Hdlr->proto.pinfo.CurProvider->HostAddr, Hdlr->proto.pinfo.CurProvider->HostAddrLength);

            // Put the packet in the socket.
			send(fd, Buf, (PktPos - Buf), 0);
            
            // If this provider has endpoints then dump them next.
            if( Hdlr->proto.pinfo.CurProvider->EndPointCount )
            {
                // Get the first endpoint info
                Hdlr->proto.pinfo.EPCount   = Hdlr->proto.pinfo.CurProvider->EndPointCount;
				Hdlr->proto.pinfo.CurEPIndx = 0;
                
                // Send endpoint info next
                Hdlr->State = SHDLR_PROVIDER_INFO_ENDPOINT;
                return 1;        
            }
            
            // Otherwise check if there are more providers.  Dump them if so.
            if( Hdlr->proto.pinfo.ProviderCount > 1 )
            {
                // Move to the next provider.
                Hdlr->proto.pinfo.ProviderCount -= 1;
                Hdlr->proto.pinfo.CurProvider = ProviderGetNextProviderInfo(&Providers, Hdlr->proto.pinfo.CurProvider);
                
                // Move to the next state to start dumping provider info
                Hdlr->State = SHDLR_PROVIDER_INFO_RECORD;
                
                return 1;                
            }
            
            // Request Handling Complete.
            Hdlr->State = SHDLR_IDLE;
        break;
        
        //
        // Followed by n endpoint records, where each record has the following format.
        // U16_T PortNumber;
        // U16_T MicroVersion;
        // U8_T  MinorVersion;
        // U8_T  MajorVersion;
        // U32_T MimeStrLength;
        // U8_T  MimeTypeStr[MimeStrLength]; 
		case SHDLR_PROVIDER_INFO_ENDPOINT:
			// Get curEP info
            CurEP = ProviderGetEndPoint(Hdlr->proto.pinfo.CurProvider, Hdlr->proto.pinfo.CurEPIndx);

            printf("EndPoint(%d) - Port: %d\n", Hdlr->proto.pinfo.CurEPIndx, CurEP->Port);
            printf("EndPoint(%d) - MicroVersion: %d\n", Hdlr->proto.pinfo.CurEPIndx, CurEP->MicroVersion);
            printf("EndPoint(%d) - MinorVersion: %d\n", Hdlr->proto.pinfo.CurEPIndx, CurEP->MinorVersion);
            printf("EndPoint(%d) - MajorVersion: %d\n", Hdlr->proto.pinfo.CurEPIndx, CurEP->MajorVersion);
            printf("EndPoint(%d) - MimeStrLength: %d\n", Hdlr->proto.pinfo.CurEPIndx, CurEP->MimeTypeLength);
            printf("EndPoint(%d) - MimeStr: %s\n", Hdlr->proto.pinfo.CurEPIndx, CurEP->MimeTypeStr);

            // Dump the info for the current provider.
            PktPos = PacketFormatU16(PktPos, CurEP->Port);
            PktPos = PacketFormatU16(PktPos, CurEP->MicroVersion);
            PktPos = PacketFormatU8(PktPos, CurEP->MinorVersion);
            PktPos = PacketFormatU8(PktPos, CurEP->MajorVersion);
            PktPos = PacketFormatU32(PktPos, CurEP->MimeTypeLength);
            PktPos = PacketFormatData(PktPos, CurEP->MimeTypeStr, CurEP->MimeTypeLength);

            // Put the packet in the socket.
			send(fd, Buf, (PktPos - Buf), 0);
            
            // If this provider has more endpoints then dump them next.
            if( Hdlr->proto.pinfo.EPCount > 1 )
            {
                // Get the first endpoint info
                Hdlr->proto.pinfo.EPCount -= 1;
                Hdlr->proto.pinfo.CurEPIndx += 1;
                
                // Send endpoint info next
                Hdlr->State = SHDLR_PROVIDER_INFO_ENDPOINT;
                return 1;        
            }
            
            // Otherwise check if there are more providers.  Dump them if so.
            if( Hdlr->proto.pinfo.ProviderCount > 1 )
            {
                // Move to the next provider.
                Hdlr->proto.pinfo.ProviderCount -= 1;
                Hdlr->proto.pinfo.CurProvider = ProviderGetNextProviderInfo(&Providers, Hdlr->proto.pinfo.CurProvider);
                
                // Move to the next state to start dumping provider info
                Hdlr->State = SHDLR_PROVIDER_INFO_RECORD;
                
                return 1;                
            }
            
            // Request Handling Complete.
            Hdlr->State = SHDLR_IDLE;
		break;
*/
	return;
}

void 
g_hnode_client_client_tx(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    GHNodeClient        *Client = data;
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE (Client);

    // 
    if( priv->State == HNODE_CLIENT_STATE_UPDATING )
    {
        priv->UpdatingNode->State  = HNODE_NODE_STATE_ESTABLISHED;
        priv->UpdateNodeCount     -= 1;
        priv->State                = HNODE_CLIENT_STATE_IDLE;   
    }

    // Check if we have more node records to report to the client.
    if(  (priv->UpdateNodeCount)   
        && (priv->EnableFlags & HNODE_ENABLE_REPORT_NODES)
        && (priv->State == HNODE_CLIENT_STATE_IDLE))
    {
        g_hnode_client_build_update_packet(Client);
    }

    // Free the packet
    g_object_unref(Packet);


	return;
}

void 
g_hnode_client_state(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    GHNodeClient        *Client = data;
    GHNodeClientPrivate *priv;
    GHNodeClientClass   *class;

    priv  = G_HNODE_CLIENT_GET_PRIVATE (Client);
    class = G_HNODE_CLIENT_GET_CLASS (Client);

    // The client connection has terminated. 
    priv->State = HNODE_CLIENT_STATE_IDLE;   

    //  Tell the server this client is done.
    g_signal_emit(Client, class->state_id, 0, 0);

	return;
}

void 
g_hnode_client_set_client_source(GHNodeClient *sb, GHNodePktSrc *Source)
{
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE (sb);

    // Setup a client record to track the new client.
    priv->ClientSource = Source;

    // Setup some sockets to rx debug and event frames
    g_signal_connect(priv->ClientSource, "rx_packet", G_CALLBACK(g_hnode_client_client_rx), sb);
    g_signal_connect(priv->ClientSource, "tx_packet", G_CALLBACK(g_hnode_client_client_tx), sb);
    g_signal_connect(priv->ClientSource, "state_change", G_CALLBACK(g_hnode_client_state), sb);

	return;
}

void 
g_hnode_client_build_update_packet(GHNodeClient *sb)
{
    GList *CurElem;
    GHNodeClientNode *ClientNode;
    GHNodePacket        *Packet;
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE (sb);

    // Check if new node records need to be sent to the client.
    if( priv->UpdateNodeCount )
    {
        // Scan the list looking for a client node that needs to be updated.
        CurElem = g_list_first(priv->NodeList);
        while( CurElem )
        {
            ClientNode = CurElem->data;

            // Does this client node need an update
            if( ClientNode->State == HNODE_NODE_STATE_NEW )
            {
                guint PacketLength;

                // Allocate a packet to send.
                Packet = g_hnode_packet_new();

                // Fill in some header information.                
                // Write the request length - 12 bytes
                g_hnode_packet_set_uint(Packet, 12);

                // Write the packet type - HNODE_MGMT_NODE_PKT
                g_hnode_packet_set_uint(Packet, HNODE_MGMT_NODE_PKT);

                // Write a packet tag 
                g_hnode_packet_set_uint(Packet, priv->NextIOTag);
                priv->NextIOTag += 1;

                // Write the ID value for this HNode 
                g_hnode_packet_set_uint(Packet, ClientNode->NodeID);

                // Have the provider fill out the node information.
                g_hnode_provider_build_client_update(ClientNode->Node, Packet);

                // Move back and write out the new packet size.
                PacketLength = g_hnode_packet_get_data_length(Packet);
                g_hnode_packet_reset(Packet);
                g_hnode_packet_set_uint(Packet, PacketLength);

                // Indicate that this client node is being exposed.
                ClientNode->State = HNODE_NODE_STATE_EXPOSING;

                // Send the packet
                g_hnode_pktsrc_send_packet(priv->ClientSource, Packet);

                // Do some client bookkeeping.
                priv->UpdatingNode = ClientNode;
                priv->State        = HNODE_CLIENT_STATE_UPDATING;
                return;
            }

            // Check the next client node
            CurElem = g_list_next(CurElem);
        }

        // Opps. Nothing to update.

    }

}

void 
g_hnode_client_add_node(GHNodeClient *sb, GHNodeProvider *Node)
{
    GHNodeClientNode *ClientNode;
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE (sb);

    // Check to make sure the new node is unique.
    
    
    // Add the new node to the client list
    ClientNode = g_new(GHNodeClientNode, 1);

    ClientNode->NodeID  = priv->NextNodeID;
    priv->NextNodeID   += 1;

    ClientNode->State   = HNODE_NODE_STATE_NEW;
    ClientNode->Node    = Node;
    
    g_object_ref(ClientNode->Node);

    priv->NodeList         = g_list_append(priv->NodeList, ClientNode);
    priv->TotalNodeCount  += 1;
    priv->UpdateNodeCount += 1;

    // Check if this node should be exposed to the client
    if( (priv->State == HNODE_CLIENT_STATE_IDLE) && (priv->EnableFlags & HNODE_ENABLE_REPORT_NODES) )
    {
        g_hnode_client_build_update_packet(sb);
    }
    
}

void 
g_hnode_client_remove_node(GHNodeClient *sb, GHNodeProvider *Node)
{

}

void 
g_hnode_client_start(GHNodeClient *sb)
{
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE (sb);

    //
    priv->State = HNODE_CLIENT_STATE_IDLE;

    // Start the client side processing
    g_hnode_pktsrc_start(priv->ClientSource);

	return;
}

void 
g_hnode_client_forward_debug_packet(GHNodeClient *sb, GHNodePacket *DebugPacket)
{
    GHNodeClientPrivate *priv;

    priv = G_HNODE_CLIENT_GET_PRIVATE (sb);

    // If debug isn't enabled then exit.
    if( (priv->EnableFlags & HNODE_ENABLE_DEBUG) == 0 )
        return;

    // Make sure client is in a state where debug packets can be forwarded.
    if( priv->State == HNODE_CLIENT_STATE_IDLE )
        return;

    // Take a reference on the Debug Packet; it will be released on tx success.
    g_object_ref(DebugPacket);

    // Send the packet
    g_hnode_pktsrc_send_packet(priv->ClientSource, DebugPacket);  

	return;
}

