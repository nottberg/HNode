#ifndef __G_HNODE_PROVIDER_H__
#define __G_HNODE_PROVIDER_H__

#include <glib.h>
#include <glib-object.h>
//#include <avahi-client/client.h>

#include <sys/socket.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <arpa/inet.h>

#include "hnode-uid.h"

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

gboolean g_hnode_provider_get_address(GHNodeProvider *sb, GHNodeAddress **ObjAddr);
gboolean g_hnode_provider_get_version(GHNodeProvider *sb, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion);
gboolean g_hnode_provider_get_uid(GHNodeProvider *sb, guint8 *UIDArray);
GHNodeUID *g_hnode_provider_get_uid_objptr(GHNodeProvider *sb);
gboolean g_hnode_provider_compare_uid(GHNodeProvider *Node, GHNodeUID *CmpUID);
gboolean g_hnode_provider_has_srvobj_id(GHNodeProvider *Node, guint32 SrvObjID);
gboolean g_hnode_provider_supports_config(GHNodeProvider *Node);
gboolean g_hnode_provider_get_config_address(GHNodeProvider *sb, GHNodeAddress **ConfigAddr);
gboolean g_hnode_provider_get_endpoint_count(GHNodeProvider *sb, guint32 *EPCount);
gboolean g_hnode_provider_endpoint_get_address(GHNodeProvider *sb, guint32 EPIndx, GHNodeAddress **EPAddr);
gboolean g_hnode_provider_endpoint_get_version(GHNodeProvider *sb, guint32 EPIndx, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion);
gboolean g_hnode_provider_endpoint_get_associate_index(GHNodeProvider *sb, guint32 EPIndx, guint32 *EPAssocIndex);
gboolean g_hnode_provider_endpoint_get_type_str(GHNodeProvider *sb, guint32 EPIndx, gchar **EPTypeStrPtr);
gboolean g_hnode_provider_supports_interface(GHNodeProvider *Node, gchar *EPTypeStrPtr, guint32 *EPIndex);

G_END_DECLS


#endif // _HMSRV_PROVIDER_H_

