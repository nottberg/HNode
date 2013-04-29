/* $Id: glib-integration.c 937 2005-11-08 21:56:28Z lennart $ */
 
/***
   This file is part of avahi.
  
   avahi is free software; you can redistribute it and/or modify it
   under the terms of the GNU Lesser General Public License as
   published by the Free Software Foundation; either version 2.1 of the
   License, or (at your option) any later version.
  
   avahi is distributed in the hope that it will be useful, but WITHOUT
   ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
   or FITNESS FOR A PARTICULAR PURPOSE. See the GNU Lesser General
   Public License for more details.
  
   You should have received a copy of the GNU Lesser General Public
   License along with avahi; if not, write to the Free Software
   Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307
   USA.
***/
 
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif
 
#include <stdlib.h>
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
#include "hnode-nodeobj.h"
#include "hnode-cfginf.h"

guint8 gUID[16] = {0x01, 0x02, 0x03, 0x44, 0x55, 0x06, 0x07, 0x08, 0x09, 0x0a, 0x0b, 0x0c, 0x0d, 0x0e, 0xfe, 0xf0};

static gint wait_time = 0;
static gint debug_period = 10000;
static gint event_period = 10000;
static gint up_period = 10000;
static gint down_period = 10000;
static gboolean debug = FALSE;
static gboolean event = FALSE;
static gboolean updown = FALSE;

static GOptionEntry entries[] = 
{
  { "wait", 'w', 0, G_OPTION_ARG_INT, &wait_time, "The number of seconds to wait before exiting.", "N" },
  { "debug-period", 0, 0, G_OPTION_ARG_INT, &debug_period, "The number of milli-seconds to wait between the generation of debug frames. (default: 10s)", "N" },
  { "event-period", 0, 0, G_OPTION_ARG_INT, &event_period, "The number of milli-seconds to wait between the generation of event frames. (default: 10s)", "N" },
  { "up-period", 0, 0, G_OPTION_ARG_INT, &up_period, "The number of milli-seconds the hnode will stay registered. (default: 10s)", "N" },
  { "down-period", 0, 0, G_OPTION_ARG_INT, &down_period, "The number of milli-seconds the hnode will stay unregistered. (default: 10s)", "N" },
  { "debug", 'd', 0, G_OPTION_ARG_NONE, &debug, "Periodically generate debug packets.", NULL },
  { "event", 'e', 0, G_OPTION_ARG_NONE, &event, "Periodically generate event packets.", NULL },
  { "updown", 'l', 0, G_OPTION_ARG_NONE, &updown, "Periodically unregister, wait, and reregister the hnode.", NULL },
  { NULL }
};

static gboolean
hnode_debug_timeout (void *userdata)
{
    GHNode    *HNode = userdata;
    guint8    DebugBuf[128];

    g_hnode_send_debug_frame(HNode, 2, 1, sizeof(DebugBuf), DebugBuf);
 
    return TRUE; /* Don't re-schedule timeout event */
}

static gboolean
hnode_event_timeout (void *userdata)
{
    GHNode    *HNode = userdata;
 
    g_hnode_send_event_frame(HNode, 1, 0, 0, NULL);

    return TRUE; /* Don't re-schedule timeout event */
}

static gboolean
hnode_updown_timeout (void *userdata)
{
    GHNode    *HNode = userdata;
 
    g_message ("hnode_updown_timeout!");
     
    return TRUE; /* Don't re-schedule timeout event */
}


//typedef struct ConfigurationMemoryRegionDef
//{
//    guint16    Attributes;
//    guint16    EraseSize;
//    guint16    MaxWriteSize;
//    guint16    MaxReadSize;
//    guint32    StartAddress;
//    guint32    Length;
//    gpointer   BufferPtr;
//}CFG_MEM_REGION;

typedef struct ConfigurationMemoryBuffers
{
    guint8    *BufPtr;
}CFG_MEM_BUFFER;

typedef struct ConfigurationMemoryStorage
{
    gboolean   RequiresErase;
    guint      Address;
    guint      Length;
    guint      SectorSize;
    guint8    *DataPtr;
}CFG_MEM_STORE;

//guint8 gConfigBuffer[512];

guint8 gNodeMem[256*1024];

CFG_MEM_STORE gNodeBasePrgSegments[] = 
{
    {TRUE, 0x1000, 512, 512, &gNodeMem[0]}
};

