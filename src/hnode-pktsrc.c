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


#define UDP_MAX_PKTLENGTH 1500


/*************************************************************************************************************************
*
*
* Packet Source Object
*
*
*************************************************************************************************************************/

#define G_HNODE_PKTSRC_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_PKTSRC, GHNodePktSrcPrivate))

G_DEFINE_TYPE (GHNodePktSrc, g_hnode_pktsrc, G_TYPE_OBJECT);

typedef struct _GHNodePktSrcSource GHNodePktSrcSource;
struct _GHNodePktSrcSource
{
    GSource               Source;
    GHNodePktSrc         *Parent;
};

typedef struct _GHNodePktSrcPrivate GHNodePktSrcPrivate;
struct _GHNodePktSrcPrivate
{
    //GHNodeServerSource *Source;
    GHNodePktSrcSource    *Source;
    //GSource               *Source;
    GPollFD                Socket;

    //gchar                  AddrStr[128];
    //guint32                AddrStrLen;   

    GHNodeAddress         *AddrObj;

//    struct sockaddr_in     MyAddress;
//    socklen_t              MyAddrLength;
//    struct sockaddr_in     RMAddress;
//    socklen_t              RMAddrLength;

    gint                   Type;
    gint                   State;

    GList                 *TxPackets;
    GHNodePacket          *CurTx;
    gint                   CurTxOffset;

    GList                 *RxPackets;
    GHNodePacket          *CurRx;
    gint                   CurRxOffset;

    //PKTSRC_RECVCALLBACK   *RxCB;
    //PKTSRC_SENTCALLBACK   *TxCB;
    //PKTSRC_STATECALLBACK  *StateCB;
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
    PKTSRC_STATE_EVENT,
    PKTSRC_NEWSRC_EVENT,
    PKTSRC_RX_EVENT,
    PKTSRC_TX_EVENT,
	PKTSRC_LAST_SIGNAL
};

static guint g_hnode_pktsrc_signals[PKTSRC_LAST_SIGNAL] = { 0 };

/* GObject callbacks */
static void g_hnode_pktsrc_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_pktsrc_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);

static gboolean pktsrc_PrepareFunc(GSource *source, gint *timeout);
static gboolean pktsrc_CheckFunc(GSource *source);
static gboolean pktsrc_DispatchFunc(GSource *source, GSourceFunc callback, gpointer userdata);


static GObjectClass *parent_class = NULL;

