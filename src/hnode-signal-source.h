/* hnode-signal_source.h - interface for hnode-signal-source.c
 */
#ifndef __HNODE_SIGNAL_SOURCE_H__
#define __HNODE_SIGNAL_SOURCE_H__

#include <glib.h>

G_BEGIN_DECLS

// The type of the callback function used for handling queued signals.
typedef gboolean ( *GHNodeSignalSourceFunc )( gint signal, gpointer );

// Specify a signal which should be queued by this source.
gint g_hnode_signal_source_handle_signal( gint const signal );

GSource *g_hnode_signal_source_new();
guint g_hnode_signal_source_add( GHNodeSignalSourceFunc callback, gpointer user_data );
guint g_hnode_signal_source_add_full( GHNodeSignalSourceFunc callback, gpointer user_data, GDestroyNotify notify );

G_END_DECLS

#endif  // __HNOCE_SIGNAL_SOURCE_H__
