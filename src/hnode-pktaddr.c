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
* Address Object
*
*
*************************************************************************************************************************/

#define G_HNODE_ADDRESS_GET_PRIVATE(obj) (G_TYPE_INSTANCE_GET_PRIVATE ((obj), G_TYPE_HNODE_ADDRESS, GHNodeAddressPrivate))

G_DEFINE_TYPE (GHNodeAddress, g_hnode_address, G_TYPE_OBJECT);

typedef struct _GHNodeAddressPrivate GHNodeAddressPrivate;
struct _GHNodeAddressPrivate
{    
    struct sockaddr_in   Addr;
    socklen_t            AddrLen;

    gchar                AddrStr[128];
    guint32              AddrStrLen;   

    gboolean                   dispose_has_run;

};

static GObjectClass *parent_class = NULL;

static void
g_hnode_address_dispose (GObject *obj)
{
    GHNodeAddress          *self = (GHNodeAddress *)obj;
    GHNodeAddressPrivate   *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE(self);

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
g_hnode_address_finalize (GObject *obj)
{
    GHNodeAddress *self = (GHNodeAddress *)obj;
    
    /* Chain up to the parent class */
    G_OBJECT_CLASS (parent_class)->finalize (obj);
}

static void
g_hnode_address_class_init (GHNodeAddressClass *class)
{
	GObjectClass *o_class;
	int error;
	
	o_class = G_OBJECT_CLASS (class);

    o_class->dispose  = g_hnode_address_dispose;
    o_class->finalize = g_hnode_address_finalize;

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
	g_type_class_add_private (o_class, sizeof (GHNodeAddressPrivate));
}

static void
g_hnode_address_init (GHNodeAddress *sb)
{
	GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE (sb);
         
    //priv->RxAddr   = NULL;
    priv->AddrLen    = sizeof(priv->Addr);

    priv->AddrStr[0] = '\0';
    priv->AddrStrLen = 0;   

    priv->dispose_has_run = FALSE;

}

gint
g_hnode_address_ResolveAddressStr( GHNodeAddress *Address )
{
    char *ptr;
    int ntoken;
    char tmpname[512];
    char tmphost[512];
    char tmpport[512];
    int tmptype;
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    // Send a query packet to the node.  Open a UDP socket 
   	// to the nodes address.
    //NodeAddr.sin_family = AF_INET;
    //NodeAddr.sin_port   = htons(AddressStr);
    //inet_aton( (char *)AddressStr, &NodeAddr.sin_addr ); 
    // Initialization stuff
    tmpname[0]= 0;
    tmphost[0]= 0;
    tmpport[0]= 0;

    //if (host)
    //    *host= 0;
    //if (port) 
    //    *port= 0;
    //if (name)
    //    *name= 0;

    // Look for a 'rpcap://' identifier
    if ( (ptr= strstr(priv->AddrStr, "hnode://")) != NULL)
    {
        ptr+= strlen("hnode://");
 
        if (strchr(ptr, '[')) // This is probably a numeric address
        {
            ntoken= sscanf(ptr, "[%[1234567890:.]]:%[^/]/%s", tmphost, tmpport, tmpname);
 
            if (ntoken == 1)    // probably the port is missing
                ntoken= sscanf(ptr, "[%[1234567890:.]]/%s", tmphost, tmpname);
 
            tmptype= 1;
        }
        else
        {
            ntoken= sscanf(ptr, "%[^/:]:%[^/]/%s", tmphost, tmpport, tmpname);
 
            if (ntoken == 1)
            {
                // This can be due to two reasons:
                // - we want a remote capture, but the network port is missing
                // - we want to do a local capture
                // To distinguish between the two, we look for the '/' char
                if (strchr(ptr, '/'))
                {
                    // We're on a remote capture
                    sscanf(ptr, "%[^/]/%s", tmphost, tmpname);
                    tmptype = 2;
                }
                else
                {
                    // We're on a local capture
                    if (*ptr)
                        strncpy(tmpname, ptr, 256);
 
                    // Clean the host name, since it is a remote capture
                    // NOTE: the host name has been assigned in the previous "ntoken= sscanf(...)" line
                    tmphost[0]= 0;
 
                    tmptype = 3;
                }
            }
            else
                tmptype= 4;
        }

        if (tmpname[0])
        {
//            if (host)
//                strcpy(host, tmphost);
//            if (port) 
//                strcpy(port, tmpport);
//            if (name)
//                strcpy(name, tmpname);
//            if (type)
//                *type= tmptype;
 
            return 0;
        }
        else
        {
            priv->Addr.sin_family = AF_INET;
            priv->Addr.sin_port   = htons((guint16)strtoul(tmpport, NULL, 0));
            inet_aton( (char *)tmphost, &priv->Addr.sin_addr ); 
            priv->AddrLen = sizeof(priv->Addr);

//            if (errbuf)
//                snprintf(errbuf, PCAP_ERRBUF_SIZE, "The interface name has not been specified.");
 
            return -1;
        }
    }

    // Look for a 'rpcap://' identifier
    if ( (ptr= strstr(priv->AddrStr, "hmnode://")) != NULL)
    {
        ptr+= strlen("hmnode://");
 
        if (strchr(ptr, '[')) // This is probably a numeric address
        {
            ntoken= sscanf(ptr, "[%[1234567890:.]]:%[^/]/%s", tmphost, tmpport, tmpname);
 
            if (ntoken == 1)    // probably the port is missing
                ntoken= sscanf(ptr, "[%[1234567890:.]]/%s", tmphost, tmpname);
 
            tmptype= 1;
        }
        else
        {
            ntoken= sscanf(ptr, "%[^/:]:%[^/]/%s", tmphost, tmpport, tmpname);
 
            if (ntoken == 1)
            {
                // This can be due to two reasons:
                // - we want a remote capture, but the network port is missing
                // - we want to do a local capture
                // To distinguish between the two, we look for the '/' char
                if (strchr(ptr, '/'))
                {
                    // We're on a remote capture
                    sscanf(ptr, "%[^/]/%s", tmphost, tmpname);
                    tmptype = 2;
                }
                else
                {
                    // We're on a local capture
                    if (*ptr)
                        strncpy(tmpname, ptr, 256);
 
                    // Clean the host name, since it is a remote capture
                    // NOTE: the host name has been assigned in the previous "ntoken= sscanf(...)" line
                    tmphost[0]= 0;
 
                    tmptype = 3;
                }
            }
            else
                tmptype= 4;
        }

        if (tmpname[0])
        {
//            if (host)
//                strcpy(host, tmphost);
//            if (port) 
//                strcpy(port, tmpport);
//            if (name)
//                strcpy(name, tmpname);
//            if (type)
//                *type= tmptype;
 
            return 0;
        }
        else
        {
            priv->Addr.sin_family = AF_INET;
            priv->Addr.sin_port   = htons((guint16)strtoul(tmpport, NULL, 0));
            inet_aton( (char *)tmphost, &priv->Addr.sin_addr ); 
            priv->AddrLen = sizeof(priv->Addr);

//            if (errbuf)
//                snprintf(errbuf, PCAP_ERRBUF_SIZE, "The interface name has not been specified.");
 
            return -1;
        }
    }

    // Look for a 'file://' identifier
    if ( (ptr= strstr(priv->AddrStr, "file://")) != NULL)
    {
        ptr+= strlen("file://");
        if (*ptr)
        {
//            if (name)
//                strncpy(name, ptr, PCAP_BUF_SIZE);

//            if (type)
//                *type= PCAP_SRC_FILE;
 
            return 0;
        }
        else
        {
//            if (errbuf)
//                snprintf(errbuf, PCAP_ERRBUF_SIZE, "The file name has not been specified.");
 
            return -1;
        }
 
    }
 
    // Backward compatibility; the user didn't use the 'rpcap://, file://'  specifiers
    if ( (priv->AddrStr) && (*priv->AddrStr) )
    {
//        if (name)
//            strncpy(name, source, PCAP_BUF_SIZE);
 
//        if (type)
//            *type= PCAP_SRC_IFLOCAL;
 
        return 0;
    }
    else
    {
//        if (errbuf)
//            snprintf(errbuf, PCAP_ERRBUF_SIZE, "The interface name has not been specified.");
 
        return -1;
    }
}

gint
g_hnode_address_CreateAddressStr( GHNodeAddress *Address )
{
    GHNodeAddressPrivate *priv;
    
	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    priv->AddrStrLen = g_snprintf(priv->AddrStr, sizeof(priv->AddrStr), "hnode://%s:%u", inet_ntoa( priv->Addr.sin_addr ), ntohs(priv->Addr.sin_port));

}

GHNodeAddress *
g_hnode_address_new (void)
{
	return g_object_new (G_TYPE_HNODE_ADDRESS, NULL);
}

//void
//g_hnode_address_free(GHNodeAddress *Addr)
//{
//	g_object_unref(Addr);
//}

void
g_hnode_address_set_str(GHNodeAddress *Address, gchar *AddrStr )
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    // Copy the address string.
    priv->AddrStrLen = g_strlcpy(priv->AddrStr, AddrStr, sizeof(priv->AddrStr));

    // Turn the address string into an address.
    g_hnode_address_ResolveAddressStr(Address);

}

