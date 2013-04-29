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
#include "hnode-provider.h"
#include "hnode-browser.h"

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

#define G_HNODE_BROWSER_MPOINT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_BROWSER_MPOINT, GHNodeBrowserMPointPrivate))

G_DEFINE_TYPE (GHNodeBrowserMPoint, g_hnode_browser_mpoint, G_TYPE_OBJECT);

typedef struct _GHNodeBrowserMPointPriv GHNodeBrowserMPointPrivate;
struct _GHNodeBrowserMPointPriv
{
    GHNodePktSrc    *MSource;

    GHNodeAddress   *AddrObj;

    guint32          CurIOTag;

    GList           *NodeList;

    gboolean                   dispose_has_run;

};

//static guint g_hnode_browser_mpoint_signals[LAST_SIGNAL] = { 0 };

/* GObject callbacks */
static void g_hnode_browser_mpoint_set_property (GObject 	 *object,
					    guint	  prop_id,
					    const GValue *value,
					    GParamSpec	 *pspec);
static void g_hnode_browser_mpoint_get_property (GObject	 *object,
					    guint	  prop_id,
					    GValue	 *value,
					    GParamSpec	 *pspec);

static GObjectClass *parent_class = NULL;

static void
g_hnode_browser_mpoint_dispose (GObject *obj)
{
    GHNodeBrowserMPoint          *self = (GHNodeBrowserMPoint *)obj;
    GHNodeBrowserMPointPrivate   *priv;

	priv = G_HNODE_BROWSER_MPOINT_GET_PRIVATE(self);

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
g_hnode_browser_mpoint_finalize (GObject *obj)
{
    GHNodeBrowserMPoint *self = (GHNodeBrowserMPoint *)obj;
    
    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_browser_mpoint_class_init (GHNodeBrowserMPointClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_browser_mpoint_dispose;
    o_class->finalize = g_hnode_browser_mpoint_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	class->hnode_add_id = g_signal_new (
			"hnode-add",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserMPointClass, hnode_add),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->remove_id = g_signal_new (
			"remove",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserMPointClass, remove),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->cmnd_done_id = g_signal_new (
			"cmnd-done",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserMPointClass, cmnd_done),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);


    parent_class = g_type_class_peek_parent (class);   
	g_type_class_add_private (o_class, sizeof (GHNodeBrowserMPointPrivate));
}

static void
g_hnode_browser_mpoint_init (GHNodeBrowserMPoint *sb)
{
	GHNodeBrowserMPointPrivate *priv;

	priv = G_HNODE_BROWSER_MPOINT_GET_PRIVATE (sb);

    // Init
    priv->MSource  = NULL;
    priv->AddrObj  = NULL; 
    priv->CurIOTag = 0;      

    priv->dispose_has_run = FALSE;

}

/*
static void
g_hnode_browser_set_mpoint_property (GObject 	*object,
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
g_hnode_browser_get_mpoint_property (GObject		*object,
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


GHNodeBrowserMPoint *
g_hnode_browser_mpoint_new (void)
{
	return g_object_new (G_TYPE_HNODE_BROWSER_MPOINT, NULL);
}

gboolean
g_hnode_browser_mpoint_set_address (GHNodeBrowserMPoint *sb, gchar *AddressStr)
{
	GHNodeBrowserMPointPrivate *priv;

	priv = G_HNODE_BROWSER_MPOINT_GET_PRIVATE(sb);

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    // Get a new address object to set.
    priv->AddrObj = g_hnode_address_new();

    g_hnode_address_set_str(priv->AddrObj, AddressStr);

}

static void 
g_hnode_browser_mpoint_rxpacket(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    GHNodeBrowserMPoint *Browser = data;
	GHNodeBrowserMPointPrivate *priv; 
	GHNodeBrowserMPointClass   *class; 

    guint32       ReqLength;
    guint32       ReqType;
    guint32       ReqIOTag;

    GHNodePacket *TxPacket;

    class = G_HNODE_BROWSER_MPOINT_GET_CLASS(Browser);
    priv  = G_HNODE_BROWSER_MPOINT_GET_PRIVATE(Browser);

    // Get the header stuff
    ReqLength = g_hnode_packet_get_uint(Packet);
    ReqType   = g_hnode_packet_get_uint(Packet);
    ReqIOTag  = g_hnode_packet_get_uint(Packet);

    // Parse the packets
    switch( ReqType )
    {
        case HNODE_MGMT_ACK:
        break;

        case HNODE_MGMT_NODE_PKT:

            // Make sure enough data exists.
            if( ReqLength > 12 )
            { 
                GHNodeBrowserHNode *Node;

                // Allocate a new node object.
                Node = g_hnode_browser_hnode_new();

                // Fill in the node information.
                g_hnode_browser_hnode_parse_client_update(Node, Packet);

                // Add a back pointer to the client link.
                g_hnode_browser_hnode_set_mpoint(Node, Browser);

                // Add this object to node list from this mpoint.
                priv->NodeList = g_list_append(priv->NodeList, Node);

                //g_hnode_browser_hnode_debug_print(Node);

                // Tell the upper layer about the new node record.
                g_signal_emit(Browser, class->hnode_add_id, 0, Node);
            }

        break;

        case HNODE_MGMT_RSP_PINFO: 
        break;

        case HNODE_MGMT_DEBUG_PKT:
        {
            GList *CurElem;
            GHNodeBrowserHNode *Node;
            guint32 SrvObjID, DbgType, DbgSeqCnt, EPIndex;
            guint32   DDLength;
            guint8   *DDPtr; 

            // Proceeding fields were all extracted above,
            // so we should already be pointing at the ServObjID
            DbgType   = g_hnode_packet_get_uint(Packet);
            DbgSeqCnt = g_hnode_packet_get_uint(Packet);
            SrvObjID  = g_hnode_packet_get_uint(Packet);
            EPIndex   = g_hnode_packet_get_uint(Packet);

            DDLength = g_hnode_packet_get_uint(Packet);
            DDPtr    = g_hnode_packet_get_offset_ptr(Packet, 0);

            // Scan the nodes to find the one that will emit the
            // debug packet.
            CurElem = g_list_first(priv->NodeList);
            while( CurElem )
            {
                Node = CurElem->data;

                // Notify this client node of the new hnode.
                if( g_hnode_browser_hnode_has_srvobj_id(Node, SrvObjID) )
                {
                    g_hnode_browser_hnode_process_debug_rx(Node, DbgType, DbgSeqCnt, EPIndex, DDLength, DDPtr);
                }

                // Next client node
                CurElem = g_list_next(CurElem);
            }
        }
        break;

        case HNODE_MGMT_EVENT_PKT:
        break;

        case HNODE_MGMT_NONE:
        case HNODE_MGMT_SHUTDOWN:
        case HNODE_MGMT_REQ_PINFO: 
        case HNODE_MGMT_DEBUG_CTRL:
        case HNODE_MGMT_EVENT_CTRL:
        break;
    }

    // Get rid of this packet.
    g_object_unref(Packet);
}

static void 
g_hnode_browser_mpoint_txpacket(GHNodePktSrc *sb, GHNodePacket *Packet, gpointer data)
{
    g_object_unref(Packet);
}

void 
g_hnode_browser_mpoint_start(GHNodeBrowserMPoint *sb)
{
	GHNodeBrowserMPointPrivate *priv;
    GHNodePacket *Packet;

	priv = G_HNODE_BROWSER_MPOINT_GET_PRIVATE(sb);

    // Open up a listener socket source
    priv->MSource = g_hnode_pktsrc_new(PST_HNODE_TCP_HMNODE_LINK);

    g_hnode_pktsrc_use_address_object(priv->MSource, priv->AddrObj);

    g_signal_connect(priv->MSource, "rx_packet", G_CALLBACK(g_hnode_browser_mpoint_rxpacket), sb);
    g_signal_connect(priv->MSource, "tx_packet", G_CALLBACK(g_hnode_browser_mpoint_txpacket), sb);

    g_hnode_pktsrc_start( priv->MSource );

    Packet = g_hnode_packet_new();

    // Write the request length - 12 bytes
    g_hnode_packet_set_uint(Packet, 12);

    // Write the request type - HNODE_MGMT_REQ_PINFO
    g_hnode_packet_set_uint(Packet, HNODE_MGMT_REQ_PINFO);

    // Write the request iotag
    g_hnode_packet_set_uint(Packet, priv->CurIOTag);

    // Bump the IOTag for the next request.
    priv->CurIOTag += 1;
    
	// Send the request
    g_hnode_pktsrc_send_packet(priv->MSource, Packet);

}

gboolean
g_hnode_browser_mpoint_get_address(GHNodeBrowserMPoint *sb, GHNodeAddress **ObjAddr)
{
	GHNodeBrowserMPointPrivate *priv;

	priv = G_HNODE_BROWSER_MPOINT_GET_PRIVATE(sb);

    *ObjAddr = priv->AddrObj;

    return FALSE;
}

void
g_hnode_browser_mpoint_enable_debug(GHNodeBrowserMPoint *sb)
{
	GHNodeBrowserMPointPrivate *priv;
    GHNodePacket        *Packet;

	priv = G_HNODE_BROWSER_MPOINT_GET_PRIVATE(sb);

    Packet = g_hnode_packet_new();

    // Write the request length - 12 bytes
    g_hnode_packet_set_uint(Packet, 16);

    // Write the request type - HNODE_MGMT_REQ_PINFO
    g_hnode_packet_set_uint(Packet, HNODE_MGMT_DEBUG_CTRL);

    // Write the request iotag
    g_hnode_packet_set_uint(Packet, priv->CurIOTag);

    // Bump the IOTag for the next request.
    priv->CurIOTag += 1;

    // Indicate we want to enable
    g_hnode_packet_set_uint(Packet, 1);
    
	// Send the request
    g_hnode_pktsrc_send_packet(priv->MSource, Packet);

    return;
}
















