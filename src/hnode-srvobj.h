/**
 * hnode-srvobj.h
 *
 * Implements a GObject that has the basics of hnode server functionality.
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
#include <glib-object.h>
//#include <avahi-client/client.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#ifndef __G_HNODE_SERVER_H__
#define __G_HNODE_SERVER_H__

G_BEGIN_DECLS

// Define Packet header values.
enum HNodeManagementPacketType
{
    HNODE_MGMT_NONE          =  0,
    HNODE_MGMT_SHUTDOWN      =  1,
    HNODE_MGMT_REQ_PINFO     =  2,
    HNODE_MGMT_RSP_PINFO     =  3,
    HNODE_MGMT_DEBUG_CTRL    =  5,
    HNODE_MGMT_EVENT_CTRL    =  6,
    HNODE_MGMT_ACK           =  7,
    HNODE_MGMT_DEBUG_PKT     =  8,
    HNODE_MGMT_EVENT_PKT     =  9,
    HNODE_MGMT_NODE_PKT      = 10,
};

#define G_TYPE_HNODE_SERVER 			(g_hnode_server_get_type ())
#define G_HNODE_SERVER(obj)	    		(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_SERVER, GHNodeServer))
#define G_HNODE_SERVER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_SERVER, GHNodeServerClass))

typedef struct _GHNodeServer		 GHNodeServer;
typedef struct _GHNodeServerClass	 GHNodeServerClass;
typedef struct _GHNodeServerEvent	 GHNodeServerEvent;

typedef enum _GHNodeServerEventType	 GHNodeServerEventType;

enum _GHNodeServerEventType
{
    G_HNODE_SERVER_EVENT,
};

struct _GHNodeServer
{
	GObject parent;
};

struct _GHNodeServerClass
{
	GObjectClass parent_class;

	//AvahiClient *client;

	/* Signal handlers */
	void (* hnode_found)		(GHNodeServer *sb, gpointer event);

};

struct _GHNodeServerEvent
{
	GHNodeServerEventType  eventtype;
	guint32          ObjID;
    guint8          *PktPtr;
    guint32          PktLength;
};
        
//typedef enum
//{
//	G_HNODE_BROWSER_BACKEND_AVAHI
//} GServiceBrowserBackend;

GHNodeServer *g_hnode_server_new (void);

void g_hnode_server_start(GHNodeServer *sb);
//gboolean g_hnode_browser_open_hmnode( GHNodeBrowser *sb, guint32 MgmtServObjID );
//gboolean g_hnode_browser_get_provider_info( GHNodeBrowser *sb, guint32 ProviderObjID, GHNodeProvider *Provider );
//gboolean g_hnode_browser_get_endpoint_info( GHNodeBrowser *sb, guint32 ProviderObjID, guint32 EPIndex, GHNodeEndPoint *EndPoint );

//gboolean g_hnode_browser_build_ep_address( GHNodeBrowser *sb, guint32 ProviderObjID, guint32 EPIndex,
//                                             struct sockaddr *AddrStruct, guint32 *AddrLength);

//void g_hnode_browser_add_device_filter(GHNodeBrowser *sb, gchar **RequiredDeviceMimeStrs);
//void g_hnode_browser_reset_device_filter(GHNodeBrowser *sb);



// Server TCP Client Object
#define G_TYPE_HNODE_CLIENT			    (g_hnode_client_get_type ())
#define G_HNODE_CLIENT(obj)			    (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_CLIENT, GHNodeClient))
#define G_HNODE_CLIENT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_CLIENT, GHNodeClientClass))

typedef struct _GHNodeClient		GHNodeClient;
typedef struct _GHNodeClientClass	GHNodeClientClass;

struct _GHNodeClient
{
	GObject parent;
};

struct _GHNodeClientClass
{
	GObjectClass parent_class;


	//AvahiClient *client;

	/* Signal handlers */
	void (* state_change)		(GHNodeClient *sb, guint32 newstate);
	void (* tx_request)	        (GHNodeClient *sb);
	//void (* discovery_start)	(GHNodeProvider *sb, gpointer event);
	//void (* discovery_end) 	    (GHNodeProvider *sb, gpointer event);
	//void (* hmnode_contact)	    (GHNodeProvider *sb, gpointer event);
	//void (* debug_rx)	        (GHNodeProvider *sb, gpointer event);
	//void (* event_rx)	        (GHNodeProvider *sb, gpointer event);

    guint32 state_id;
    guint32 txreq_id;
};

GHNodeClient *g_hnode_client_new (void);

// Prototypes
void g_hnode_client_set_client_pktsrc(GHNodeClient *, GHNodePktSrc *);

G_END_DECLS

#endif
