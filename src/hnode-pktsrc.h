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

#include <glib.h>
#include <glib-object.h>
//#include <avahi-client/client.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#ifndef __G_HNODE_PKTSRC_H__
#define __G_HNODE_PKTSRC_H__

#ifdef __cplusplus
extern "C" {
#endif

G_BEGIN_DECLS

#define G_TYPE_HNODE_ADDRESS		    (g_hnode_address_get_type ())
#define G_HNODE_ADDRESS(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_ADDRESS, GHNodeAddress))
#define G_HNODE_ADDRESS_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_ADDRESS, GHNodeAddressClass))

typedef struct _GHNodeAddress		GHNodeAddress;
typedef struct _GHNodeAddressClass	GHNodeAddressClass;
typedef struct _GHNodeAddressEvent  GHNodeAddressEvent;

struct _GHNodeAddress
{
	GObject parent;
};

struct _GHNodeAddressClass
{
	GObjectClass parent_class;

};

GHNodeAddress *g_hnode_address_new (void);

void g_hnode_address_free (GHNodeAddress *Addr);

void g_hnode_address_set_str(GHNodeAddress *Address, gchar *AddrStr );

void g_hnode_address_get_str_ptr(GHNodeAddress *Address, gchar **AddrStr, guint32 *AddrStrLen);

gboolean g_hnode_address_get_ipv4_ptr(GHNodeAddress *Address, struct sockaddr_in **Addr, guint32 **AddrLen);

gboolean g_hnode_address_get_ipv4_address(GHNodeAddress *Address, struct sockaddr_in *Addr, guint32 *AddrLen);

gboolean g_hnode_address_set_ipv4_address(GHNodeAddress *Address, struct sockaddr_in *Addr, guint32 AddrLen);

gboolean g_hnode_address_ipv4_commit( GHNodeAddress *Address );

guint16 g_hnode_address_GetPort( GHNodeAddress *Address );

void g_hnode_address_set_port( GHNodeAddress *Address, guint16 Port );

gboolean g_hnode_address_is_equal( GHNodeAddress *Address, GHNodeAddress *TestAddrObj);


#define G_TYPE_HNODE_PACKET			    (g_hnode_packet_get_type ())
#define G_HNODE_PACKET(obj)			    (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_PACKET, GHNodePacket))
#define G_HNODE_PACKET_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_PACKET, GHNodePacketClass))

typedef struct _GHNodePacket		GHNodePacket;
typedef struct _GHNodePacketClass	GHNodePacketClass;
typedef struct _GHNodePacketEvent   GHNodePacketEvent;

//typedef enum _GHNodeEventType		GHNodeEventType;

//enum _GHNodePacketEventType
//{
//	G_HNODE_STATE,
//    G_HNODE_NEW_PKTSRC,
//    G_HNODE_RX,
//    G_HNODE_TXCOMPLETE,
//    G_HNODE_EVENT,
//};

enum GHNPacketType
{
   GHNPKT_HNODE_NONE,
};

struct _GHNodePacket
{
	GObject parent;
};

struct _GHNodePacketClass
{
	GObjectClass parent_class;

};

//struct _GHNodePacketEvent
//{
//	GHNodeEventType  eventtype;
//	guint32          ObjID;
//    guint8          *PktPtr;
//    guint32          PktLength;
//};


        
//typedef enum
//{
//	G_HNODE_BROWSER_BACKEND_AVAHI
//} GServicePacketBackend;

GHNodePacket *g_hnode_packet_new (void);

guint8 *g_hnode_packet_get_buffer_ptr(GHNodePacket *Packet, guint32 TotalLength);

GHNodeAddress *g_hnode_packet_get_setable_address_object(GHNodePacket *Packet);

GHNodeAddress *g_hnode_packet_get_address_object(GHNodePacket *Packet);

void g_hnode_packet_assign_addrobj(GHNodePacket *Packet, GHNodeAddress *AddrObj );

gboolean g_hnode_packet_clone_address(GHNodePacket *Packet, GHNodePacket *TemplatePacket);

gboolean g_hnode_packet_get_payload_ptr(GHNodePacket *Packet, guint8 **PayloadPtr, guint32 *PayloadLength);

guint8 *g_hnode_packet_get_offset_ptr(GHNodePacket *Packet, guint32 TotalLength);

gboolean g_hnode_packet_increment_data_length(GHNodePacket *Packet, guint32 DataLength);

guint32 g_hnode_packet_get_data_length(GHNodePacket *Packet);

void g_hnode_packet_reset(GHNodePacket *Packet);

void g_hnode_update_data_length(GHNodePacket *Packet);

gboolean g_hnode_packet_set_uint(GHNodePacket *Packet, guint32 Value);

guint32 g_hnode_packet_get_uint(GHNodePacket *Packet);

gboolean g_hnode_packet_set_short(GHNodePacket *Packet, guint16 Value);

guint16 g_hnode_packet_get_short(GHNodePacket *Packet);

gboolean g_hnode_packet_set_char(GHNodePacket *Packet, guint8 Value);

gchar g_hnode_packet_get_char(GHNodePacket *Packet);

gboolean g_hnode_packet_set_bytes(GHNodePacket *Packet, guint8 *Buffer, guint32 ByteCount);

gboolean g_hnode_packet_get_bytes(GHNodePacket *Packet, guint8 *Buffer, guint32 ByteCount);

gboolean g_hnode_packet_skip_bytes(GHNodePacket *Packet, guint32 SkipCount);


// Packet Source Object
#define G_TYPE_HNODE_PKTSRC			    (g_hnode_pktsrc_get_type ())
#define G_HNODE_PKTSRC(obj)			    (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_PKTSRC, GHNodePktSrc))
#define G_HNODE_PKTSRC_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_PKTSRC, GHNodePktSrcClass))

typedef struct _GHNodePktSrc		GHNodePktSrc;
typedef struct _GHNodePktSrcClass	GHNodePktSrcClass;
typedef struct _GHNodePktSrcEvent   GHNodePktSrcEvent;

//typedef enum _GHNodeEventType		GHNodeEventType;

//enum _GHNodePktSrcEventType
//{
//	G_HNODE_STATE,
//    G_HNODE_NEW_PKTSRC,
//    G_HNODE_RX,
//    G_HNODE_TXCOMPLETE,
//    G_HNODE_EVENT,
//};

struct _GHNodePktSrc
{
	GObject parent;
};

enum PacketSourceType
{
   PST_HNODE_TCP_HMNODE_LISTEN,
   PST_HNODE_TCP_HMNODE_LINK,
   PST_HNODE_TCP_HMNODE_LINK_ACCEPT,
   PST_HNODE_UDP_ASYNCH,
   PST_HNODE_UDP_HNODE,
};

struct _GHNodePktSrcClass
{
	GObjectClass parent_class;


	//AvahiClient *client;

	/* Signal handlers */
	void (* state_change)		(GHNodePktSrc *sb, guint eventID);
	void (* new_source)		    (GHNodePktSrc *sb, GHNodePktSrc *newsrc);
	void (* rx_packet)		    (GHNodePktSrc *sb, GHNodePacket *packet);
	void (* tx_packet)		    (GHNodePktSrc *sb, GHNodePacket *packet);

	//void (* hnode_found)		(GHNodePktSrc *sb, gpointer event);
	//void (* hnode_removed)	    (GHNodePktSrc *sb, gpointer event);
	//void (* discovery_start)	(GHNodePktSrc *sb, gpointer event);
	//void (* discovery_end) 	    (GHNodePktSrc *sb, gpointer event);
	//void (* hmnode_contact)	    (GHNodePktSrc *sb, gpointer event);
	//void (* debug_rx)	        (GHNodePktSrc *sb, gpointer event);
	//void (* event_rx)	        (GHNodePktSrc *sb, gpointer event);

};

struct _GHNodePktSrcEvent
{
//	GHNodeEventType  eventtype;
//	guint32          ObjID;
//    guint8          *PktPtr;
    guint32          PktLength;
};
        
//typedef enum
//{
//	G_HNODE_BROWSER_BACKEND_AVAHI
//} GServicePktSrcBackend;

GHNodePktSrc *g_hnode_pktsrc_new (guint32 Type);

// Prototypes
//PacketSource *pktsrc_SourceCreate( gint Type, char *Address, 
//                                    PKTSRC_RECVCALLBACK *RxCB, PKTSRC_SENTCALLBACK *TxCB, 
//                                    PKTSRC_STATECALLBACK *StateCB );

GHNodeAddress *g_hnode_pktsrc_get_address_object(GHNodePktSrc *Source);

GHNodeAddress *g_hnode_pktsrc_use_address_object(GHNodePktSrc *Source, GHNodeAddress *AddrObj);

void g_hnode_pktsrc_use_socket(GHNodePktSrc *Source, guint32 sockfd);

gboolean g_hnode_pktsrc_start( GHNodePktSrc *sb );

gboolean g_hnode_pktsrc_stop( GHNodePktSrc *sb );

gboolean g_hnode_pktsrc_send_packet(GHNodePktSrc *sb, GHNodePacket *Packet);

G_END_DECLS

#ifdef __cplusplus
}
#endif

#endif // _HMSRV_PKTSRC_H_

