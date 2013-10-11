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
 
#include <string.h>
#include <stdio.h>
#include <glib.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <arpa/inet.h>
 
#include "hnode-pktsrc.h"
#include "hnode-uid.h"
#include "hnode-provider.h"

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
};

enum
{
    GHPROVIDER_TXS_INIT, 
    GHPROVIDER_TXS_REQ_PINFO,
    GHPROVIDER_TXS_WAIT_PINFO_RPY,
    GHPROVIDER_TXS_REQ_EPOINT,
    GHPROVIDER_TXS_WAIT_EPOINT_RPY,
    GHPROVIDER_TXS_WAIT_CLAIM,
    GHPROVIDER_TXS_SEND_SYSINFO,
    GHPROVIDER_TXS_WAIT_SYSINFO_ACK,
    GHPROVIDER_TXS_READY,
    GHPROVIDER_TXS_REQ_STATUS,
    GHPROVIDER_TXS_WAIT_STATUS_RPY,
};


#define G_HNODE_PROVIDER_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_PROVIDER, GHNodeProviderPrivate))

G_DEFINE_TYPE (GHNodeProvider, g_hnode_provider, G_TYPE_OBJECT);

typedef struct _GHNodeProviderPrivate GHNodeProviderPrivate;
struct _GHNodeProviderPrivate
{
    GHNodeUID *NodeUID;
	//guint8    NodeUID[16];

//	guint32   HostAddrType;
//	guint32   HostAddrLength;
//    guint8    HostAddr[256];
//    guint16   Port;

    GHNodeAddress *AddrObj;
    GHNodeAddress *ConfigAddrObj;

	guint8    MajorVersion;
	guint8    MinorVersion;
	guint16   MicroVersion;

    guint32    EndPointCount;
    GPtrArray *EndPointArray;

    guint32   TimeToLive;  // The number of discovery cycles to wait before removing this entry
    guint32   Flags;
    guint32   Status;

    guint32   TxState;
    guint32   ReqTag;
    guint32   ReqEndPoint;

    guint16   DebugPort;
    guint16   EventPort;

    guint16   ConfigPort;

    guint32   ServerObjID;

    //GHNodeProvider *HNodeSource;  
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
    PROVIDER_STATE_EVENT,
    PROVIDER_TX_REQUEST,
	PROVIDER_LAST_SIGNAL
};

static guint g_hnode_provider_signals[PROVIDER_LAST_SIGNAL] = { 0 };

/* GObject callbacks */
static void g_hnode_provider_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_provider_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);

static GObjectClass *parent_class = NULL;

static void
g_hnode_provider_dispose (GObject *obj)
{
//    GList               *CurElem;
//    GHNodeClientNode    *ClientNode;
    GHNodeProvider        *self = (GHNodeProvider *)obj;
    GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(self);

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

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    if(priv->ConfigAddrObj)
        g_object_unref(priv->ConfigAddrObj);

/*
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
*/

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
g_hnode_provider_finalize (GObject *obj)
{
    GHNodeProvider *self = (GHNodeProvider *)obj;

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_provider_class_init (GHNodeProviderClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_provider_dispose;
    o_class->finalize = g_hnode_provider_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	g_hnode_provider_signals[PROVIDER_STATE_EVENT] = g_signal_new (
			"state_change",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeProviderClass, state_change),
			NULL, NULL,
			g_cclosure_marshal_VOID__UINT,
			G_TYPE_NONE,
			1, G_TYPE_UINT);

	g_hnode_provider_signals[PROVIDER_TX_REQUEST] = g_signal_new (
			"tx_request",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeProviderClass, tx_request),
			NULL, NULL,
			g_cclosure_marshal_VOID__VOID,
			G_TYPE_NONE,
			0,
            NULL);

    parent_class = g_type_class_peek_parent (class);   
	g_type_class_add_private (o_class, sizeof (GHNodeProviderPrivate));
}

static void
g_hnode_provider_init (GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;
    //GHNodeServerSource  *NodeSource;

	priv = G_HNODE_PROVIDER_GET_PRIVATE (sb);
          
    // Initilize the ObjID value
    priv->NodeUID = NULL;

    priv->AddrObj        = NULL;
    priv->ConfigAddrObj  = NULL;
    priv->MajorVersion   = 0;
    priv->MinorVersion   = 0;
    priv->MicroVersion   = 0;

    priv->EndPointCount  = 0;
    priv->EndPointArray  = NULL;

    priv->ConfigPort     = 0;

    priv->TimeToLive     = 0;  
    priv->Flags          = 0;
    priv->Status         = 0;

    priv->ServerObjID    = 0;

    priv->dispose_has_run = FALSE;        
}

