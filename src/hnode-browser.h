/**
 * hnode-browser.h
 *
 * Implements a GObject for Rendezvous mDNS implementations wrapping Avahi
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

#ifndef __G_HNODE_BROWSER_H__
#define __G_HNODE_BROWSER_H__

G_BEGIN_DECLS

/*
typedef struct GHNodeProviderStruct GHNodeProvider;
struct GHNodeProviderStruct
{
    guint16 PortNumber;
    guint16 MicroVersion;
    guint8  MinorVersion;
    guint8  MajorVersion;
    guint32 EndPointCount;
    guint32 HostAddrType;
    guint32 HostAddrLength;
    guint8  HostAddr[128];
	guint8  NodeUID[16];
};

typedef struct GHNodeEndPointStruct GHNodeEndPoint;
struct GHNodeEndPointStruct
{
    guint16 PortNumber;
    guint16 MicroVersion;
    guint8  MinorVersion;
    guint8  MajorVersion;
    guint32 MimeStrLength;
    guint8  MimeTypeStr[256];
};

#define G_TYPE_HNODE_BROWSER_HNODE			(g_hnode_browser_hnode_get_type ())
#define G_HNODE_BROWSER_HNODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_BROWSER_HNODE, GHNodeBrowserHNode))
#define G_HNODE_BROWSER_HNODE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_BROWSER_HNODE, GHNodeBrowserHNodeClass))

typedef struct _GHNodeBrowserHNode		GHNodeBrowserHNode;
typedef struct _GHNodeBrowserHNodeClass	GHNodeBrowserHNodeClass;

struct _GHNodeBrowserHNode
{
	GObject parent;
};

struct _GHNodeBrowserHNodeClass
{
	GObjectClass parent_class;

	//AvahiClient *client;

	// Signal handlers 
	void (* hnode_found)		(GHNodeBrowser *sb, gpointer event);
	void (* hnode_removed)	    (GHNodeBrowser *sb, gpointer event);
	void (* discovery_start)	(GHNodeBrowser *sb, gpointer event);
	void (* discovery_end) 	    (GHNodeBrowser *sb, gpointer event);
	void (* hmnode_contact)	    (GHNodeBrowser *sb, gpointer event);
	void (* debug_rx)	        (GHNodeBrowser *sb, gpointer event);
	void (* event_rx)	        (GHNodeBrowser *sb, gpointer event);

};


        
GHNodeBrowserHNode *g_hnode_browser_hnode_new (void);

void g_hnode_browser_hnode_start(GHNodeBrowserHNode *sb);

*/





#define G_TYPE_HNODE_BROWSER_MPOINT	(g_hnode_browser_mpoint_get_type ())
#define G_HNODE_BROWSER_MPOINT(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_BROWSER_MPOINT, GHNodeBrowserMPoint))
#define G_HNODE_BROWSER_MPOINT_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_BROWSER_MPOINT, GHNodeBrowserMPointClass))

typedef struct _GHNodeBrowserMPoint		 GHNodeBrowserMPoint;
typedef struct _GHNodeBrowserMPointClass GHNodeBrowserMPointClass;

struct _GHNodeBrowserMPoint
{
	GObject parent;
};

struct _GHNodeBrowserMPointClass
{
	GObjectClass parent_class;

	//AvahiClient *client;

	/* Signal handlers */
	void (* hnode_add)	(GHNodeBrowserMPoint *sb, gpointer event);
	void (* cmnd_done)	(GHNodeBrowserMPoint *sb, gpointer event);
	void (* remove)	    (GHNodeBrowserMPoint *sb, gpointer event);

    guint32 hnode_add_id;
    guint32 cmnd_done_id;
    guint32 remove_id;

};
        
GHNodeBrowserMPoint *g_hnode_browser_mpoint_new (void);

void g_hnode_browser_mpoint_start(GHNodeBrowserMPoint *sb);




#define G_TYPE_HNODE_BROWSER_HNODE	(g_hnode_browser_hnode_get_type ())
#define G_HNODE_BROWSER_HNODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_BROWSER_HNODE, GHNodeBrowserHNode))
#define G_HNODE_BROWSER_HNODE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_BROWSER_HNODE, GHNodeBrowserHNodeClass))

typedef struct _GHNodeBrowserHNode		 GHNodeBrowserHNode;
typedef struct _GHNodeBrowserHNodeClass GHNodeBrowserHNodeClass;

struct _GHNodeBrowserHNode
{
	GObject parent;
};

struct _GHNodeBrowserHNodeClass
{
	GObjectClass parent_class;

	//AvahiClient *client;