CFG_MEM_STORE gNodeBaseCfgSegments[] = 
{
    {TRUE, 0x0000, 512, 512, &gNodeMem[(512*1)]}
};

CFG_MEM_STORE gNodeAppPrgSegments[] = 
{
    {TRUE, 0x8200, 512, 512, &gNodeMem[(512*2)]},
    {TRUE, 0x8400, 8192, 512, &gNodeMem[(512*4)]},
};

CFG_MEM_STORE gNodeAppCfgSegments[] = 
{
    {TRUE, 0x1600, 512, 512, &gNodeMem[(512*3)]}
};

CFG_MEM_STORE gNodeDumpSegments[] = 
{
    {TRUE, 0x8200, 512, 512, &gNodeMem[(512*2)]},
    {TRUE, 0x8400, 8192, 512, &gNodeMem[(512*4)]},
};

typedef struct 
{
    guint          SegmentCount;
    CFG_MEM_STORE *SegmentArray;
}CFG_MEM_STORE_REF;

CFG_MEM_STORE_REF gNodeRegionSegments[] =
{
    {1, gNodeBasePrgSegments},
    {1, gNodeBaseCfgSegments},
    {2, gNodeAppPrgSegments},
    {1, gNodeAppCfgSegments},
    {2, gNodeDumpSegments}
};

CFG_MEM_REGION gNodeMemoryRegions[] = 
{
//  Attributes AccessAlignment AccessSize;
    {CFGPKT_MEMATTR_WRITE | CFGPKT_MEMATTR_FLASH | CFGPKT_MEMATTR_CONFIG, 9, 512},
    {CFGPKT_MEMATTR_WRITE | CFGPKT_MEMATTR_FLASH | CFGPKT_MEMATTR_CONFIG, 9, 512},
    {CFGPKT_MEMATTR_WRITE | CFGPKT_MEMATTR_FLASH | CFGPKT_MEMATTR_CONFIG, 9, 512},
    {CFGPKT_MEMATTR_WRITE | CFGPKT_MEMATTR_FLASH | CFGPKT_MEMATTR_CONFIG, 9, 512},
    {CFGPKT_MEMATTR_WRITE | CFGPKT_MEMATTR_FLASH | CFGPKT_MEMATTR_CONFIG, 9, 512}
};



//guint gMemBufCount = sizeof(gNodeMemoryBuffers)/sizeof(CFG_MEM_STORE);

static void
hnode_init_config_store()
{
    guint i,j;
    CFG_MEM_BUFFER *TmpBuffer;
    CFG_MEM_STORE  *TmpStore;
    guint SectorCount;

    for( i = 0; i < sizeof(gNodeMem); i++ )
    {
        gNodeMem[i] = i;
    }

#if 0
    // Initilize each memory region.
    for(i = 0; i < gMemBufCount; i++)
    {
        TmpStore = &gNodeMemoryBuffers[i];

        // Determine the number of Sector Buffers
        SectorCount = TmpStore->Length / TmpStore->SectorSize;
                
        // Allocate a GArray with the correct number of elements
        TmpStore->Buffers = g_array_sized_new(FALSE, TRUE, sizeof(CFG_MEM_BUFFER), SectorCount);

        // Allocate and Initialize the Data Buffers
        for(j = 0; j < SectorCount; j++)
        {
            TmpBuffer = &g_array_index(TmpStore->Buffers, CFG_MEM_BUFFER, j);

            TmpBuffer->BufPtr = g_malloc(TmpStore->SectorSize);

            memset(TmpBuffer->BufPtr, j+1, TmpStore->SectorSize);
        }

    }
#endif    
    
}
#if 0
guint 
hnode_find_config_memreg(GHNode *HNode, guint TargetAddress)
{
    guint RegionCnt = sizeof(gNodeMemoryRegions)/sizeof(CFG_MEM_REGION);
    guint i;

    // Find the memory region of interest
    for(i = 0; i < RegionCnt; i++)
    {
        // Check if the target address is in the current region.
        if(    (TargetAddress >= gNodeMemoryRegions[i].StartAddress)
            && (TargetAddress < (gNodeMemoryRegions[i].StartAddress + gNodeMemoryRegions[i].Length)))
        {
            // Return a pointer to the memory region of interest.
            return i;
        }
     }

    // Memory region was not found
    return (-1);
}
#endif

