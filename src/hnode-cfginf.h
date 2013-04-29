/**
 * hnode-cfginf.h
 *
 * Implements defines for hnode configuration interface.
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

#ifndef __G_HNODE_CFGINF_H__
#define __G_HNODE_CFGINF_H__

// Packet type enumeration.
enum
{
    CFGPKT_INFO_REQ,            // 0
    CFGPKT_INFO_RPY,            // 1
    CFGPKT_REGMAP_REQ,          // 2
    CFGPKT_REGMAP_RPY,          // 3
    CFGPKT_READMEM_REQ,         // 4
    CFGPKT_READMEM_RPY,         // 5
    CFGPKT_WRITEMEM_REQ,        // 6
    CFGPKT_WRITEMEM_RPY,        // 7
    CFGPKT_ERASEMEM_REQ,        // 8
    CFGPKT_ERASEMEM_RPY,        // 9
    CFGPKT_RESET_REQ,           // 10
    CFGPKT_RESET_RPY,           // 11
};

// Reply Status Enumeration.
enum
{
    CFGPKT_RESULT_OK,                      // 0
    CFGPKT_RESULT_NOT_IMPLEMENTED,         // 1
    CFGPKT_RESULT_ERROR,                   // 2
    CFGPKT_RESULT_TOO_LONG,                // 3
    CFGPKT_RESULT_BAD_ADDRESS,             // 4
    CFGPKT_RESULT_MISALIGNED_ADDRESS,      // 5
    CFGPKT_RESULT_ACCESS_OUT_OF_BOUNDS,    // 6
    CFGPKT_RESULT_NO_WRITE,                // 7
    CFGPKT_RESULT_ACCESS_ERROR,            // 8
};

// Memory Region Attribute Flags
enum
{
    CFGPKT_MEMATTR_WRITE           = 0x0001,
    CFGPKT_MEMATTR_ERASE_SECTOR    = 0x0002,
    CFGPKT_MEMATTR_CONFIG          = 0x0004,
    CFGPKT_MEMATTR_APPLICATION     = 0x0008,
    CFGPKT_MEMATTR_VECTORS         = 0x0010,
    CFGPKT_MEMATTR_REGISTER        = 0x0020,
    CFGPKT_MEMATTR_RAM             = 0x0040,
    CFGPKT_MEMATTR_FLASH           = 0x0080,
    CFGPKT_MEMATTR_EEPROM          = 0x0100,
    CFGPKT_MEMATTR_READ_ONLY       = 0x0200,
};


enum HNodeCfgUpdateRegionNames
{
    HNCFG_REGNAME_BASE_PRG  = 0,
    HNCFG_REGNAME_BASE_CFG  = 1,
    HNCFG_REGNAME_APP_PRG   = 2,
    HNCFG_REGNAME_APP_CFG   = 3,
    HNCFG_REGNAME_DUMP      = 4,
    HNCFG_REGNAME_FENCE     = 5,
};

// Data structure for a memory region
typedef struct ConfigurationMemoryRegionDef
{
    guint16    Attributes;
    guint16    AccessAlignment;
    guint16    AccessSize;
}CFG_MEM_REGION;

// Data structure for a memory segment
typedef struct ConfigurationMemorySegmentDef
{
    guint32    Address;
    guint32    Length;
}CFG_MEM_SEGMENT;

#endif

