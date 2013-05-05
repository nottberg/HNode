// hnode-signal-source.c - GSource to bind libdaemon's signal queue to the GLib main loop
#include <glib.h>
#include <libdaemon/dsignal.h>

#include "hnode-signal-source.h"

typedef struct _GHNodeSignalSource GHNodeSignalSource;
struct _GHNodeSignalSource 
{
    GSource source;

    // Used by the main loop's polling
    GPollFD poll_fd;
};

// Static functions
static gboolean g_hnode_signal_source_prepare( GSource *source, gint *timeout );
static gboolean g_hnode_signal_source_check( GSource *source );
static gboolean g_hnode_signal_source_dispatch( GSource *source, GSourceFunc callback, gpointer user_data );
static void g_hnode_signal_source_finalize( GSource *source );

// Singleton instance of the source -- only one may exist at a time
static GSource *the_source = NULL;

GSourceFuncs signal_source_funcs = 
{
    g_hnode_signal_source_prepare,
    g_hnode_signal_source_check,
    g_hnode_signal_source_dispatch,
    g_hnode_signal_source_finalize,
    NULL,  /* closure_callback */
    NULL,  /* closure_marshal */
};

gint 
g_hnode_signal_source_handle_signal( gint const signal ) 
{
    return daemon_signal_install(signal);
}

GSource *g_hnode_signal_source_new() 
{
    // Abort if there's already a live instance of the source
    g_return_val_if_fail(the_source == NULL, NULL);

    the_source = g_source_new( &signal_source_funcs, sizeof(GHNodeSignalSource) );
    GHNodeSignalSource *signal_source = (GHNodeSignalSource *) the_source;

    // Initialize the structure members
    signal_source->poll_fd.fd = daemon_signal_fd();
    signal_source->poll_fd.events = G_IO_IN | G_IO_HUP | G_IO_ERR;

    // Safety check in case daemon_signal_fd() fails
    g_return_val_if_fail( signal_source->poll_fd.fd != -1, NULL );

    // Tell the main loop to poll the signal queue for activity
    g_source_add_poll( the_source, &signal_source->poll_fd );

    return the_source;
}

guint 
g_hnode_signal_source_add( GHNodeSignalSourceFunc callback, gpointer user_data ) 
{
    return g_hnode_signal_source_add_full( callback, user_data, NULL );
}

guint 
g_hnode_signal_source_add_full( GHNodeSignalSourceFunc callback, gpointer user_data, GDestroyNotify notify ) 
{
    g_return_val_if_fail( callback != NULL, 0 );

    GSource *source = g_hnode_signal_source_new();
    g_return_val_if_fail( source != NULL, 0 );

    // Register the given callback with the source.
    g_source_set_callback( source, (GSourceFunc) callback, user_data, notify );

    // Add the source to the default main context.
    guint const attach_id = g_source_attach( source, NULL );

    // Release this function's reference to the source; it remains referenced
    // by the GMainContext to which it's attached.
    g_source_unref( source );

    return attach_id;
}

gboolean 
g_hnode_signal_source_prepare( GSource *source, gint *timeout ) 
{
    g_return_val_if_fail( source != NULL, FALSE );
    g_return_val_if_fail( timeout != NULL, FALSE );

    g_assert( source == the_source );

    *timeout = -1;

    return FALSE;
}

gboolean 
g_hnode_signal_source_check( GSource *source ) 
{
    g_return_val_if_fail( source != NULL, FALSE );

    g_assert( source == the_source );

    GHNodeSignalSource *signal_source = (GHNodeSignalSource *) source;

    // This was established in signal_source_new()
    g_assert( signal_source->poll_fd.fd == daemon_signal_fd() );

    return( signal_source->poll_fd.revents != 0 );
}

gboolean 
g_hnode_signal_source_dispatch( GSource *source, GSourceFunc callback, gpointer user_data ) 
{
    g_return_val_if_fail( source != NULL, FALSE );
    g_return_val_if_fail( callback != NULL, FALSE );

    g_assert( source == the_source );

    GHNodeSignalSource const *signal_source = (GHNodeSignalSource *) source;
    GHNodeSignalSourceFunc signal_callback = (GHNodeSignalSourceFunc) callback;

    gint signal = daemon_signal_next();
    if( G_LIKELY( signal > 0 ) )  // Successfully received a signal
        signal_callback( signal, user_data );
    else if( signal == 0 ) 
    {  // No signal was queued
        g_warning( "GHNodeSignalSource dispatcher called when no signal was queued" );
    }
    else 
    {  // daemon_signal_next() failed
        g_warning( "Failed to receive signal" );
    }

    if( G_UNLIKELY( signal_source->poll_fd.revents & (G_IO_HUP | G_IO_ERR) ) )
        return FALSE;  // No more input will come from this source
    else
        return TRUE;  // Main loop should continue checking this source
}

void 
g_hnode_signal_source_finalize( GSource *source ) 
{
    g_return_if_fail( source != NULL );

    g_assert( source == the_source );

    // Clean up libdaemon's signal handler
    daemon_signal_done();

    // Reset the singleton pointer
    the_source = NULL;
}