	/* Signal handlers */
	void (* debug_rx)	    (GHNodeBrowserHNode *sb, gpointer event);
	void (* event_rx)	    (GHNodeBrowserHNode *sb, gpointer event);
    void (* update)         (GHNodeBrowserHNode *sb, gpointer event);
    void (* remove)         (GHNodeBrowserHNode *sb, gpointer event);

    guint32 update_id;
    guint32 debug_rx_id;
    guint32 event_rx_id;
    guint32 remove_id;

};
        
GHNodeBrowserHNode *g_hnode_browser_hnode_new (void);

void g_hnode_browser_hnode_start(GHNodeBrowserHNode *sb);





#define G_TYPE_HNODE_BROWSER			(g_hnode_browser_get_type ())
#define G_HNODE_BROWSER(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_BROWSER, GHNodeBrowser))
#define G_HNODE_BROWSER_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_BROWSER, GHNodeBrowserClass))

typedef struct _GHNodeBrowser		GHNodeBrowser;
typedef struct _GHNodeBrowserClass	GHNodeBrowserClass;
typedef struct _GHNodeEvent		    GHNodeEvent;

typedef enum _GHNodeEventType		GHNodeEventType;

enum _GHNodeEventType
{
	G_HNODE_DISC_START,
	G_HNODE_DISC_END,
    G_HNODE_DISC_HMNODE,
    G_HNODE_ADD,
    G_HNODE_REMOVE,
    G_HNODE_DEBUG,
    G_HNODE_EVENT,
};

struct _GHNodeBrowser
{
	GObject parent;
};

struct _GHNodeBrowserClass
{
	GObjectClass parent_class;

	//AvahiClient *client;

	/* Signal handlers */
	void (* hnode_add)		(GHNodeBrowser *sb, gpointer hnode);
	void (* mgmt_add)	    (GHNodeBrowser *sb, gpointer mpoint);
	void (* enum_cmplt)	    (GHNodeBrowser *sb, gpointer dummy);

    guint hnode_add_id;
    guint mgmt_add_id;
    guint enum_cmplt_id;
};

/*
struct _GHNodeEvent
{
	GHNodeEventType  eventtype;
	guint32          ObjID;
    guint8          *PktPtr;
    guint32          PktLength;
};

typedef struct GHNodeProviderStruct GHNodeProvider;
struct GHNodeProviderStruct
{
    guint16 PortNumber;
    guint16 MicroVersion;
    guint8  MinorVersion;
    guint8  MajorVersion;
    guint32 EndPointCount;
    guint32 HostAddrType;
    guint32 HostAddrLength;
    guint8  HostAddr[128];
	guint8  NodeUID[16];
};

typedef struct GHNodeEndPointStruct GHNodeEndPoint;
struct GHNodeEndPointStruct
{
    guint16 PortNumber;
    guint16 MicroVersion;
    guint8  MinorVersion;
    guint8  MajorVersion;
    guint32 MimeStrLength;
    guint8  MimeTypeStr[256];
};
*/        
//typedef enum
//{
//	G_HNODE_BROWSER_BACKEND_AVAHI
//} GServiceBrowserBackend;

GHNodeBrowser *g_hnode_browser_new (void);

void g_hnode_browser_start(GHNodeBrowser *sb);
gboolean g_hnode_browser_open_hmnode( GHNodeBrowser *sb, guint32 MgmtServObjID );

GHNodeBrowserMPoint *g_hnode_browser_first_mpoint(GHNodeBrowser *Browser);
GHNodeBrowserMPoint *g_hnode_browser_next_mpoint(GHNodeBrowser *Browser);
GHNodeBrowserHNode *g_hnode_browser_first_hnode(GHNodeBrowser *Browser);
GHNodeBrowserHNode *g_hnode_browser_next_hnode(GHNodeBrowser *Browser);


//gboolean g_hnode_browser_get_provider_info( GHNodeBrowser *sb, guint32 ProviderObjID, GHNodeProvider *Provider );
//gboolean g_hnode_browser_get_endpoint_info( GHNodeBrowser *sb, guint32 ProviderObjID, guint32 EPIndex, GHNodeEndPoint *EndPoint );

//gboolean g_hnode_browser_build_ep_address( GHNodeBrowser *sb, guint32 ProviderObjID, guint32 EPIndex,
//                                             struct sockaddr *AddrStruct, guint32 *AddrLength);

void g_hnode_browser_add_device_filter(GHNodeBrowser *sb, gchar **RequiredDeviceMimeStrs);
void g_hnode_browser_reset_device_filter(GHNodeBrowser *sb);

G_END_DECLS

#endif
