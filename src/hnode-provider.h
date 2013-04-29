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

#ifndef __G_HNODE_PROVIDER_H__
#define __G_HNODE_PROVIDER_H__

G_BEGIN_DECLS

// Provider Endpoint Object
#define G_TYPE_HNODE_ENDPOINT			(g_hnode_endpoint_get_type ())
#define G_HNODE_ENDPOINT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_ENDPOINT, GHNodeEndPoint))
#define G_HNODE_ENDPOINT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_ENDPOINT, GHNodeEndPointClass))

typedef struct _GHNodeEndPoint		GHNodeEndPoint;
typedef struct _GHNodeEndPointClass	GHNodeEndPointClass;
typedef struct _GHNodeEndPointEvent   GHNodeEndPointEvent;

enum GHNEndPointType
{
   GHNEP_HNODE_NONE,
};

struct _GHNodeEndPoint
{
	GObject parent;
};

struct _GHNodeEndPointClass
{
	GObjectClass parent_class;

};

GHNodeEndPoint *g_hnode_endpoint_new (void);





// Server Provider Object
#define G_TYPE_HNODE_PROVIDER			    (g_hnode_provider_get_type ())
#define G_HNODE_PROVIDER(obj)			    (G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_PROVIDER, GHNodeProvider))
#define G_HNODE_PROVIDER_GET_CLASS(obj)	    (G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_PROVIDER, GHNodeProviderClass))

typedef struct _GHNodeProvider		GHNodeProvider;
typedef struct _GHNodeProviderClass	GHNodeProviderClass;
typedef struct _GHNodeProviderEvent   GHNodeProviderEvent;

struct _GHNodeProvider
{
	GObject parent;
};

struct _GHNodeProviderClass
{
	GObjectClass parent_class;


	//AvahiClient *client;

	/* Signal handlers */
	void (* state_change)		(GHNodeProvider *sb, guint32 newstate);
	void (* tx_request)	        (GHNodeProvider *sb);
	//void (* discovery_start)	(GHNodeProvider *sb, gpointer event);
	//void (* discovery_end) 	    (GHNodeProvider *sb, gpointer event);
	//void (* hmnode_contact)	    (GHNodeProvider *sb, gpointer event);
	//void (* debug_rx)	        (GHNodeProvider *sb, gpointer event);
	//void (* event_rx)	        (GHNodeProvider *sb, gpointer event);

};

// Event enumeration for the Provider State Callback
enum GHNProviderStateEvents
{
   GHNP_STATE_INFO_COMPLETE,
   GHNP_STATE_READY
};

GHNodeProvider *g_hnode_provider_new (void);

// Prototypes
guint16 g_hnode_provider_GetPort( GHNodeProvider *sb );
gboolean g_hnode_provider_set_address(GHNodeProvider *sb, gchar *AddressStr);
gboolean g_hnode_provider_start(GHNodeProvider *sb);

gboolean g_hnode_provider_claim(GHNodeProvider *sb, guint32 ServerObjID, guint16 DebugPort, guint16 EventPort);

gboolean g_hnode_provider_generate_tx_request(GHNodeProvider *sb, GHNodePacket *Packet);




G_END_DECLS


#endif // _HMSRV_PROVIDER_H_

