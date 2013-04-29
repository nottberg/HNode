#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <string.h>
#include <stdio.h>
#include <glib.h>
 
#include "hnode-uid.h"

/*************************************************************************************************************************
*
*
* UID Object
*
*
*************************************************************************************************************************/

#define G_HNODE_UID_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_UID, GHNodeUIDPrivate))

G_DEFINE_TYPE (GHNodeUID, g_hnode_uid, G_TYPE_OBJECT);

typedef struct _GHNodeUIDPrivate GHNodeUIDPrivate;
struct _GHNodeUIDPrivate
{
    // 16-byte Identifier
    gint      Id[4];

    gboolean  IdValid;         
    gboolean  dispose_has_run;
};

static GObjectClass *parent_class = NULL;

static void
g_hnode_uid_dispose (GObject *obj)
{
    GHNodeUID          *self = (GHNodeUID *)obj;
    GHNodeUIDPrivate   *priv;

	priv  = G_HNODE_UID_GET_PRIVATE(self);

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
g_hnode_uid_finalize (GObject *obj)
{
    GHNodeUID *self = (GHNodeUID *)obj;
    GHNodeUIDClass     *class;

    class = G_HNODE_UID_GET_CLASS(self);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_uid_class_init (GHNodeUIDClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_uid_dispose;
    o_class->finalize = g_hnode_uid_finalize;

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
	g_type_class_add_private (o_class, sizeof (GHNodeUIDPrivate));
}

static void
g_hnode_uid_init (GHNodeUID *sb)
{
	GHNodeUIDPrivate *priv;

	priv = G_HNODE_UID_GET_PRIVATE (sb);
         
    // Initilize the ObjID value
    priv->Id[0] = 0;
    priv->Id[1] = 0;
    priv->Id[2] = 0;
    priv->Id[3] = 0;

    priv->IdValid = FALSE;

    priv->dispose_has_run = FALSE;

}

static guint8 *
g_hnode_uid_get_uid_ptr( GHNodeUID *UIDObj )
{
	GHNodeUIDPrivate *priv;

	priv = G_HNODE_UID_GET_PRIVATE(UIDObj);

    if( priv->IdValid )
        return priv->Id;
    else
        return NULL; 
}

GHNodeUID *
g_hnode_uid_new (void)
{
	return g_object_new (G_TYPE_HNODE_UID, NULL);
}

void
g_hnode_uid_set_uid( GHNodeUID *UIDObj, guint8 *UIDData )
{
	GHNodeUIDPrivate *priv;

	priv = G_HNODE_UID_GET_PRIVATE(UIDObj);

    memcpy((guint8 *)priv->Id, UIDData, 16);

    priv->IdValid = TRUE;
}

gboolean
g_hnode_uid_get_uid( GHNodeUID *UIDObj, guint *UIDData )
{
	GHNodeUIDPrivate *priv;

	priv = G_HNODE_UID_GET_PRIVATE(UIDObj);

    if( priv->IdValid )
        memcpy(UIDData, (guint8 *)priv->Id, 16);

    return priv->IdValid; 
}

gboolean
g_hnode_uid_is_equal( GHNodeUID *UIDObj, GHNodeUID *CmpUIDObj )
{
	GHNodeUIDPrivate *priv;
    guint8 *CmpData;
    guint i;

	priv = G_HNODE_UID_GET_PRIVATE(UIDObj);

    // check if our data is valid
    if( priv->IdValid == FALSE )
        return FALSE;

    // check is the compare object is valid and get
    // a pointer to it's data to allow a comparison.
    CmpData = g_hnode_uid_get_uid_ptr(CmpUIDObj);

    if(CmpData == NULL)
        return FALSE;

    // Check for match
    if( memcmp(CmpData, priv->Id, 16) == 0 )
        return TRUE;

    // Match Failed
    return FALSE;
}

gboolean
g_hnode_uid_set_uid_from_string( GHNodeUID *UIDObj, gchar *UIDStr )
{
	GHNodeUIDPrivate *priv;
    gchar**  Tokens;
    guint    i;

	priv = G_HNODE_UID_GET_PRIVATE(UIDObj);

    // Init the token variable
    Tokens = NULL;

    // Get rid of any starting or ending whitespace
    g_strstrip(UIDStr);    

    // Make sure the start of the string has the expected format
    if( g_ascii_strncasecmp( UIDStr, "hnode_", 6) != 0 )
        return TRUE;

    // Tokenize the remainder of the string with a ":" delimiter.
    Tokens = g_strsplit( &UIDStr[6], ":", 17);

    for(i = 0; i <= 15; i++)
    {
        // Make sure there is something to convert
        if(    (Tokens[i] == NULL) 
            || (Tokens[i][2] != NULL)  
            || (g_ascii_isxdigit(Tokens[i][0]) == FALSE)
            || (g_ascii_isxdigit(Tokens[i][1]) == FALSE) )
        {
            // Error the conversion
            g_strfreev(Tokens);
            priv->IdValid = FALSE;
            return TRUE;
        }

        // Convert the current string to a number.
        ((guint8 *)priv->Id)[i] = strtol( Tokens[i], NULL, 16 ); 

    }

    // Successful conversion
    g_strfreev(Tokens);
    priv->IdValid = TRUE;
    return FALSE;
}

gboolean
g_hnode_uid_get_uid_as_string( GHNodeUID *UIDObj, gchar *UIDStr )
{
	GHNodeUIDPrivate *priv;
    guint8 *TmpPtr;

	priv = G_HNODE_UID_GET_PRIVATE(UIDObj);

    if( priv->IdValid )
    {
        TmpPtr = priv->Id;

        g_sprintf(UIDStr, "hnode_%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx:%2.2hhx",
                            TmpPtr[0], TmpPtr[1], TmpPtr[2], TmpPtr[3], TmpPtr[4], TmpPtr[5], TmpPtr[6], TmpPtr[7],
                            TmpPtr[8], TmpPtr[9], TmpPtr[10], TmpPtr[11], TmpPtr[12], TmpPtr[13], TmpPtr[14], TmpPtr[15]);

    }

    return priv->IdValid; 
}