// Validate that a memory access request is legitimate        
guint
hnode_check_memory_access(guint RegionID, guint Address, guint Length, CFG_MEM_STORE **SegmentPtr)
{
    CFG_MEM_STORE_REF *CurRef;
    CFG_MEM_STORE     *SPtr;
    guint i;
    guint TgtAddress, TgtLength, TgtMaxSize, TgtAlignment;

    // Init
    *SegmentPtr = NULL;

    // Validate the region request
    if( RegionID >=  HNCFG_REGNAME_FENCE )
        return CFGPKT_RESULT_ACCESS_OUT_OF_BOUNDS;

    // Get the region information and find the segment index.
    CurRef = &gNodeRegionSegments[RegionID];

    if( CurRef->SegmentCount == 0 )
        return CFGPKT_RESULT_ACCESS_OUT_OF_BOUNDS;

    for( i = 0; i < CurRef->SegmentCount; i++ )
    {
        SPtr = &CurRef->SegmentArray[i];
 
        TgtAddress   = SPtr->Address;
        TgtLength    = SPtr->Length;
        TgtMaxSize   = gNodeMemoryRegions[RegionID].AccessSize;
        TgtAlignment = gNodeMemoryRegions[RegionID].AccessAlignment;


        // Check if the target address is in the current region.
        if( (Address >= TgtAddress) && (Address < (TgtAddress + TgtLength)) )
        {
            // Make sure the transfer characteristics are supported
            if( Length > TgtMaxSize )
            {
                // Send an error
                return CFGPKT_RESULT_TOO_LONG;  
            }

            // Make sure the transfer characteristics are supported
            if( Address & (0xFFFFFFFF >> (32 - TgtAlignment) ) )
            {
                // Send an error
                return CFGPKT_RESULT_MISALIGNED_ADDRESS;  
            }

            if( (Address + Length) > (TgtAddress + TgtLength) )
            {
                // Send an error
                return CFGPKT_RESULT_ACCESS_OUT_OF_BOUNDS;  
            }

            *SegmentPtr = SPtr;
            return CFGPKT_RESULT_OK;
        }
    }

    // Failed to find a matching memory region
    return CFGPKT_RESULT_BAD_ADDRESS;
}

gboolean
hnode_erase_region(guint RegionID)
{
    CFG_MEM_STORE_REF *CurRef;
    CFG_MEM_STORE     *SPtr;
    guint i;

    // Validate the region request
    if( RegionID >=  HNCFG_REGNAME_FENCE )
        return TRUE;

    // Get the region information and find the segment index.
    CurRef = &gNodeRegionSegments[RegionID];

    if( CurRef->SegmentCount == 0 )
        return FALSE;

    for( i = 0; i < CurRef->SegmentCount; i++ )
    {
        SPtr = &CurRef->SegmentArray[i];

        memset(SPtr->DataPtr, 0xff, SPtr->Length); 
    }

    return FALSE;
}

gboolean
g_hnode_erased_check(guint8 *BufPtr, guint Length)
{
    guint i;
    for(i = 0; i < Length; i++, BufPtr++)
        if( *BufPtr != 0xFF )
            return TRUE;

    // Memory is erased.
    return FALSE; 
}


