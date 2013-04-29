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
#include "hnode-uid.h"
#include "hnode-provider.h"
#include "hnode-browser.h"



#define G_HNODE_BROWSER_HNODE_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_BROWSER_HNODE, GHNodeBrowserHNodePrivate))

G_DEFINE_TYPE (GHNodeBrowserHNode, g_hnode_browser_hnode, G_TYPE_OBJECT);

typedef struct _GHNodeBrowserHNodePriv GHNodeBrowserHNodePrivate;
struct _GHNodeBrowserHNodePriv
{
    GHNodeBrowserMPoint *MPoint;
    guint32              MPointNodeID;  

    GHNodeProvider      *Provider;

    gboolean             dispose_has_run;

};

extern void g_cclosure_user_marshal_VOID__UINT_UINT_UINT_UINT_POINTER (GClosure     *closure,
                                                                       GValue       *return_value,
                                                                       guint         n_param_values,
                                                                       const GValue *param_values,
                                                                       gpointer      invocation_hint,
                                                                       gpointer      marshal_data);
/* GObject callbacks */

static GObjectClass *parent_class = NULL;

static void
g_hnode_browser_hnode_dispose (GObject *obj)
{
    GHNodeBrowserHNode          *self = (GHNodeBrowserHNode *)obj;
    GHNodeBrowserHNodePrivate   *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE(self);

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
g_hnode_browser_hnode_finalize (GObject *obj)
{
    GHNodeBrowserHNode *self = (GHNodeBrowserHNode *)obj;
    
    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_browser_hnode_class_init (GHNodeBrowserHNodeClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_browser_hnode_dispose;
    o_class->finalize = g_hnode_browser_hnode_finalize;

	/*
	o_class->set_property = g_hnode_browser_set_property;
	o_class->get_property = g_hnode_browser_get_property;
	*/
	
	/* create signals */
	class->update_id = g_signal_new (
			"update",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserHNodeClass, update),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->debug_rx_id = g_signal_new (
			"debug-rx",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserHNodeClass, debug_rx),
			NULL, NULL,
			g_cclosure_user_marshal_VOID__UINT_UINT_UINT_UINT_POINTER,
			G_TYPE_NONE,
			5, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_UINT, G_TYPE_POINTER);

	class->event_rx_id = g_signal_new (
			"event-rx",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserHNodeClass, event_rx),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

	class->remove_id = g_signal_new (
			"remove",
			G_OBJECT_CLASS_TYPE (o_class),
			G_SIGNAL_RUN_FIRST,
			G_STRUCT_OFFSET (GHNodeBrowserHNodeClass, remove),
			NULL, NULL,
			g_cclosure_marshal_VOID__POINTER,
			G_TYPE_NONE,
			1, G_TYPE_POINTER);

    parent_class = g_type_class_peek_parent (class);   
	g_type_class_add_private (o_class, sizeof (GHNodeBrowserHNodePrivate));
}

static void
g_hnode_browser_hnode_init (GHNodeBrowserHNode *sb)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (sb);

    // Init
    priv->MPoint       = NULL;
    priv->Provider     = NULL; 
    priv->MPointNodeID = 0;      

    priv->dispose_has_run = FALSE;

}

GHNodeBrowserHNode *
g_hnode_browser_hnode_new (void)
{
	return g_object_new (G_TYPE_HNODE_BROWSER_HNODE, NULL);
}

void
g_hnode_browser_hnode_set_mpoint (GHNodeBrowserHNode *sb, GHNodeBrowserMPoint *MPoint)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (sb);

    // A pointer back to the parent client
    priv->MPoint = MPoint;
}

void
g_hnode_browser_hnode_parse_client_update (GHNodeBrowserHNode *sb, GHNodePacket *Packet)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (sb);

    // Grab a uint which will be the MPointNodeID
    priv->MPointNodeID = g_hnode_packet_get_uint(Packet);

    // Allocate a Provider if needed.
    if(priv->Provider == NULL)
    {
        priv->Provider = g_hnode_provider_new();
    }

    // Set the provider fields from the packet contents
    g_hnode_provider_parse_client_update(priv->Provider, Packet);

}

gboolean
g_hnode_browser_hnode_get_address(GHNodeBrowserHNode *Node, GHNodeAddress **ObjAddr)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_get_address(priv->Provider, ObjAddr);

}

gboolean
g_hnode_browser_hnode_get_version(GHNodeBrowserHNode *Node, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_get_version(priv->Provider, MajorVersion, MinorVersion, MicroVersion);

}


gboolean
g_hnode_browser_hnode_get_uid(GHNodeBrowserHNode *Node, guint8 *UIDArray)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_get_uid(priv->Provider, UIDArray);

}

gboolean
g_hnode_browser_hnode_get_endpoint_count(GHNodeBrowserHNode *Node, guint32 *EPCount)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_get_endpoint_count(priv->Provider, EPCount);

}

gboolean
g_hnode_browser_hnode_endpoint_get_address(GHNodeBrowserHNode *Node, guint32 EPIndx, GHNodeAddress **EPAddr)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_endpoint_get_address(priv->Provider, EPIndx, EPAddr);

}

gboolean
g_hnode_browser_hnode_endpoint_get_version(GHNodeBrowserHNode *Node, guint32 EPIndx, guint32 *MajorVersion, guint32 *MinorVersion, guint32 *MicroVersion)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_endpoint_get_version(priv->Provider, EPIndx, MajorVersion, MinorVersion, MicroVersion);

}

