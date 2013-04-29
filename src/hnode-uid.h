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

#include <glib.h>
#include <glib-object.h>

#ifndef __G_HNODE_UID_H__
#define __G_HNODE_UID_H__

G_BEGIN_DECLS

#define G_TYPE_HNODE_UID		    (g_hnode_address_get_type ())
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

G_END_DECLS

#endif // _G_HNODE_UID_H_