void 
hnode_config_rx(GHNode *HNode, GHNodePacket *Packet, gpointer data)
{
    guint DataLength;
    guint PktType;
    guint ReqTag;
    guint i;
    guint Address;
    guint Length;

    guint RegionCnt = sizeof(gNodeMemoryRegions)/sizeof(CFG_MEM_REGION);
    guint MemRegIndx;
    guint MemAttr;
    guint status;

    CFG_MEM_REGION *MemRegPtr;
    CFG_MEM_STORE  *MemStore;

    guint MemBufIndx;
    guint MemBufOffset;
    guint TmpLength;
    guint RemainingLength;

    GHNodePacket   *TxPacket;

    DataLength = g_hnode_packet_get_data_length(Packet);

    PktType    = g_hnode_packet_get_uint(Packet);
    ReqTag     = g_hnode_packet_get_uint(Packet);

	switch( PktType )
    {
        // Request for general info about this hnode.
        case CFGPKT_INFO_REQ:

            TxPacket = g_hnode_packet_new();						
    
            // Set the TX address
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Check to see if this is the packet we are interested in.
            g_hnode_packet_set_uint(TxPacket, CFGPKT_INFO_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

    		g_hnode_packet_set_short(TxPacket, 1); // Config Interface Version
    		g_hnode_packet_set_short(TxPacket, RegionCnt);  // Memory Region Count

            g_hnode_packet_set_uint(TxPacket, 1026); // Boot ID
            g_hnode_packet_set_uint(TxPacket, 1026); // Chip ID

            g_hnode_packet_set_short(TxPacket, 1011); // Config ID
            g_hnode_packet_set_short(TxPacket, 1); // Config Version

            for(i = 0; i < RegionCnt; i++)
            {
                // Memory Region
                g_hnode_packet_set_short(TxPacket, gNodeMemoryRegions[i].Attributes);   // Attributes
                g_hnode_packet_set_short(TxPacket, gNodeMemoryRegions[i].AccessAlignment);   // 
                g_hnode_packet_set_short(TxPacket, gNodeMemoryRegions[i].AccessSize);    // Erase Size
            }

			// Send a request for info about the provider.
            g_hnode_send_config_frame(HNode, TxPacket);

		break;

        case CFGPKT_REGMAP_REQ:

            // Read out the region ID that is being asked for.
            MemRegIndx = g_hnode_packet_get_short(Packet);

            // Grab a response packet.
            TxPacket = g_hnode_packet_new();						
    
            // Set the TX address
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Build the response packet.
            g_hnode_packet_set_uint(TxPacket, CFGPKT_REGMAP_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

            // Error check
            if( MemRegIndx > RegionCnt )
            {
                // Send an error
                g_hnode_packet_set_short(TxPacket, CFGPKT_RESULT_ERROR);  
			    g_hnode_send_config_frame(HNode, TxPacket);
                break;  
            }

        	g_hnode_packet_set_short(TxPacket, CFGPKT_RESULT_OK);   
    		g_hnode_packet_set_short(TxPacket, gNodeRegionSegments[MemRegIndx].SegmentCount);  

            for(i = 0; i < gNodeRegionSegments[MemRegIndx].SegmentCount; i++)
            {
                // Memory Region
                g_hnode_packet_set_uint(TxPacket, gNodeRegionSegments[MemRegIndx].SegmentArray[i].Address);   // Attributes
                g_hnode_packet_set_uint(TxPacket, gNodeRegionSegments[MemRegIndx].SegmentArray[i].Length);   // 
            }

			// Send a request for info about the provider.
            g_hnode_send_config_frame(HNode, TxPacket);
        break;

        // Request to read from memory
        case CFGPKT_READMEM_REQ:

            // Read out the additional information from the request packet.
            MemRegIndx  = g_hnode_packet_get_short(Packet);
            Length      = g_hnode_packet_get_short(Packet);
            Address     = g_hnode_packet_get_uint(Packet);
            
            // Allocate a reply packet
            TxPacket = g_hnode_packet_new();						
    
            // Set the TX address
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Check to see if this is the packet we are interested in.
            g_hnode_packet_set_uint(TxPacket, CFGPKT_READMEM_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

            // Find the region that the read will fall into
            status = hnode_check_memory_access( MemRegIndx, Address, Length, &MemStore );
            if(status != CFGPKT_RESULT_OK )
            {
                // Send an error
                g_hnode_packet_set_short(TxPacket, status);  
			    g_hnode_send_config_frame(HNode, TxPacket);
                break;  
            }

            MemRegPtr = &gNodeMemoryRegions[MemRegIndx];

            // Result, Length, Start Address, Data
        	g_hnode_packet_set_short(TxPacket, CFGPKT_RESULT_OK); 
        	g_hnode_packet_set_short(TxPacket, Length);  
            g_hnode_packet_set_uint(TxPacket, Address); 
    
            // Load the data.
            MemBufOffset = Address - MemStore->Address;            
            g_hnode_packet_set_bytes(TxPacket, &MemStore->DataPtr[ MemBufOffset ], Length);  

			// Send the memory read response.
            g_hnode_send_config_frame(HNode, TxPacket);

		break;

        case CFGPKT_ERASEMEM_REQ:
            // Allocate a reply packet
            TxPacket = g_hnode_packet_new();						
    
            // Set the TX address
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Check to see if this is the packet we are interested in.
            g_hnode_packet_set_uint(TxPacket, CFGPKT_ERASEMEM_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

            // Read out the additional information from the request packet.
            MemRegIndx = g_hnode_packet_get_short(Packet);

            //  Get the memory region pointer 
            MemRegPtr = &gNodeMemoryRegions[MemRegIndx];
                        
            // Error check the memory region.
            if( MemRegIndx >=  HNCFG_REGNAME_FENCE )
            {
                // Send an error
                g_hnode_packet_set_short(TxPacket, CFGPKT_RESULT_ACCESS_OUT_OF_BOUNDS);  
			    g_hnode_send_config_frame(HNode, TxPacket);
                break;  
            }

            // Erase the requested region.
            hnode_erase_region(MemRegIndx);

        	g_hnode_packet_set_short(TxPacket, CFGPKT_RESULT_OK);  
            g_hnode_send_config_frame(HNode, TxPacket);

        break;

        case CFGPKT_WRITEMEM_REQ:
            // Allocate a reply packet
            TxPacket = g_hnode_packet_new();						
    
            // Set the TX address
            g_hnode_packet_clone_address(TxPacket, Packet);

	        // Check to see if this is the packet we are interested in.
            g_hnode_packet_set_uint(TxPacket, CFGPKT_WRITEMEM_RPY);
            g_hnode_packet_set_uint(TxPacket, ReqTag);

            // Read out the additional information from the request packet.
            MemRegIndx = g_hnode_packet_get_short(Packet);
            Length     = g_hnode_packet_get_short(Packet);
            Address    = g_hnode_packet_get_uint(Packet);

            // Find the region that the read will fall into
            status = hnode_check_memory_access( MemRegIndx, Address, Length, &MemStore);
            if(status != CFGPKT_RESULT_OK )
            {
                // Send an error
                g_hnode_packet_set_short(TxPacket, status);  
			    g_hnode_send_config_frame(HNode, TxPacket);
                break;  
            }

            MemRegPtr   = &gNodeMemoryRegions[MemRegIndx];

            // Load the data.
            MemBufOffset = Address - MemStore->Address;            
            g_hnode_packet_get_bytes(Packet, &MemStore->DataPtr[ MemBufOffset ], Length);  

            // Send the response packet.
        	g_hnode_packet_set_short(TxPacket, CFGPKT_RESULT_OK); 
        	g_hnode_packet_set_short(TxPacket, Length);  
            g_hnode_packet_set_uint(TxPacket,  Address); 
            g_hnode_send_config_frame(HNode, TxPacket);

        break;

    }

}

int
main (AVAHI_GCC_UNUSED int argc, AVAHI_GCC_UNUSED char *argv[])
{
    GMainLoop *loop = NULL;
    GHNode    *HNode;
    GOptionContext *cmdline_context;
    GError *error = NULL;
 
    // Initialize the gobject type system
    g_type_init();

    // Init memory buffers
    hnode_init_config_store();

    // Parse any command line options.
    cmdline_context = g_option_context_new ("- a hnode implementation used for testing.");
    g_option_context_add_main_entries (cmdline_context, entries, NULL); // GETTEXT_PACKAGE);
    g_option_context_parse (cmdline_context, &argc, &argv, &error);

    // Allocate a server object
    HNode = g_hnode_new();

    if( HNode == NULL )
        exit(-1);

    // HNode intialization

    /* Create the GLIB main loop */
    loop = g_main_loop_new (NULL, FALSE);

    // Register the server event callback
    //g_signal_connect (G_OBJECT (HNode), "state_change",
	//	      G_CALLBACK (hnode_change), NULL);

    // Setup the HNode
    g_hnode_set_version(HNode, 1, 0, 2);
    g_hnode_set_uid(HNode, gUID);
    g_hnode_set_name_prefix(HNode, "TestNode");

    g_hnode_enable_config_support(HNode);
    g_signal_connect (G_OBJECT( HNode ), "config-rx", G_CALLBACK( hnode_config_rx ), NULL);

    g_hnode_set_endpoint_count(HNode, 2);
    
    g_hnode_set_endpoint(HNode, 0, 1, "/x-testintf", 12000, 1, 0, 2);	
    g_hnode_set_endpoint(HNode, 1, 0, "/x-testintf2", 12001, 1, 0, 2);	

    if( debug )
    {
        // Setup a timeout to end the list process
        g_timeout_add( debug_period, hnode_debug_timeout, HNode);
    }

    if( event )
    {
        // Setup a timeout to end the list process
        g_timeout_add( event_period, hnode_event_timeout, HNode);
    }

    if( updown )
    {
        // Setup a timeout to end the list process
        g_timeout_add( up_period, hnode_updown_timeout, HNode);
    }

    // Start up the server object
    g_hnode_start(HNode);

    /* Start the GLIB Main Loop */
    g_main_loop_run (loop);

    fail:
    /* Clean up */
    g_main_loop_unref (loop);

    return 0;
}