void
g_hnode_address_get_str_ptr(GHNodeAddress *Address, gchar **AddrStr, guint32 *AddrStrLen)    
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    // Copy the address string.
    *AddrStr     = priv->AddrStr;
    *AddrStrLen = priv->AddrStrLen;
}

gboolean
g_hnode_address_get_ipv4_ptr(GHNodeAddress *Address, struct sockaddr_in **Addr, guint32 **AddrLen)
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    // Copy the address over.
    *Addr    = &priv->Addr;
    *AddrLen = &priv->AddrLen;    

    return FALSE;
}

gboolean
g_hnode_address_get_ipv4_address(GHNodeAddress *Address, struct sockaddr_in *Addr, guint32 *AddrLen)
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    // Copy the address over.
    *Addr    = priv->Addr;
    *AddrLen = priv->AddrLen;    

    return FALSE;
}

gboolean
g_hnode_address_set_ipv4_address(GHNodeAddress *Address, struct sockaddr_in *Addr, guint32 AddrLen)
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    priv->Addr    = *Addr;
    priv->AddrLen = sizeof(priv->Addr);

    g_hnode_address_CreateAddressStr(Address);
    
    return FALSE;
}

gboolean 
g_hnode_address_ipv4_commit( GHNodeAddress *Address )
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    g_hnode_address_CreateAddressStr(Address);

    return FALSE;
}

