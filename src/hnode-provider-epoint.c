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
#include "hnode-provider.h"

#define G_HNODE_ENDPOINT_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_ENDPOINT, GHNodeEndPointPrivate))

G_DEFINE_TYPE (GHNodeEndPoint, g_hnode_endpoint, G_TYPE_OBJECT);

typedef struct _GHNodeEndPointPrivate GHNodeEndPointPrivate;
struct _GHNodeEndPointPrivate
{
    GHNodeAddress *AddrObj;

	guint16   MimeTypeLength;
	guint8    MimeTypeStr[128];	
//	guint16   Port;

	guint16   AssociateEPIndex;

	guint8    MajorVersion;
	guint8    MinorVersion;
	guint16   MicroVersion;

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
    STATE_EVENT,
	LAST_SIGNAL
};

static guint g_hnode_endpoint_signals[LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;

static void
g_hnode_endpoint_dispose (GObject *obj)
{
    GHNodeEndPoint          *self = (GHNodeEndPoint *)obj;
    GHNodeEndPointPrivate   *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(self);

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


    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->dispose (obj);
}

static void
g_hnode_endpoint_finalize (GObject *obj)
{
    GHNodeEndPoint *self = (GHNodeEndPoint *)obj;
    
    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_endpoint_class_init (GHNodeEndPointClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_endpoint_dispose;
    o_class->finalize = g_hnode_endpoint_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	//g_hnode_server_signals[STATE_EVENT] = g_signal_new (
	//		"state_change",
	//		G_OBJECT_CLASS_TYPE (o_class),
	//		G_SIGNAL_RUN_FIRST,
	//		G_STRUCT_OFFSET (GHNodeServerClass, hnode_found),
	//		NULL, NULL,
	//		g_cclosure_marshal_VOID__POINTER,
	//		G_TYPE_NONE,
	//		1, G_TYPE_POINTER);

    parent_class = g_type_class_peek_parent (class);      
	g_type_class_add_private (o_class, sizeof (GHNodeEndPointPrivate));
}

static void
g_hnode_endpoint_init (GHNodeEndPoint *sb)
{
	GHNodeEndPointPrivate *priv;
    //GHNodeServerSource  *NodeSource;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE (sb);
        
    // Initilize the ObjID value
	priv->MimeTypeLength    = 0;
	priv->MimeTypeStr[0]    = '\0';	
	//priv->Port              = 0;

    priv->AddrObj           = NULL;

	priv->AssociateEPIndex  = 0;

	priv->MajorVersion      = 0;
	priv->MinorVersion      = 0;
	priv->MicroVersion      = 0;

    priv->dispose_has_run = FALSE;

}

GHNodeEndPoint *
g_hnode_endpoint_new (void)
{
	return g_object_new (G_TYPE_HNODE_ENDPOINT, NULL);
}

gboolean
g_hnode_endpoint_parse_rx_packet(GHNodeEndPoint *sb, GHNodeAddress *NodeAddrObj, GHNodePacket *Packet)
{
	GHNodeEndPointPrivate *priv;
    guint16  Port;
    gchar   *AddrStr;
    guint32  AddrStrLen;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    // Duplicate the node address.
    if( priv->AddrObj == NULL )
    {
        priv->AddrObj = g_hnode_address_new();
    }

    // Copy the address object.
    g_hnode_address_get_str_ptr(NodeAddrObj, &AddrStr, &AddrStrLen);
    g_hnode_address_set_str(priv->AddrObj, AddrStr );

    // Update the port reference for this endpoint
    Port = g_hnode_packet_get_short(Packet);

    g_hnode_address_set_port(priv->AddrObj, Port);

    g_hnode_packet_skip_bytes(Packet, 2);      // Reserved Bytes
 
    priv->MicroVersion = g_hnode_packet_get_short(Packet);
    priv->MinorVersion = g_hnode_packet_get_char(Packet);
    priv->MajorVersion = g_hnode_packet_get_char(Packet);

    priv->AssociateEPIndex = g_hnode_packet_get_uint(Packet);

    priv->MimeTypeLength = g_hnode_packet_get_uint(Packet);
    g_hnode_packet_get_bytes(Packet, &priv->MimeTypeStr, priv->MimeTypeLength);

}

void 
g_hnode_endpoint_build_client_update(GHNodeEndPoint *sb, GHNodePacket *Packet)
{
	GHNodeEndPointPrivate *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    g_hnode_packet_set_short(Packet, g_hnode_address_GetPort(priv->AddrObj));
    
    g_hnode_packet_set_short(Packet, priv->MicroVersion);
    g_hnode_packet_set_char(Packet, priv->MinorVersion);
    g_hnode_packet_set_char(Packet, priv->MajorVersion);

    g_hnode_packet_set_uint(Packet, priv->AssociateEPIndex);

    g_hnode_packet_set_uint(Packet, priv->MimeTypeLength);
    g_hnode_packet_set_bytes(Packet, priv->MimeTypeStr, priv->MimeTypeLength);

}

void 
g_hnode_endpoint_parse_client_update(GHNodeEndPoint *sb, GHNodeAddress *NodeAddrObj, GHNodePacket *Packet)
{
	GHNodeEndPointPrivate *priv;
    guint16  Port;
    gchar   *AddrStr;
    guint32  AddrStrLen;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    // Duplicate the node address.
    if( priv->AddrObj == NULL )
    {
        priv->AddrObj = g_hnode_address_new();
        
        g_hnode_address_get_str_ptr(NodeAddrObj, &AddrStr, &AddrStrLen);
        g_hnode_address_set_str(priv->AddrObj, AddrStr );
    }

    // Update the port reference for this endpoint
    Port = g_hnode_packet_get_short(Packet);

    g_hnode_address_set_port(priv->AddrObj, Port);
    
    priv->MicroVersion = g_hnode_packet_get_short(Packet);
    priv->MinorVersion = g_hnode_packet_get_char(Packet);
    priv->MajorVersion = g_hnode_packet_get_char(Packet);

    priv->AssociateEPIndex = g_hnode_packet_get_uint(Packet);

    priv->MimeTypeLength = g_hnode_packet_get_uint(Packet);
    g_hnode_packet_get_bytes(Packet, &priv->MimeTypeStr, priv->MimeTypeLength);

}

gboolean
g_hnode_endpoint_get_address(GHNodeEndPoint *sb, GHNodeAddress **EPAddr)
{
	GHNodeEndPointPrivate *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    *EPAddr = priv->AddrObj;

    return FALSE;
}

gboolean
g_hnode_endpoint_get_version(GHNodeEndPoint *sb, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion)
{
	GHNodeEndPointPrivate *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    *MicroVersion = priv->MicroVersion;
    *MinorVersion = priv->MinorVersion;
    *MajorVersion = priv->MajorVersion;

    return FALSE;
}

gboolean
g_hnode_endpoint_get_associate_index(GHNodeEndPoint *sb, guint32 *EPAssocIndex)   
{
	GHNodeEndPointPrivate *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    *EPAssocIndex = priv->AssociateEPIndex;

    return FALSE;
}

gboolean
g_hnode_endpoint_get_type_str(GHNodeEndPoint *sb, gchar **EPTypeStrPtr)
{
	GHNodeEndPointPrivate *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    *EPTypeStrPtr = priv->MimeTypeStr;

    return FALSE;
}

gboolean
g_hnode_endpoint_supports_interface(GHNodeEndPoint *Node, gchar *EPTypeStrPtr)
{
	GHNodeEndPointPrivate *priv;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE (Node);

    // Check for a interface type string match
    if( g_strcmp0(priv->MimeTypeStr, EPTypeStrPtr) == 0 )
        return TRUE;

    // Did not match
    return FALSE;
}

void
g_hnode_endpoint_debug_print(GHNodeEndPoint *sb)
{
	GHNodeEndPointPrivate *priv;
    guint32          AddrStrLen;
    guint8          *AddrStr;

	priv = G_HNODE_ENDPOINT_GET_PRIVATE(sb);

    g_message("Port: %d\n", g_hnode_address_GetPort(priv->AddrObj));   
    g_message("Version: %d.%d.%d\n", priv->MajorVersion, priv->MinorVersion, priv->MicroVersion);   
    g_message("Associated End Point Index: %d\n", priv->AssociateEPIndex);
    g_message("Mime Type Str: %s\n", priv->MimeTypeStr);   

}


