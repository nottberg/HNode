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

/*************************************************************************************************************************
*
*
* Packet Object
*
*
*************************************************************************************************************************/

#define G_HNODE_PACKET_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_PACKET, GHNodePacketPrivate))

G_DEFINE_TYPE (GHNodePacket, g_hnode_packet, G_TYPE_OBJECT);

typedef struct _GHNodePacketPrivate GHNodePacketPrivate;
struct _GHNodePacketPrivate
{
    gint                 Type;

    gint                 BufLength;
    gint                 DataLength; 
    gint                 CurOffset;
    guint8              *Data;
    
    GHNodeAddress       *AddrObj;

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

static guint g_hnode_packet_signals[LAST_SIGNAL] = { 0 };

static GObjectClass *parent_class = NULL;

static void
g_hnode_packet_dispose (GObject *obj)
{
    GHNodePacket          *self = (GHNodePacket *)obj;
    GHNodePacketPrivate   *priv;

	priv  = G_HNODE_PACKET_GET_PRIVATE(self);

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
g_hnode_packet_finalize (GObject *obj)
{
    GHNodePacket *self = (GHNodePacket *)obj;
    GHNodePacketClass     *class;

    class = G_HNODE_PACKET_GET_CLASS(self);

    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_packet_class_init (GHNodePacketClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_packet_dispose;
    o_class->finalize = g_hnode_packet_finalize;

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
	g_type_class_add_private (o_class, sizeof (GHNodePacketPrivate));
}

static void
g_hnode_packet_init (GHNodePacket *sb)
{
	GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE (sb);
         
    // Initilize the ObjID value
    priv->Type       = 0;
    priv->BufLength  = 0;
    priv->DataLength = 0;

    priv->Data       = NULL;

    priv->CurOffset  = 0;

    priv->AddrObj    = NULL;

    priv->dispose_has_run = FALSE;

}

GHNodePacket *
g_hnode_packet_new (void)
{
	return g_object_new (G_TYPE_HNODE_PACKET, NULL);
}

//void
//g_hnode_packet_free(GHNodePacket *Packet)
//{
//	g_object_unref(Packet);
//}

guint8 *
g_hnode_packet_get_buffer_ptr(GHNodePacket *Packet, guint32 TotalLength)
{
    GHNodePacketPrivate *priv;
    guint8 *TmpBuffer;
    
	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // If we already have enough space then we don't need 
    // to allocate a bigger buffer
    if( TotalLength <= priv->BufLength )
        return priv->Data;

    // Do Allocation in 512 byte increments; so round
    // the total length up to the next 512 byte boundary.
    if( TotalLength & 0x1FF )
        TotalLength = (TotalLength & 0xFFFFFE00) + 0x200;

    // Allocate a bigger buffer and copy pre-existing
    // data to the new buffer.  Then de-allocate the old
    // buffer.
    TmpBuffer = g_try_malloc(TotalLength);
    if(TmpBuffer == NULL)
        return NULL;


    if( priv->DataLength )
        memcpy(TmpBuffer, priv->Data, priv->DataLength);

    if( priv->DataLength )
        g_free(priv->Data);

    priv->Data      = TmpBuffer;
    priv->BufLength = TotalLength;

    return priv->Data;
}

GHNodeAddress *
g_hnode_packet_get_setable_address_object(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;

	priv  = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    // Get a new address object to set.
    priv->AddrObj = g_hnode_address_new();

    return priv->AddrObj;
}

GHNodeAddress *
g_hnode_packet_get_address_object(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;

	priv  = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Get a new address object to set.
    if(priv->AddrObj == NULL)
        priv->AddrObj = g_hnode_address_new();

    return priv->AddrObj;
}


void
g_hnode_packet_assign_addrobj(GHNodePacket *Packet, GHNodeAddress *AddrObj )
{
    GHNodePacketPrivate *priv;
    gchar   *AddrStr;
    guint32 AddrStrLen;

	priv  = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    priv->AddrObj = g_object_ref(AddrObj);

}

gboolean
g_hnode_packet_clone_address(GHNodePacket *Packet, GHNodePacket *TemplatePacket)
{
    GHNodePacketPrivate *priv;
    GHNodeAddress       *addrObj;
    gchar   *AddrStr;
    guint32 AddrStrLen;

	priv  = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // If an existing address; then free it.
    if(priv->AddrObj)
        g_object_unref(priv->AddrObj);

    addrObj       = g_hnode_packet_get_address_object(TemplatePacket);
    priv->AddrObj = g_object_ref(addrObj);

    return FALSE;
}

/*

void
g_hnode_packet_set_address(GHNodePacket *Packet, gchar *AddrStr )
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    g_hnode_packet_get_setable_address_object(Packet);

    g_hnode_address_set_str(priv->AddrObj, AddrStr);

}

gboolean
g_hnode_packet_set_ipv4_address(GHNodePacket *Packet, struct sockaddr_in *Addr, guint32 AddrLen)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    g_hnode_packet_get_setable_address_object(Packet);

    g_hnode_address_set_ipv4_address(priv->AddrObj, Addr, AddrLen);

    return FALSE;
}

void
g_hnode_packet_get_address_string_ptr(GHNodePacket *Packet, gchar **AddrStr, guint32 *AddrStrLen)    
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    g_hnode_address_get_str_ptr(priv->AddrObj, AddrStr, AddrStrLen);

}

gboolean
g_hnode_packet_get_ipv4_address(GHNodePacket *Packet, struct sockaddr_in *Addr, guint32 *AddrLen)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    g_hnode_address_get_ipv4_address(priv->AddrObj, Addr, AddrLen);

    return FALSE;
}
*/





gboolean
g_hnode_packet_get_payload_ptr(GHNodePacket *Packet, guint8 **PayloadPtr, guint32 *PayloadLength)
{
    GHNodePacketPrivate *priv;
    
	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    *PayloadPtr    = priv->Data;
    *PayloadLength = priv->DataLength;

    return FALSE;
}

guint8 *
g_hnode_packet_get_offset_ptr(GHNodePacket *Packet, guint32 TotalLength)
{
    GHNodePacketPrivate *priv;
    
	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    if( g_hnode_packet_get_buffer_ptr(Packet, TotalLength) == NULL )
        return NULL;

    return (priv->Data+priv->CurOffset);
}

gboolean
g_hnode_packet_increment_data_length(GHNodePacket *Packet, guint32 DataLength)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    priv->DataLength += DataLength;

    return FALSE;
}

guint32
g_hnode_packet_get_data_length(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    return priv->DataLength;
}

void
g_hnode_packet_reset(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Move the current offset back to the beginning.
    priv->CurOffset = 0;
}



void
g_hnode_update_data_length(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Copy the address string.
    if(priv->CurOffset > priv->DataLength)
        priv->DataLength = priv->CurOffset;
}

gboolean
g_hnode_packet_set_uint(GHNodePacket *Packet, guint32 Value)
{
    GHNodePacketPrivate *priv;
    guint8 *DataPtr;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Make sure there is enough room to insert a guint32
    DataPtr = g_hnode_packet_get_buffer_ptr(Packet, priv->CurOffset+4);

    if(DataPtr == NULL)
        return TRUE;

    DataPtr[priv->CurOffset+3] = (Value & 0xFF);
    Value >>= 8;
    DataPtr[priv->CurOffset+2] = (Value & 0xFF);
    Value >>= 8;
    DataPtr[priv->CurOffset+1] = (Value & 0xFF);
    Value >>= 8;
    DataPtr[priv->CurOffset+0] = (Value & 0xFF);

    priv->CurOffset += 4;

    g_hnode_update_data_length( Packet );

    return FALSE;
}

guint32
g_hnode_packet_get_uint(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;
    guint32 TmpVal;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );
    
    TmpVal = 0;
     
    TmpVal = priv->Data[priv->CurOffset];
    TmpVal <<= 8;
    TmpVal |= priv->Data[priv->CurOffset+1];
    TmpVal <<= 8;
    TmpVal |= priv->Data[priv->CurOffset+2];
    TmpVal <<= 8;
    TmpVal |= priv->Data[priv->CurOffset+3];

    priv->CurOffset += 4;
                      
    return TmpVal;
}

gboolean
g_hnode_packet_set_short(GHNodePacket *Packet, guint16 Value)
{
    GHNodePacketPrivate *priv;
    guint8 *DataPtr;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Make sure there is enough room to insert a guint16
    DataPtr = g_hnode_packet_get_buffer_ptr(Packet, priv->CurOffset+2);

    if(DataPtr == NULL)
        return TRUE;

    DataPtr[priv->CurOffset+1] = (Value & 0xFF);
    Value >>= 8;
    DataPtr[priv->CurOffset] = (Value & 0xFF);

    priv->CurOffset += 2;

    g_hnode_update_data_length( Packet );

    return FALSE;
}

guint16
g_hnode_packet_get_short(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;
    guint16 TmpVal;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    TmpVal = 0;
     
    TmpVal = priv->Data[priv->CurOffset];
    TmpVal <<= 8;
    TmpVal |= priv->Data[priv->CurOffset+1];

    priv->CurOffset += 2;
        
    return TmpVal;
}

gboolean
g_hnode_packet_set_char(GHNodePacket *Packet, guint8 Value)
{
    GHNodePacketPrivate *priv;
    guint8 *DataPtr;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Make sure there is enough room to insert a guint32
    DataPtr = g_hnode_packet_get_buffer_ptr(Packet, priv->CurOffset+1);

    if(DataPtr == NULL)
        return TRUE;

    DataPtr[priv->CurOffset] = (Value & 0xFF);

    priv->CurOffset += 1;

    g_hnode_update_data_length( Packet );

    return FALSE;
}

gchar
g_hnode_packet_get_char(GHNodePacket *Packet)
{
    GHNodePacketPrivate *priv;
    gchar TmpVal;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    TmpVal = priv->Data[priv->CurOffset];

    priv->CurOffset += 1;
                       
    return TmpVal;
}

gboolean
g_hnode_packet_set_bytes(GHNodePacket *Packet, guint8 *Buffer, guint32 ByteCount)
{
    GHNodePacketPrivate *priv;
    guint8 *DataPtr;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    // Make sure there is enough room to insert the string of bytes
    DataPtr = g_hnode_packet_get_buffer_ptr(Packet, priv->CurOffset+ByteCount);

    if(DataPtr == NULL)
        return TRUE;

    memcpy(DataPtr+priv->CurOffset, Buffer, ByteCount);

    priv->CurOffset += ByteCount;

    g_hnode_update_data_length( Packet );

    return FALSE;
}

gboolean
g_hnode_packet_get_bytes(GHNodePacket *Packet, guint8 *Buffer, guint32 ByteCount)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    memcpy(Buffer, (priv->Data+priv->CurOffset), ByteCount );

    priv->CurOffset += ByteCount;

    return FALSE;
}

gboolean
g_hnode_packet_skip_bytes(GHNodePacket *Packet, guint32 SkipCount)
{
    GHNodePacketPrivate *priv;

	priv = G_HNODE_PACKET_GET_PRIVATE( Packet );

    priv->CurOffset += SkipCount;

    g_hnode_update_data_length( Packet );

    return FALSE;
}