guint16 
g_hnode_address_GetPort( GHNodeAddress *Address )
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    return htons(priv->Addr.sin_port);
}

void
g_hnode_address_set_port( GHNodeAddress *Address, guint16 Port )
{
    GHNodeAddressPrivate *priv;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    priv->Addr.sin_port = ntohs(Port);

    // Update the address string to pick up the new port value.
    g_hnode_address_CreateAddressStr(Address);

}
gboolean
g_hnode_address_is_equal( GHNodeAddress *Address, GHNodeAddress *TestAddrObj)
{
    GHNodeAddressPrivate *priv;

    struct sockaddr_in   *TestAddr;
    guint32              *TestAddrLen;

	priv = G_HNODE_ADDRESS_GET_PRIVATE( Address );

    // Get the comparison data from the test address
    g_hnode_address_get_ipv4_ptr(TestAddrObj, &TestAddr, &TestAddrLen);

    // Make sure the lengths are equal
    if( *TestAddrLen != priv->AddrLen )
        return FALSE;

    // Compare the ports
    if( TestAddr->sin_port != priv->Addr.sin_port )
        return FALSE;

    // Compare the addresses
    if( TestAddr->sin_addr.s_addr != priv->Addr.sin_addr.s_addr )
        return FALSE;

    return TRUE;
}