static void
g_hnode_pktsrc_dispose (GObject *obj)
{
//    GList               *CurElem;
//    GHNodeClientNode    *ClientNode;
    GHNodePktSrc          *self = (GHNodePktSrc *)obj;
    GHNodePktSrcPrivate   *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE(self);

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

    if( priv->Source )
    {
        g_source_destroy( (GSource *)priv->Source );
        g_source_unref( (GSource *)priv->Source );
    }

    if( priv->AddrObj )
        g_object_unref(priv->AddrObj);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
g_hnode_pktsrc_finalize (GObject *obj)
{
    GHNodePktSrc *self = (GHNodePktSrc *)obj;

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_pktsrc_class_init (GHNodePktSrcClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_pktsrc_dispose;
    o_class->finalize = g_hnode_pktsrc_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	g_hnode_pktsrc_signals[PKTSRC_STATE_EVENT] = g_signal_new (
			"state_change",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodePktSrcClass, state_change),
			NULL, NULL,
			g_cclosure_marshal_VOID__UINT,
			G_TYPE_NONE,
			1, G_TYPE_UINT);

	g_hnode_pktsrc_signals[PKTSRC_NEWSRC_EVENT] = g_signal_new (
			"new_source",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodePktSrcClass, new_source),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	g_hnode_pktsrc_signals[PKTSRC_RX_EVENT] = g_signal_new (
			"rx_packet",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodePktSrcClass, rx_packet),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	g_hnode_pktsrc_signals[PKTSRC_TX_EVENT] = g_signal_new (
			"tx_packet",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodePktSrcClass, tx_packet),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

    parent_class = g_type_class_peek_parent (class);          
	g_type_class_add_private (o_class, sizeof (GHNodePktSrcPrivate));
}

static GSourceFuncs source_funcs = {
     pktsrc_PrepareFunc,
     pktsrc_CheckFunc,
     pktsrc_DispatchFunc,
    NULL,
    NULL,
    NULL
};

static void
g_hnode_pktsrc_init (GHNodePktSrc *sb)
{
	GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE (sb);

    // Get a source object to use when hooking into GLIB.
    priv->Source = (GHNodePktSrcSource *) g_source_new(&source_funcs, sizeof(GHNodePktSrcSource));
    priv->Source->Parent = sb;

    // Initilize the ObjID value
        //priv->Source         = NULL;
        //priv->Socket         = NULL;
        //priv->MyAddress    = NULL;
        //priv->MyAddrLength = 0;
        //priv->RMAddress    = NULL;
        //priv->RMAddrLength = 0;
        priv->AddrObj      = NULL;
        priv->Type         = 0;
        priv->State        = 0;
        priv->RxPackets    = NULL;
        priv->TxPackets    = NULL;
        priv->CurTx        = NULL;
        priv->CurTxOffset  = 0;
        priv->CurRx        = NULL;
        priv->CurRxOffset  = 0;
        priv->dispose_has_run = FALSE;

    g_main_context_ref( g_main_context_default() );

    g_source_attach((GSource *)priv->Source, g_main_context_default());
 
          
}

/*
static void
g_hnode_pktsrc_set_property (GObject 	*object,
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
g_hnode_pktsrc_get_property (GObject		*object,
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

GHNodePktSrc *
g_hnode_pktsrc_new (guint32 Type)
{
    GHNodePktSrc *sb;
    GHNodePktSrcPrivate *priv;

	sb = g_object_new (G_TYPE_HNODE_PKTSRC, NULL);

    if(sb)
    {
    	priv = G_HNODE_PKTSRC_GET_PRIVATE( sb );
        priv->Type = Type;
    }

    return sb;
}

GHNodeAddress *
g_hnode_pktsrc_get_address_object(GHNodePktSrc *Source)
{
    GHNodePktSrcPrivate *priv;

	priv  = G_HNODE_PKTSRC_GET_PRIVATE( Source );

    // Get a new address object to set.
    if(priv->AddrObj == NULL)
        priv->AddrObj = g_hnode_address_new();

    return priv->AddrObj;
}

GHNodeAddress *
g_hnode_pktsrc_use_address_object(GHNodePktSrc *Source, GHNodeAddress *AddrObj)
{
    GHNodePktSrcPrivate *priv;

	priv  = G_HNODE_PKTSRC_GET_PRIVATE( Source );

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    priv->AddrObj = g_object_ref(AddrObj);

    return priv->AddrObj;
}

void
g_hnode_pktsrc_use_socket(GHNodePktSrc *Source, guint32 sockfd)
{
    GHNodePktSrcPrivate *priv;

	priv  = G_HNODE_PKTSRC_GET_PRIVATE( Source );

    priv->Socket.fd = sockfd;

}

/*  
guint16 
g_hnode_pktsrc_GetPort( GHNodePktSrc *sb )
{
    GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( sb );

    return g_hnode_address_GetPort(priv->AddrObj);
} 

gboolean 
g_hnode_pktsrc_set_address( GHNodePktSrc *sb, gchar *Address )
{
    GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( sb );

    // Allocate an address object
    priv->AddrObj = g_hnode_address_new();

    // Copy the address string.
    g_hnode_address_set_str(priv->AddrObj, Address);

    return FALSE;
} 
*/
gboolean 
g_hnode_pktsrc_start( GHNodePktSrc *sb )
{
    GHNodePktSrcPrivate *priv;
    struct sockaddr_in *AddrPtr;
    guint32  *AddrLen;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( sb );

    // Allocate the socket based on the type
    switch( priv->Type )
    {
        case PST_HNODE_UDP_ASYNCH:
        {
            // Open up a UDP socket to handle debug packets
            if ( (priv->Socket.fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
            {
                g_error("Error creating server socket.\n");
                return TRUE;
            }

            // Make sure an address object exists.
            g_hnode_pktsrc_get_address_object(sb);

            // Get a pointer to the address object
            g_hnode_address_get_ipv4_ptr(priv->AddrObj, &AddrPtr, &AddrLen); 

	        // Init the address structure.
	        //*AddrLen                 = sizeof(priv->MyAddress);
            AddrPtr->sin_family      = AF_INET;
            AddrPtr->sin_port        = 0;
	        AddrPtr->sin_addr.s_addr = INADDR_ANY;
      
            // Call Bind to allocate a port number for the socket.
            if ( bind(priv->Socket.fd, (struct sockaddr *)AddrPtr, *AddrLen) < 0 )
            {
                g_error("Error binding device socket.\n");
                return TRUE;
            }
       
	        // Get the port address given to us by bind.
            memset( AddrPtr, 0, *AddrLen );  
	        getsockname(priv->Socket.fd, (struct sockaddr *)AddrPtr, AddrLen);

            // Commit the new address information
            g_hnode_address_ipv4_commit(priv->AddrObj);
        }
        break;

        // Listen for TCP connection requests from clients.
        case PST_HNODE_TCP_HMNODE_LISTEN:
            // Open up a TCP socket to handle client requests
            if ( (priv->Socket.fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
            {
                g_error("Error creating server socket.\n");
                return TRUE;
            }
     
            // Mark the socket so it will listen for incoming connections 
            if( listen(priv->Socket.fd, 4) < 0 )
            {
                g_error("Failed to listen on server socket.\n");
                return TRUE;
            }

            // Make sure an address object exists.
            g_hnode_pktsrc_get_address_object(sb);

            // Get a pointer to the address object
            g_hnode_address_get_ipv4_ptr(priv->AddrObj, &AddrPtr, &AddrLen); 

	        // Init the address structure.
	        //*AddrLen                 = sizeof(priv->MyAddress);

            // Get the port address given to us by bind.
            memset( AddrPtr, 0, *AddrLen );  
	        getsockname(priv->Socket.fd, (struct sockaddr *)AddrPtr, AddrLen);

            // Commit the new address information
            g_hnode_address_ipv4_commit(priv->AddrObj);

        break;

        // Formalize a connection to a client.  The socket will
        // have been received from the listen call.
        case PST_HNODE_TCP_HMNODE_LINK:
            // Open up a TCP socket to handle client requests
            if ( (priv->Socket.fd = socket(PF_INET, SOCK_STREAM, IPPROTO_TCP)) < 0 )
            {
                g_error("Error creating server socket.\n");
                return TRUE;
            }

            // Make sure an address object exists.
            g_hnode_pktsrc_get_address_object(sb);

            // Get a pointer to the address object
            g_hnode_address_get_ipv4_ptr(priv->AddrObj, &AddrPtr, &AddrLen); 

            // Try to connect to the management server.
            if ( connect(priv->Socket.fd, AddrPtr, *AddrLen) < 0 )
            {
                g_printf("Error connecting socket.\n");
                //Error = 1;
                return TRUE;
            }

        break;

        // Open a UDP socket that is used for communication with hnodes.
        case PST_HNODE_UDP_HNODE:
            // Open up a socket.            
            if ( (priv->Socket.fd = socket(PF_INET, SOCK_DGRAM, IPPROTO_UDP)) < 0 )
            {
        	   g_error("socket");    
               return TRUE;
            }

            // Resolve a string address
//            if( pktsrc_ResolveAddressStr(AddressStr, priv->RemoteAddr, priv->RemoteAddrLength ) )
//            {
//
//
//            }


        break;

        // This one was already setup on the accept side.
        // So change state to indicate readiness.
        case PST_HNODE_TCP_HMNODE_LINK_ACCEPT:
            priv->Type = PST_HNODE_TCP_HMNODE_LINK;
        break;

        default:
        break;
    }
 
    priv->Socket.events  = G_IO_IN | G_IO_ERR | G_IO_HUP; // G_IO_OUT
    priv->Socket.revents = 0;
       
    g_source_add_poll((GSource *)priv->Source, &priv->Socket);

    return FALSE;
} 

gboolean 
g_hnode_pktsrc_stop( GHNodePktSrc *sb )
{
    GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( sb );

    return TRUE;
} 

gboolean 
g_hnode_pktsrc_send_packet(GHNodePktSrc *sb, GHNodePacket *Packet)
{
    GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( sb );

    // Add this packet to the transmit queue
    priv->TxPackets = g_list_append(priv->TxPackets, Packet);

    // Enable G_IO_OUT so that we get a tx-event
    priv->Socket.events  |= G_IO_OUT;

    g_source_remove_poll((GSource *)priv->Source, &priv->Socket);
    g_source_add_poll((GSource *)priv->Source, &priv->Socket);

    return FALSE;
}



static gboolean 
pktsrc_PrepareFunc(GSource *source, gint *timeout)
{
    GHNodePktSrcSource  *PktSource = (GHNodePktSrcSource *)source;
    GHNodePktSrc        *PktSrc = PktSource->Parent;
    GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( PktSrc );

   *timeout = -1;
        
    return FALSE;
}

static gboolean 
pktsrc_CheckFunc(GSource *source)
{
    GHNodePktSrcSource  *PktSource = (GHNodePktSrcSource *)source;
    GHNodePktSrc        *PktSrc = PktSource->Parent;
    GHNodePktSrcPrivate *priv;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( PktSrc );

    if( priv->Socket.revents > 0 )
    {
        return TRUE;
    }
              
    return FALSE;
}

static gboolean 
pktsrc_DispatchFunc(GSource *source, GSourceFunc callback, gpointer userdata) 
{
    GHNodePktSrcSource  *PktSource = (GHNodePktSrcSource *)source;
    GHNodePktSrc        *PktSrc = PktSource->Parent;
    GHNodePktSrcPrivate *priv;

    GHNodePktSrc        *newSource;
    GHNodePacket        *Packet;

    guint8              *DataPtr;
    guint32              DataLength;

    struct sockaddr_in  *RxAddr;
    guint32             *RxAddrLen;

	priv = G_HNODE_PKTSRC_GET_PRIVATE( PktSrc );

    if( priv->Socket.revents > 0)
    {
        if( priv->Socket.revents & (G_IO_ERR | G_IO_HUP) )
        {
            // The other end of the socket has gone down.
            // Close things out an clean up.  Also tell the 
            // upper layer that things failed.
            close( priv->Socket.fd );

            // Call the State Callback to indicate received data.
            g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_STATE_EVENT], 0, 0);                    

            return FALSE;        
        }

        if( priv->Socket.revents & G_IO_IN )
        {
            switch( priv->Type )
            {
                // Client Open Requests
                case PST_HNODE_TCP_HMNODE_LISTEN:
                {
                    GHNodeAddress *AddrObj;
                    guint32        tmpfd;

                    // Create a new packet source to send back in the callback.
                    newSource = g_hnode_pktsrc_new(PST_HNODE_TCP_HMNODE_LINK_ACCEPT);

                    // Initialize the new source.
                    // Get a settable address structure
                    AddrObj = g_hnode_pktsrc_get_address_object(newSource);

                    // Get a destination address structure
                    g_hnode_address_get_ipv4_ptr(AddrObj, &RxAddr, &RxAddrLen);

                    // Get the socket for the new tcp connection.   
                    tmpfd = accept(priv->Socket.fd, (struct sockaddr *)RxAddr, RxAddrLen);

                    // Assign the new socket to the new packet source. 
                    g_hnode_pktsrc_use_socket(newSource, tmpfd);

                    // Commit the new address.
                    g_hnode_address_ipv4_commit(AddrObj);                    

                    // Make the callback to tell the upper layer that a new source
                    // was created and has been added to the event loop.
                    g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_NEWSRC_EVENT], 0, newSource);
                }
                break;

                // Receive a packet from a UDP socket.  This Packet could be upto the maximum UDP length.
                // For our application here we will not consider Jumbo packets so the maximum length will
                // be on the order of 1500 bytes.
                case PST_HNODE_UDP_HNODE:
                case PST_HNODE_UDP_ASYNCH:
                {
                    GHNodeAddress      *AddrObj;

                    // Allocate the packet to receive the data into.
                    Packet = g_hnode_packet_new();               
                     
                    // If we were unable to get a Packet then bail-out in the hopes that
                    // another spin through the event loop will free up enough memory.
                    if( Packet == NULL )
                        return FALSE;

                    // Allocate enough space for the maximum Ethernet UDP packet
                    DataPtr = g_hnode_packet_get_buffer_ptr(Packet, 1500);

                    if( DataPtr == NULL )
                    {
                        return FALSE;
                    }

                    // Get the address object
                    AddrObj = g_hnode_packet_get_setable_address_object(Packet);

                    // Get a destination address structure
                    g_hnode_address_get_ipv4_ptr(AddrObj, &RxAddr, &RxAddrLen);

                    DataLength = recvfrom( priv->Socket.fd, DataPtr, 1500, 0,
                                            (struct sockaddr *)RxAddr, RxAddrLen);

                    if( DataLength == -1 )
                    {
                        return FALSE;
                    }
            
                    g_hnode_packet_increment_data_length(Packet, DataLength);
                    
                    g_hnode_address_ipv4_commit(AddrObj);

                    // Call the RX Callback to indicate received data.
                    g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_RX_EVENT], 0, Packet);                    
                }
                break;

                // Formalize a connection to a client.  The socket will
                // have been received from the listen call.
                case PST_HNODE_TCP_HMNODE_LINK:
                {
                    guint32 PacketLength = 0;

                    // Allocate the packet to receive the data into.
                    Packet = g_hnode_packet_new();               
                     
                    // If we were unable to get a Packet then bail-out in the hopes that
                    // another spin through the event loop will free up enough memory.
                    if( Packet == NULL )
                        return FALSE;

                    // Allocate enough space for the standard header
                    DataPtr = g_hnode_packet_get_buffer_ptr(Packet, 12);

                    if( DataPtr == NULL )
                    {
                        return FALSE;
                    }

                    DataLength = recv( priv->Socket.fd, DataPtr, 12, 0);

                    if( (DataLength == 0) || (DataLength == -1) )
                    {
                        // The other end of the socket has gone down.
                        // Close things out an clean up.  Also tell the 
                        // upper layer that things failed.
                        close( priv->Socket.fd );

                        // Call the State Callback to indicate received data.
                        g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_STATE_EVENT], 0, 0);                    
    
                        return FALSE;
                    }
            
                    g_hnode_packet_increment_data_length(Packet, DataLength);

                    PacketLength = g_hnode_packet_get_uint(Packet);                    

                    g_hnode_packet_skip_bytes(Packet, 8);

                    if( PacketLength > 12 )
                    {
                        DataPtr = g_hnode_packet_get_offset_ptr(Packet, PacketLength);

                        // Try to get the remainder of the packet.
                        if( DataPtr == NULL )
                        {
                            return FALSE;
                        }

                        DataLength = recv( priv->Socket.fd, DataPtr, PacketLength-12, 0);

                        if( DataLength == -1 )
                        {
                            return FALSE;
                        }
                        
                        g_hnode_packet_increment_data_length(Packet, DataLength);

                    }

                    g_hnode_packet_reset(Packet);

                    // Call the RX Callback to indicate received data.
                    g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_RX_EVENT], 0, Packet);                    
                }
                break;

            }

            
        }
       
        // Handle transmit requests.
        if( priv->Socket.revents & G_IO_OUT )
        {
            // Get the Packet pointer to use
            if( priv->CurTx == NULL )
            {
                GList *TmpElem;

                // Get the next packet to work on.
                TmpElem = g_list_first( priv->TxPackets );

                if( TmpElem == NULL )
                    return;

                priv->CurTx       = TmpElem->data;                
                priv->CurTxOffset = 0; 

                // Exit if there isn't work todo
                if( priv->CurTx )
                {
                    return;
                }
            }


            // Send the packet
            switch( priv->Type )
            {
                case PST_HNODE_TCP_HMNODE_LISTEN:
        
                break;

                // An UDP socket.
                case PST_HNODE_UDP_HNODE:
                case PST_HNODE_UDP_ASYNCH:
                {
                    GHNodeAddress      *AddrObj;
                    struct sockaddr_in *DestAddr;
                    guint32            *DestAddrLen;
                    guint8             *PktData;
                    guint32             PktLength;
                    guint32             SentLength;

                    // Initialize the length value
                    AddrObj = g_hnode_packet_get_address_object(priv->CurTx);

                    // Get a destination address structure
                    g_hnode_address_get_ipv4_ptr(AddrObj, &DestAddr, &DestAddrLen);

                    // Get a data payload pointer and length
                    g_hnode_packet_get_payload_ptr(priv->CurTx, &PktData, &PktLength);

                     // Send the data
                   	SentLength = sendto( priv->Socket.fd, PktData + priv->CurTxOffset, PktLength - priv->CurTxOffset,
                                            0, (struct sockaddr *)DestAddr, *DestAddrLen);

                    // Process the completion
                    if( SentLength == -1 )
                	{
                        // Done with this packet
                        perror("socket"); 
                        exit(-1);   
                        return 1;
	                }	
                    else if( SentLength == (PktLength - priv->CurTxOffset) )
                	{
                        // Done with this packet
                        priv->TxPackets = g_list_remove(priv->TxPackets, priv->CurTx);

                        // Callback to indicate TX complete
                        g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_TX_EVENT], 0, priv->CurTx);                    

                        // done with this packet.                        
                        priv->CurTx = NULL;

                        // If there isn't anything else to send then turn of the send event.
                        if( g_list_first( priv->TxPackets ) == NULL )
                        {    
                            // Turn off G_IO_OUT
                            priv->Socket.events  &= ~G_IO_OUT;

                            g_source_remove_poll((GSource *)priv->Source, &priv->Socket);
                            g_source_add_poll((GSource *)priv->Source, &priv->Socket);
                        }

	                }	
                    else
                    {
                        priv->CurTxOffset += SentLength;
                    }
                }
                break;

                // A TCP socket.
                case PST_HNODE_TCP_HMNODE_LINK:
                {
                    GHNodeAddress      *AddrObj;
                    struct sockaddr_in *DestAddr;
                    guint32            *DestAddrLen;
                    guint8             *PktData;
                    guint32             PktLength;
                    guint32             SentLength;

                    // Get a data payload pointer and length
                    g_hnode_packet_get_payload_ptr(priv->CurTx, &PktData, &PktLength);

                    // Send the data
                   	SentLength = send( priv->Socket.fd, PktData + priv->CurTxOffset, PktLength - priv->CurTxOffset, 0);

                    // Process the completion
                    if( SentLength == -1 )
                	{
                        // Done with this packet
                        perror("socket"); 
                        exit(-1);   
                        return 1;
	                }	
                    else if( SentLength == (PktLength - priv->CurTxOffset) )
                	{
                        // Done with this packet
                        priv->TxPackets = g_list_remove(priv->TxPackets, priv->CurTx);

                        // Callback to indicate TX complete
                        g_signal_emit(PktSrc, g_hnode_pktsrc_signals[PKTSRC_TX_EVENT], 0, priv->CurTx);                    

                        // done with this packet.                        
                        priv->CurTx = NULL;

                        // If there isn't anything else to send then turn of the send event.
                        if( g_list_first( priv->TxPackets ) == NULL )
                        {    
                            // Turn off G_IO_OUT
                            priv->Socket.events  &= ~G_IO_OUT;

                            g_source_remove_poll((GSource *)priv->Source, &priv->Socket);
                            g_source_add_poll((GSource *)priv->Source, &priv->Socket);
                        }

	                }	
                    else
                    {
                        priv->CurTxOffset += SentLength;
                    }
                }
                break;
                
            }
        }

            
        priv->Socket.revents = 0;
    }
                                        
    return TRUE;
}