gboolean
g_hnode_browser_hnode_endpoint_get_associate_index(GHNodeBrowserHNode *Node, guint32 EPIndx, guint32 *EPAssocIndex)   
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_endpoint_get_associate_index(priv->Provider, EPIndx, EPAssocIndex);

}

gboolean
g_hnode_browser_hnode_endpoint_get_type_str(GHNodeBrowserHNode *Node, guint32 EPIndx, gchar **EPTypeStrPtr)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_endpoint_get_type_str(priv->Provider, EPIndx, EPTypeStrPtr);

}

gboolean
g_hnode_browser_hnode_supports_interface(GHNodeBrowserHNode *Node, gchar *EPTypeStrPtr, guint32 *EPIndex)
{
	GHNodeBrowserHNodePrivate *priv;
    guint i;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    // Check if this hnode supports an interface.
    return g_hnode_provider_supports_interface( priv->Provider, EPTypeStrPtr, EPIndex );
}

GHNodeUID *
g_hnode_browser_hnode_get_uid_objptr(GHNodeBrowserHNode *Node)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_get_uid_objptr(priv->Provider);

}

gboolean
g_hnode_browser_hnode_is_uid_equal(GHNodeBrowserHNode *Node, GHNodeUID *CmpUID)
{	
    GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_compare_uid(priv->Provider, CmpUID);

}

gboolean
g_hnode_browser_hnode_has_srvobj_id(GHNodeBrowserHNode *Node, guint32 SrvObjID)
{
    GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_has_srvobj_id(priv->Provider, SrvObjID);
}

gboolean
g_hnode_browser_hnode_supports_config(GHNodeBrowserHNode *Node)
{
    GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_supports_config(priv->Provider);
}

gboolean
g_hnode_browser_hnode_get_config_address(GHNodeBrowserHNode *Node, GHNodeAddress **ConfigAddr)
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    return g_hnode_provider_get_config_address(priv->Provider, ConfigAddr);

}

void
g_hnode_browser_hnode_process_debug_rx(GHNodeBrowserHNode *Node, 
                                        guint32 DbgType, 
                                        guint32 DbgSeqCnt, 
                                        guint32 EPIndex, 
                                        guint32 DDLength, 
                                        guint8 *DDPtr)
{
	GHNodeBrowserHNodePrivate *priv;
	GHNodeBrowserHNodeClass   *class; 

    class = G_HNODE_BROWSER_HNODE_GET_CLASS(Node);
	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (Node);

    // Tell the upper layer about the new node record.
    g_signal_emit(Node, class->debug_rx_id, 0, DbgType, DbgSeqCnt, EPIndex, DDLength, DDPtr);

}

/*
void
g_hnode_browser_hnode_get_core_info (GHNodeBrowserHNode *sb, )
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (sb);

}

void
g_hnode_browser_hnode_get_endpoint_info (GHNodeBrowserHNode *sb, )
{
	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (sb);

}
*/
void
g_hnode_browser_hnode_debug_print(GHNodeBrowserHNode *sb)
{
 	GHNodeBrowserHNodePrivate *priv;

	priv = G_HNODE_BROWSER_HNODE_GET_PRIVATE (sb);

    g_message("MPointNodeID: 0x%x\n", priv->MPointNodeID);   

    if( priv->Provider )
    {
        g_hnode_provider_debug_print(priv->Provider);
    }
    else
    {
        g_message("Provider: NULL\n");
    }
}

#define g_marshal_value_peek_uint(v)     (v)->data[0].v_uint
#define g_marshal_value_peek_pointer(v)  (v)->data[0].v_pointer

void
g_cclosure_user_marshal_VOID__UINT_UINT_UINT_UINT_POINTER (GClosure     *closure,
                                                           GValue       *return_value,
                                                           guint         n_param_values,
                                                           const GValue *param_values,
                                                           gpointer      invocation_hint,
                                                           gpointer      marshal_data)
{
  typedef void (*GMarshalFunc_VOID__UINT_UINT_UINT_UINT_POINTER) (gpointer     data1,
                                                                  guint        arg_1,
                                                                  guint        arg_2,
                                                                  guint        arg_3,
                                                                  guint        arg_4,
                                                                  gpointer     arg_5,
                                                                  gpointer     data2);
  register GMarshalFunc_VOID__UINT_UINT_UINT_UINT_POINTER callback;
  register GCClosure *cc = (GCClosure*) closure;
  register gpointer data1, data2;

  g_return_if_fail (n_param_values == 6);

  if (G_CCLOSURE_SWAP_DATA (closure))
    {
      data1 = closure->data;
      data2 = g_value_peek_pointer (param_values + 0);
    }
  else
    {
      data1 = g_value_peek_pointer (param_values + 0);
      data2 = closure->data;
    }
  callback = (GMarshalFunc_VOID__UINT_UINT_UINT_UINT_POINTER) (marshal_data ? marshal_data : cc->callback);

  callback (data1,
            g_marshal_value_peek_uint (param_values + 1),
            g_marshal_value_peek_uint (param_values + 2),
            g_marshal_value_peek_uint (param_values + 3),
            g_marshal_value_peek_uint (param_values + 4),
            g_marshal_value_peek_pointer (param_values + 5),
            data2);
}