/*
static void
g_hnode_Provider_set_property (GObject 	*object,
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
g_hnode_Provider_get_property (GObject		*object,
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

GHNodeProvider *
g_hnode_provider_new (void)
{
	return g_object_new (G_TYPE_HNODE_PROVIDER, NULL);
}

gboolean
g_hnode_provider_set_address (GHNodeProvider *sb, gchar *AddressStr)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    if(priv->ConfigAddrObj)
        g_object_unref(priv->ConfigAddrObj);
    priv->ConfigAddrObj = NULL;

    // Get a new address object to set.
    priv->AddrObj = g_hnode_address_new();

    // If no address to set then exit.
    if(AddressStr == NULL)
        return FALSE;

    g_hnode_address_set_str(priv->AddrObj, AddressStr);

    // Check if config port applies
    if( priv->ConfigPort )
    {
        // Get a new address object to set.
        priv->ConfigAddrObj = g_hnode_address_new();

        g_hnode_address_set_str(priv->ConfigAddrObj, AddressStr);

        g_hnode_address_set_port(priv->ConfigAddrObj, priv->ConfigPort);
    }

    return FALSE;
}

gboolean
g_hnode_provider_update_config_addr(GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;
    gchar   *AddrStr;
    guint32  AddrStrLen;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // If an existing address; then free it.
    if(priv->AddrObj == NULL)
        return TRUE;

    if(priv->ConfigAddrObj)
        g_object_unref(priv->ConfigAddrObj);
    priv->ConfigAddrObj = NULL;

    // Check if config port applies
    if( priv->ConfigPort )
    {
        // Get a new address object to set.
        priv->ConfigAddrObj = g_hnode_address_new();

        g_hnode_address_get_str_ptr(priv->AddrObj, &AddrStr, &AddrStrLen);        
        g_hnode_address_set_str(priv->ConfigAddrObj, AddrStr );
        g_hnode_address_set_port(priv->ConfigAddrObj, priv->ConfigPort);
    }

    return FALSE;
}

gboolean
g_hnode_provider_free_endpoint_array(GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;
    guint                  EPIndex;
    GHNodeEndPoint        *EPPtr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    if(priv->EndPointArray == NULL)
        return;

    EPIndex = 0;
    while( EPIndex < priv->EndPointArray->len )
    {
         // Get a pointer to the endpoint index.
         EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndex);

         if(EPPtr)
            g_object_unref(EPPtr);

         // Check the next client node
         EPIndex += 1;
    }

    // Allocate an array for the endpoint data.
    priv->EndPointArray = (GPtrArray *)g_ptr_array_free(priv->EndPointArray, TRUE);

}

gboolean
g_hnode_provider_allocate_endpoint_array(GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // If an Array already exists then free it.
    g_hnode_provider_free_endpoint_array(sb);

    // Check if a new array needs to be allocated.
    if( priv->EndPointCount )
    {
        guint EPIndex;
        GHNodeEndPoint *EPPtr;

        // Allocate an array for the endpoint data.
        priv->EndPointArray = g_ptr_array_sized_new(priv->EndPointCount);

        EPIndex = 0;
        while( EPIndex < priv->EndPointCount )
        {
           // Get a pointer to the endpoint index.
           EPPtr = g_hnode_endpoint_new();

           g_ptr_array_add(priv->EndPointArray, EPPtr);
        
           // Check the next client node
           EPIndex += 1;
        }

    }
}

gboolean
g_hnode_provider_start (GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    priv->TxState = GHPROVIDER_TXS_REQ_PINFO;

    g_signal_emit(sb, g_hnode_provider_signals[PROVIDER_TX_REQUEST], 0, NULL);
}

gboolean
g_hnode_provider_claim (GHNodeProvider *sb, guint32 ServerObjID, guint16 DebugPort, guint16 EventPort)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // Make sure the provider is able to be claimed.
    if( priv->TxState != GHPROVIDER_TXS_WAIT_CLAIM )
        return TRUE;

    priv->DebugPort    = DebugPort;
    priv->EventPort    = EventPort;
    priv->ServerObjID  = ServerObjID;

    // Send out the sysinfo packet to claim the node.
    priv->TxState = GHPROVIDER_TXS_SEND_SYSINFO;

    g_signal_emit(sb, g_hnode_provider_signals[PROVIDER_TX_REQUEST], 0, NULL);

    return FALSE;
}

guint16 
g_hnode_provider_GetPort( GHNodeProvider *sb )
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    return 0;
} 

gboolean
g_hnode_provider_generate_tx_request(GHNodeProvider *sb, GHNodePacket *Packet)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);
 	
    switch( priv->TxState )
    {
        case GHPROVIDER_TXS_REQ_PINFO:
            // Reset the insertion point to zero.
            g_hnode_packet_reset(Packet);
        
            // Set-up the destination address.
            g_hnode_packet_assign_addrobj(Packet, priv->AddrObj);

            // Build the packet IU.
            g_hnode_packet_set_uint(Packet, HNPKT_PINFO_REQ);
            g_hnode_packet_set_uint(Packet, priv->ReqTag);

            // change the state to wait for the reply
            priv->TxState = GHPROVIDER_TXS_WAIT_PINFO_RPY;

        break;

        case GHPROVIDER_TXS_REQ_EPOINT:
            // Reset the insertion point to zero.
            g_hnode_packet_reset(Packet);
        
            // Set-up the destination address.
            g_hnode_packet_assign_addrobj(Packet, priv->AddrObj);

            // Build the packet IU.
            g_hnode_packet_set_uint(Packet, HNPKT_EPINFO_REQ);
            g_hnode_packet_set_uint(Packet, priv->ReqTag);
            g_hnode_packet_set_uint(Packet, priv->ReqEndPoint);

            // change the state to wait for the reply
            priv->TxState = GHPROVIDER_TXS_WAIT_EPOINT_RPY;

        break;

        case GHPROVIDER_TXS_SEND_SYSINFO:
        {
            time_t CurSec;
            struct tm *CurrentTime;

            // Reset the insertion point to zero.
            g_hnode_packet_reset(Packet);

            // Set-up the destination address.
            g_hnode_packet_assign_addrobj(Packet, priv->AddrObj);

            // Build the packet IU.
            g_hnode_packet_set_uint(Packet, HNPKT_SYSINFO_REQ);
            g_hnode_packet_set_uint(Packet, priv->ReqTag);

            // Grab the current system time and fill the 
            // packet with the appropriate time info.
            // Should probably use localtime_r for thread safety.
        	time(&CurSec);
            CurrentTime = localtime(&CurSec);

            g_hnode_packet_set_char(Packet, CurrentTime->tm_sec);
            g_hnode_packet_set_char(Packet, CurrentTime->tm_min);
            g_hnode_packet_set_char(Packet, CurrentTime->tm_hour);
            g_hnode_packet_set_char(Packet, CurrentTime->tm_wday);

            // Setup the ports for debug and event traffic.
            g_hnode_packet_set_short(Packet, priv->DebugPort);
            g_hnode_packet_set_short(Packet, priv->EventPort);

            // Pass the Server Object ID for use in future communications.
            g_hnode_packet_set_uint(Packet, priv->ServerObjID);

            // change the state to wait for the reply
            priv->TxState = GHPROVIDER_TXS_WAIT_SYSINFO_ACK;
        }
        break;

        case GHPROVIDER_TXS_REQ_STATUS:
        break;

        default:
        break;
    }

}

gboolean
g_hnode_provider_claim_rx_packet(GHNodeProvider *sb, GHNodePacket *Packet)
{
  	GHNodeProviderPrivate *priv;
    GHNodeAddress         *AddrObj;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    AddrObj = g_hnode_packet_get_address_object(Packet);

    if( g_hnode_address_is_equal(priv->AddrObj, AddrObj) )
        return TRUE;

    return FALSE;
}

gboolean
g_hnode_provider_process_rx_reply(GHNodeProvider *sb, GHNodePacket *Packet)
{
	GHNodeProviderPrivate *priv;
    guint32         PacketType;
    guint32         PacketTag;
    guint32         PacketEPIndex;
    GHNodeEndPoint *EPPtr; 

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    switch( priv->TxState )
    {
        case GHPROVIDER_TXS_WAIT_PINFO_RPY:
        {
        	guint32   HostAddrType;
        	guint32   HostAddrLength;
            guint8    HostAddr[256];
            guint16   Port;
            guint8    TmpUID[16];

            PacketType = g_hnode_packet_get_uint(Packet);
            PacketTag  = g_hnode_packet_get_uint(Packet);

	        // Check to see if this is the packet we are interested in.
	        if(	PacketType != HNPKT_PINFO_RPY )
                return TRUE;
                
	        if( PacketTag != priv->ReqTag )
                return TRUE;

    		priv->MicroVersion = g_hnode_packet_get_short(Packet);
    		priv->MinorVersion = g_hnode_packet_get_char(Packet);
    		priv->MajorVersion = g_hnode_packet_get_char(Packet);

    		// Get the unique ID for the node.
            g_hnode_packet_get_bytes(Packet, TmpUID, 16);

            if( priv->NodeUID )
                g_object_unref(priv->NodeUID);
            
            priv->NodeUID = g_hnode_uid_new();

            if( priv->NodeUID )
                g_hnode_uid_set_uid(priv->NodeUID, TmpUID);

        	priv->EndPointCount = g_hnode_packet_get_uint(Packet);

            // Get a clean array to populate endpoints into.
            g_hnode_provider_allocate_endpoint_array(sb);

            // Get the management port; although we use the one from discovery
         	Port = g_hnode_packet_get_short(Packet);

            // Retrieve the Config Port, 0 == unsupportted.
         	priv->ConfigPort = g_hnode_packet_get_short(Packet);
						
            g_hnode_provider_update_config_addr(sb);

    		// Save away the addressing information.
        	HostAddrType    = g_hnode_packet_get_uint(Packet);
            HostAddrLength  = g_hnode_packet_get_uint(Packet);
            g_hnode_packet_get_bytes(Packet, HostAddr, HostAddrLength);

            // Setup for the next request
            priv->ReqTag += 1;
            priv->ReqEndPoint = 0;
            priv->TxState = GHPROVIDER_TXS_REQ_EPOINT;
            g_signal_emit(sb, g_hnode_provider_signals[PROVIDER_TX_REQUEST], 0, NULL);
        }
        break;

        case GHPROVIDER_TXS_WAIT_EPOINT_RPY:
            PacketType    = g_hnode_packet_get_uint(Packet);
            PacketTag     = g_hnode_packet_get_uint(Packet);
            PacketEPIndex = g_hnode_packet_get_uint(Packet);

	        // Check to see if this is the packet we are interested in.
	        if(	PacketType != HNPKT_EPINFO_RPY )
                return TRUE;
                
	        if( PacketTag != priv->ReqTag )
                return TRUE;

	        if( PacketEPIndex != priv->ReqEndPoint )
                return TRUE;


            // Parse the endpoint information.
            EPPtr = g_ptr_array_index(priv->EndPointArray, priv->ReqEndPoint);
            g_hnode_endpoint_parse_rx_packet(EPPtr, priv->AddrObj, Packet);

            // Setup for the next request
            priv->ReqTag += 1;
            priv->ReqEndPoint += 1;
            if( priv->ReqEndPoint < priv->EndPointCount )
            {
                priv->TxState = GHPROVIDER_TXS_REQ_EPOINT;
                g_signal_emit(sb, g_hnode_provider_signals[PROVIDER_TX_REQUEST], 0, NULL);
            }
            else
            {
                priv->TxState = GHPROVIDER_TXS_WAIT_CLAIM;
                g_signal_emit(sb, g_hnode_provider_signals[PROVIDER_STATE_EVENT], 0, GHNP_STATE_INFO_COMPLETE);
            }


        break;

        case GHPROVIDER_TXS_WAIT_SYSINFO_ACK:

            // Go Idle
            priv->ReqTag += 1;
            priv->TxState = GHPROVIDER_TXS_READY;

            //extern void g_hnode_provider_debug_print(GHNodeProvider *sb);
            //g_hnode_provider_debug_print(sb);

            g_signal_emit(sb, g_hnode_provider_signals[PROVIDER_STATE_EVENT], 0, GHNP_STATE_READY);

        break;

        case GHPROVIDER_TXS_READY:

        break;

        case GHPROVIDER_TXS_WAIT_STATUS_RPY:

        break;

        default:
        break;
    }

    return FALSE;
}

gboolean 
g_hnode_provider_is_ready(GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    if( priv->TxState == GHPROVIDER_TXS_READY )
        return TRUE;

    return FALSE;
}

void 
g_hnode_provider_build_client_update(GHNodeProvider *sb, GHNodePacket *Packet)
{
	GHNodeProviderPrivate *priv;
    guint32          Port;
    guint32          AddrStrLen;
    guint8          *AddrStr;
    guint32          EPIndex;
    GHNodeEndPoint  *EPPtr;
    guint8           TmpUID[16];

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    g_hnode_packet_set_uint(Packet, priv->ServerObjID);

    Port = g_hnode_address_GetPort( priv->AddrObj );
    g_hnode_packet_set_short(Packet, Port);

    g_hnode_packet_set_short(Packet, priv->ConfigPort);

    g_hnode_packet_set_short(Packet, priv->MicroVersion);
    g_hnode_packet_set_char(Packet, priv->MinorVersion);
    g_hnode_packet_set_char(Packet, priv->MajorVersion);

	// Send the unique ID for the node.
    g_hnode_uid_get_uid(priv->NodeUID, TmpUID);
    g_hnode_packet_set_bytes(Packet, TmpUID, 16);

    g_hnode_packet_set_uint(Packet, priv->EndPointCount);

    g_hnode_packet_set_uint(Packet, 0); // Address Type

    g_hnode_address_get_str_ptr( priv->AddrObj, &AddrStr, &AddrStrLen);   
    g_hnode_packet_set_uint(Packet, AddrStrLen);
    g_hnode_packet_set_bytes(Packet, AddrStr, AddrStrLen);

    // Scan the list looking for a client node that needs to be updated.
    EPIndex = 0;
    while( EPIndex < priv->EndPointCount )
    {
         // Get a pointer to the endpoint index.
         EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndex);

         g_hnode_endpoint_build_client_update(EPPtr, Packet);

         // Check the next client node
         EPIndex += 1;
     }

}

void 
g_hnode_provider_parse_client_update(GHNodeProvider *sb, GHNodePacket *Packet)
{
	GHNodeProviderPrivate *priv;
    guint32  Port;
    guint32  AddrStrLen;
    guint8   AddrStr[256];
    guint8   TmpUID[16];

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    priv->ServerObjID = g_hnode_packet_get_uint(Packet);

    Port = g_hnode_packet_get_short(Packet);

    priv->ConfigPort = g_hnode_packet_get_short(Packet);    

    priv->MicroVersion = g_hnode_packet_get_short(Packet);
    priv->MinorVersion = g_hnode_packet_get_char(Packet);
    priv->MajorVersion = g_hnode_packet_get_char(Packet);

	// Get the unique ID for the node.
    g_hnode_packet_get_bytes(Packet, TmpUID, 16);

    if( priv->NodeUID )
       g_object_unref(priv->NodeUID);
            
    priv->NodeUID = g_hnode_uid_new();

    if( priv->NodeUID )
       g_hnode_uid_set_uid(priv->NodeUID, TmpUID);

    priv->EndPointCount = g_hnode_packet_get_uint(Packet);

    g_hnode_packet_get_uint(Packet); // Address Type

    AddrStrLen = g_hnode_packet_get_uint(Packet);
    g_hnode_packet_get_bytes(Packet, AddrStr, AddrStrLen);
    AddrStr[AddrStrLen] = '\0';

    g_hnode_provider_set_address(sb, AddrStr);   

    if( priv->EndPointCount )
    {
        guint EPIndex;
        GHNodeEndPoint *EPPtr;

        // Get a clean array to populate endpoints into.
        g_hnode_provider_allocate_endpoint_array(sb);

//        // Allocate an array for the endpoint data.
//        priv->EndPointArray = g_ptr_array_sized_new(priv->EndPointCount);

        EPIndex = 0;
        while( EPIndex < priv->EndPointCount )
        {
             // Get a pointer to the endpoint index.
             EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndex);

             g_hnode_endpoint_parse_client_update(EPPtr, priv->AddrObj, Packet);

             // Check the next client node
             EPIndex += 1;
        }

    }
}

gboolean
g_hnode_provider_get_address(GHNodeProvider *sb, GHNodeAddress **ObjAddr)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    *ObjAddr = priv->AddrObj;

    return FALSE;
}

gboolean
g_hnode_provider_get_version(GHNodeProvider *sb, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    *MajorVersion = priv->MajorVersion;
    *MinorVersion = priv->MinorVersion;
    *MicroVersion = priv->MicroVersion;

    return FALSE;
}

gboolean
g_hnode_provider_get_uid(GHNodeProvider *sb, guint8 *UIDArray)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    g_hnode_uid_get_uid(priv->NodeUID, UIDArray);

    return FALSE;
}

GHNodeUID *
g_hnode_provider_get_uid_objptr(GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    return priv->NodeUID;
}

gboolean
g_hnode_provider_compare_uid(GHNodeProvider *Node, GHNodeUID *CmpUID)
{	
    GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE (Node);

    return g_hnode_uid_is_equal( priv->NodeUID, CmpUID );
}

gboolean
g_hnode_provider_has_srvobj_id(GHNodeProvider *Node, guint32 SrvObjID)
{
    GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE (Node);

    if( priv->ServerObjID == SrvObjID )
        return TRUE;

    return FALSE;
}

gboolean
g_hnode_provider_supports_config(GHNodeProvider *Node)
{
    GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE (Node);

    if( priv->ConfigPort == 0 )
        return FALSE;

    return TRUE;
}

gboolean
g_hnode_provider_get_config_address(GHNodeProvider *sb, GHNodeAddress **ConfigAddr)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    *ConfigAddr = priv->ConfigAddrObj;

    return FALSE;
}

gboolean
g_hnode_provider_get_endpoint_count(GHNodeProvider *sb, guint32 *EPCount)
{
	GHNodeProviderPrivate *priv;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    *EPCount = priv->EndPointCount;
    
    return FALSE;
}

gboolean
g_hnode_provider_endpoint_get_address(GHNodeProvider *sb, guint32 EPIndx, GHNodeAddress **EPAddr)
{
	GHNodeProviderPrivate *priv;
    GHNodeEndPoint *EPPtr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // Get a pointer to the endpoint index.
    EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndx);

    if( EPPtr == NULL )
        return TRUE;

    // Have the endpoint return the requested data.
    return g_hnode_endpoint_get_address(EPPtr, EPAddr);
}

gboolean
g_hnode_provider_endpoint_get_version(GHNodeProvider *sb, guint32 EPIndx, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion)
{
	GHNodeProviderPrivate *priv;
    GHNodeEndPoint *EPPtr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // Get a pointer to the endpoint index.
    EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndx);

    if( EPPtr == NULL )
        return TRUE;

    // Have the endpoint return the requested data.
    return g_hnode_endpoint_get_version(EPPtr, MajorVersion, MinorVersion, MicroVersion);
}

gboolean
g_hnode_provider_endpoint_get_associate_index(GHNodeProvider *sb, guint32 EPIndx, guint32 *EPAssocIndex)   
{
	GHNodeProviderPrivate *priv;
    GHNodeEndPoint *EPPtr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // Get a pointer to the endpoint index.
    EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndx);

    if( EPPtr == NULL )
        return TRUE;

    // Have the endpoint return the requested data.
    return g_hnode_endpoint_get_associate_index(EPPtr, EPAssocIndex);
}

gboolean
g_hnode_provider_endpoint_get_type_str(GHNodeProvider *sb, guint32 EPIndx, gchar **EPTypeStrPtr)
{
	GHNodeProviderPrivate *priv;
    GHNodeEndPoint *EPPtr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

    // Get a pointer to the endpoint index.
    EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndx);

    if( EPPtr == NULL )
        return TRUE;

    // Have the endpoint return the requested data.
    return g_hnode_endpoint_get_type_str(EPPtr, EPTypeStrPtr);
}

gboolean
g_hnode_provider_supports_interface(GHNodeProvider *Node, gchar *EPTypeStrPtr, guint32 *EPIndex)
{
	GHNodeProviderPrivate *priv;
    GHNodeEndPoint *EPPtr;
    guint i;

	priv = G_HNODE_PROVIDER_GET_PRIVATE (Node);

    *EPIndex = 0;

    for(i = 0; i < priv->EndPointCount; i++)
    {
        EPPtr = g_ptr_array_index(priv->EndPointArray, i);
            
        if( g_hnode_endpoint_supports_interface(EPPtr, EPTypeStrPtr) == TRUE )
        {
            *EPIndex = i;
            return TRUE;
        }
    }

    // No matching interface found.
    return FALSE;
}

void
g_hnode_provider_debug_print(GHNodeProvider *sb)
{
	GHNodeProviderPrivate *priv;
    guint32          AddrStrLen;
    guint8          *AddrStr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

 
    g_hnode_address_get_str_ptr( priv->AddrObj, &AddrStr, &AddrStrLen);   

    g_message("AddrStr: %s\n", AddrStr);   
    g_message("Version: %d.%d.%d\n", priv->MajorVersion, priv->MinorVersion, priv->MicroVersion);
    
//    g_message("NodeID: %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n", 
//                priv->NodeUID[0], priv->NodeUID[1], priv->NodeUID[2], priv->NodeUID[3],
//                priv->NodeUID[4], priv->NodeUID[5], priv->NodeUID[6], priv->NodeUID[7],
//                priv->NodeUID[8], priv->NodeUID[9], priv->NodeUID[10], priv->NodeUID[11],
//                priv->NodeUID[12], priv->NodeUID[13], priv->NodeUID[14], priv->NodeUID[15]);   
    g_message("End Point Count: %d\n", priv->EndPointCount);   

    if( priv->EndPointCount )
    {
        guint EPIndex;
        GHNodeEndPoint *EPPtr;

        EPIndex = 0;
        while( EPIndex < priv->EndPointCount )
        {
             // Get a pointer to the endpoint index.
             EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndex);

             g_message("---End Point #%d---\n", EPIndex);

             g_hnode_endpoint_debug_print(EPPtr);

             // Check the next client node
             EPIndex += 1;
        }

    }
    

}

/*
void
g_hnode_provider_get_info(GHNodeProvider *sb, )
{
	GHNodeProviderPrivate *priv;
    guint32          AddrStrLen;
    guint8          *AddrStr;

	priv = G_HNODE_PROVIDER_GET_PRIVATE(sb);

 
    g_hnode_address_get_str_ptr( priv->AddrObj, &AddrStr, &AddrStrLen);   

    g_message("AddrStr: %s\n", AddrStr);   
    g_message("Version: %d.%d.%d\n", priv->MajorVersiog_messagen, priv->MinorVersion, priv->MicroVersion);
    g_message("NodeID: %x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x:%x\n", 
                priv->NodeUID[0], priv->NodeUID[1], priv->NodeUID[2], priv->NodeUID[3],
                priv->NodeUID[4], priv->NodeUID[5], priv->NodeUID[6], priv->NodeUID[7],
                priv->NodeUID[8], priv->NodeUID[9], priv->NodeUID[10], priv->NodeUID[11],
                priv->NodeUID[12], priv->NodeUID[13], priv->NodeUID[14], priv->NodeUID[15]);   
    g_message("End Point Count: %d\n", priv->EndPointCount);   

    if( priv->EndPointCount )
    {
        guint EPIndex;
        GHNodeEndPoint *EPPtr;

        EPIndex = 0;
        while( EPIndex < priv->EndPointCount )
        {
             // Get a pointer to the endpoint index.
             EPPtr = g_ptr_array_index(priv->EndPointArray, EPIndex);

             g_message("---End Point #%d---\n", EPIndex);

             g_hnode_endpoint_debug_print(EPPtr);

             // Check the next client node
             EPIndex += 1;
        }

    }
    

}
*/
/*
U32_T
ProviderUpdateProviderInfo(HNODE_RECORD  *curNode)
{
    U32_T Change = 0; 
    int Sock;
	U8_T MsgBuf[1024];
	int result;
	U32_T ReqTag;
	int Indx;

	HNPKT_PINFO_REQ_T   *PInfoReqPtr;
	HNPKT_PINFO_RPY_T   *PInfoRspPtr;
	HNPKT_EPINFO_REQ_T  *EPInfoReqPtr;
	HNPKT_EPINFO_RPY_T  *EPInfoRspPtr;
	HNPKT_SYSINFO_REQ_T *SysInfoReqPtr;
    
    struct sockaddr_in NodeAddr;
    struct sockaddr_in FromAddr;        
    unsigned int       socklen;

    time_t     CurSec;
	struct tm *CurrentTime;
	
    printf("ProviderUpdateProviderInfo -- start\n");

    // Send a query packet to the node.  Open a UDP socket 
	// to the nodes address.
    NodeAddr.sin_family = AF_INET;
    NodeAddr.sin_port   = htons(curNode->Port);
    inet_aton( (char *)curNode->HostAddr, &NodeAddr.sin_addr ); 
 
    // Open up a socket.            
    if ( (Sock = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
    {
	   perror("socket");    
       return 1;
    }

	printf("ProviderUpdateProviderInfo - Send Pinfo Request\n");

	// Build the request packet
	PInfoReqPtr = (HNPKT_PINFO_REQ_T *)MsgBuf;
	PInfoRspPtr = (HNPKT_PINFO_RPY_T *)MsgBuf;
	EPInfoReqPtr = (HNPKT_EPINFO_REQ_T *)MsgBuf;
	EPInfoRspPtr = (HNPKT_EPINFO_RPY_T *)MsgBuf;

	ReqTag = 0x65;

	PInfoReqPtr->PktType = htonl(HNPKT_PINFO_REQ);
	PInfoReqPtr->ReqTag  = htonl(ReqTag);

	// Send a request for info about the provider.
	if( sendto( Sock, MsgBuf, sizeof(HNPKT_PINFO_REQ_T),
               0, (struct sockaddr *)&NodeAddr, sizeof(NodeAddr)) == -1)
	{
	   perror("socket");    
       return 1;
	}	

	// Wait for a reply.
	socklen = sizeof(FromAddr);
	result = recvfrom( Sock, MsgBuf, sizeof(MsgBuf),
               0, (struct sockaddr *)&FromAddr, &socklen);
	
	if(result < 0)
	{
	   perror("socket");    
       return 1;
	}

	printf("ProviderUpdateProviderInfo - Recvd Packet (%d bytes)\n", result);

	// Check to see if this is the packet we are interested in.
	if(	  PInfoReqPtr->PktType == htonl(HNPKT_PINFO_RPY)
	   && PInfoReqPtr->ReqTag == htonl(ReqTag) )
	{
		printf("ProviderUpdateProviderInfo - Got Pinfo Response\n");

		// Get the unique ID for the node.
		memcpy(curNode->NodeUID, PInfoRspPtr->NodeUID, 16);
 						
		// Save away the addressing information.
    	curNode->HostAddrType    = ntohl(PInfoRspPtr->HostAddrType);
		curNode->HostAddrLength  = ntohl(PInfoRspPtr->HostAddrLength);
    	memcpy(curNode->HostAddr, PInfoRspPtr->HostAddr, curNode->HostAddrLength);
    	curNode->Port = ntohs(PInfoRspPtr->MgmtPort);

        printf("Port Update 3: %d (0x%x)\n", curNode->Port, curNode->Port);

		printf("Host Info -- AddrType:0x%x  AddrLength:%d  Port:0x%x\n",
				curNode->HostAddrType, curNode->HostAddrLength, curNode->Port);

		curNode->MajorVersion = PInfoRspPtr->MajorVersion;
		curNode->MinorVersion = PInfoRspPtr->MinorVersion;
		curNode->MicroVersion = ntohs(PInfoRspPtr->MicroVersion);

		printf("Version Info -- %d.%d.%d\n",
				 curNode->MajorVersion, curNode->MinorVersion, curNode->MicroVersion);

    	curNode->EndPointCount = ntohl(PInfoRspPtr->EndPointCount);

		printf("EP Info -- Count:%d\n", curNode->EndPointCount);

		// Allocate an EP record.
		curNode->EndPointArray = malloc( sizeof(HNODE_ENDPOINT) * curNode->EndPointCount );
		if(curNode->EndPointArray == NULL)
		{
			fprintf(stderr, "ERROR: Out of Memory!");
			exit(1);
		}

		// Fill in any endpoint information
		for( Indx = 0; Indx < curNode->EndPointCount; Indx++ )
		{
			ReqTag = 0x67;

			EPInfoReqPtr->PktType = htonl(HNPKT_EPINFO_REQ);
			EPInfoReqPtr->ReqTag  = htonl(ReqTag);
			EPInfoReqPtr->EndPointIndex  = htonl(Indx);

			// Send a request for info about the provider.
			if( sendto( Sock, MsgBuf, sizeof(HNPKT_EPINFO_REQ_T),
            		   0, (struct sockaddr *)&NodeAddr, sizeof(NodeAddr)) == -1)
			{
	   			perror("socket");    
       			return 1;
			}	

			// Wait for a reply.
			socklen = sizeof(FromAddr);
			result = recvfrom( Sock, MsgBuf, sizeof(MsgBuf),
            				   0, (struct sockaddr *)&FromAddr, &socklen);
	
			if(result < 0)
			{
	   			perror("socket");    
       			return 1;
			}

			printf("EP Check: 0x%x, 0x%x, 0x%x\n",
					 ntohl(EPInfoRspPtr->PktType),
					 ntohl(EPInfoRspPtr->ReqTag),
					 ntohl(EPInfoRspPtr->EndPointIndex));
 
			// Check to see if this is the packet we are interested in.
			if(	  EPInfoRspPtr->PktType == htonl(HNPKT_EPINFO_RPY)
			   && EPInfoRspPtr->ReqTag  == htonl(ReqTag)
			   && EPInfoRspPtr->EndPointIndex  == htonl(Indx) )
			{
				printf("EP Match\n");

				curNode->EndPointArray[Indx].MicroVersion = ntohs(EPInfoRspPtr->MicroVersion);
 				curNode->EndPointArray[Indx].MinorVersion = EPInfoRspPtr->MinorVersion;
 				curNode->EndPointArray[Indx].MajorVersion = EPInfoRspPtr->MajorVersion;
 				curNode->EndPointArray[Indx].Port = ntohs(EPInfoRspPtr->PortNumber);

 				curNode->EndPointArray[Indx].AssociateEPIndex = ntohs(EPInfoRspPtr->AssociateEPIndex);

 				curNode->EndPointArray[Indx].MimeTypeLength = ntohl(EPInfoRspPtr->MimeStrLength);
				memcpy(curNode->EndPointArray[Indx].MimeTypeStr, EPInfoRspPtr->MimeTypeStr, curNode->EndPointArray[Indx].MimeTypeLength);
			}
		}

	}

    // Send the sysinfo packet to setup time/debug/event
	printf("ProviderUpdateProviderInfo - Send SysInfo Packet\n");

	// Build the request packet
	SysInfoReqPtr = (HNPKT_SYSINFO_REQ_T *)MsgBuf;

	ReqTag = 0x68;

    // Fill out packet type, etc.
	SysInfoReqPtr->PktType   = htonl(HNPKT_SYSINFO_REQ);
	SysInfoReqPtr->ReqTag    = htonl(ReqTag);

    // Setup the ports for debug and event traffic.
    SysInfoReqPtr->DebugPort = htons(gHnodeDebugPort);
    SysInfoReqPtr->EventPort = htons(gHnodeEventPort);

    // Grab the current system time and fill the 
    // packet with the appropriate time info.
    // Should probably use localtime_r for thread safety.
	time(&CurSec);
    CurrentTime = localtime(&CurSec);

    SysInfoReqPtr->Seconds = CurrentTime->tm_sec;
    SysInfoReqPtr->Minutes = CurrentTime->tm_min;
    SysInfoReqPtr->Hours   = CurrentTime->tm_hour;
    SysInfoReqPtr->Day     = CurrentTime->tm_wday;

	// Send a request for info about the provider.
	if( sendto( Sock, MsgBuf, sizeof(HNPKT_SYSINFO_REQ_T),
               0, (struct sockaddr *)&NodeAddr, sizeof(NodeAddr)) == -1)
	{
	   perror("socket");    
       return 1;
	}	

	// Wait for a reply.
	//socklen = sizeof(FromAddr);
	//result = recvfrom( Sock, MsgBuf, sizeof(MsgBuf),
    //           0, (struct sockaddr *)&FromAddr, &socklen);
	
	//if(result < 0)
	//{
	//   perror("socket");    
    //   return 1;
	//}

	printf("ProviderUpdate - SysInfo complete.\n", result);
    
    
	return 0;
}
*/







