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

#ifndef __G_HNODE_NODEOBJ_H__
#define __G_HNODE_NODEOBJ_H__

#ifdef __cplusplus
extern "C" {
#endif

G_BEGIN_DECLS

#define G_TYPE_HNODE			(g_hnode_get_type ())
#define G_HNODE(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE, GHNode))
#define G_HNODE_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE, GHNodeClass))

typedef struct _GHNode		GHNode;
typedef struct _GHNodeClass	GHNodeClass;
typedef struct _GHNodeEvent	GHNodeEvent;

struct _GHNode
{
	GObject parent;
};

struct _GHNodeClass
{
	GObjectClass parent_class;

	/* Signal handlers */
	void (* state_change)   (GHNode *sb, gpointer event);
	void (* config_rx)	    (GHNode *sb, gpointer event);

    guint32 state_change_id;
    guint32 config_rx_id;
};

struct _GHNodeEvent
{
	guint32          ObjID;
    guint8          *PktPtr;
    guint32          PktLength;
};


GHNode *g_hnode_new (void);

void g_hnode_start(GHNode *sb);
void g_hnode_send_debug_frame(GHNode *HNode, guint32 Type, guint32 EPIndex, guint32 DebugDataLength, guint8 *DebugDataPtr);
void g_hnode_send_event_frame(GHNode *HNode, guint32 EventID, guint32 EPIndex, guint32 EventParamLength, guint8 *EventParamDataPtr);

void g_hnode_set_name_prefix(GHNode  *HNode, guint8  *NamePrefix);

void g_hnode_set_version(GHNode *HNode, guint8 MajorVersion, guint8 MinorVersion, guint16 MicroVersion );

void g_hnode_set_uid(GHNode *HNode, guint8 *UIDBuffer);

void g_hnode_enable_config_support(GHNode *HNode);

void g_hnode_set_endpoint_count(GHNode *HNode, guint16 EndPointCount);

void g_hnode_set_endpoint(GHNode  *HNode, 
                     guint16  EndPointIndex,
                     guint16  AssociateEPIndex,
	                 guint8  *MimeTypeStr,	
	                 guint16  Port,
	                 guint8   MajorVersion,
	                 guint8   MinorVersion,
	                 guint16  MicroVersion);

void g_hnode_send_config_frame(GHNode  *HNode, GHNodePacket *TxPacket);

G_END_DECLS

#ifdef __cplusplus
}
#endif

#endif
