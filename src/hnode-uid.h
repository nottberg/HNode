#ifndef __G_HNODE_UID_H__
#define __G_HNODE_UID_H__

#include <glib.h>
#include <glib-object.h>

G_BEGIN_DECLS

#define G_TYPE_HNODE_UID		    (g_hnode_uid_get_type ())
#define G_HNODE_UID(obj)			(G_TYPE_CHECK_INSTANCE_CAST ((obj), G_TYPE_HNODE_UID, GHNodeUID))
#define G_HNODE_UID_GET_CLASS(obj)	(G_TYPE_INSTANCE_GET_CLASS ((obj), G_TYPE_HNODE_UID, GHNodeUIDClass))

typedef struct _GHNodeUID		GHNodeUID;
typedef struct _GHNodeUIDClass	GHNodeUIDClass;
//typedef struct _GHNodeAddressEvent  GHNodeAddressEvent;

struct _GHNodeUID
{
	GObject parent;
};

struct _GHNodeUIDClass
{
	GObjectClass parent_class;

};

GHNodeUID *g_hnode_uid_new (void);

void g_hnode_uid_set_uid( GHNodeUID *UIDObj, guint8 *UIDData );
gboolean g_hnode_uid_get_uid( GHNodeUID *UIDObj, guint *UIDData );
gboolean g_hnode_uid_is_equal( GHNodeUID *UIDObj, GHNodeUID *CmpUIDObj );
gboolean g_hnode_uid_set_uid_from_string( GHNodeUID *UIDObj, gchar *UIDStr );
gboolean g_hnode_uid_get_uid_as_string( GHNodeUID *UIDObj, gchar *UIDStr );

G_END_DECLS

#endif // _G_HNODE_UID_H_

