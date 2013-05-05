// logging.c - subsystem for writing diagnostic and error messages
#include <libdaemon/dlog.h>
#include <errno.h>
#include <glib.h>
#include <stdarg.h>  // for va_start()/va_end()
#include <string.h>  // for sterror()
#include <stdlib.h>  // for exit()

#include "hnode-logging.h"

/* Functions for writing messages to syslog.
 *
 * The "_perror" functions write the supplied message followed by a colon, a
 *   space, and the error string associated with the "errno" variable, in the
 *   same way perror() does.
 * The "_msg" functions allow the caller to provide the format string and
 *   its arguments.
 * The "_fatal" variants write the message and then terminate the program with
 *   the supplied exit status.
 */

void 
hnode_log_perror(gint priority, gchar const *prefix) 
{
    daemon_log(priority, "%s: %s", prefix, strerror(errno));
}

void 
hnode_log_msg(gint priority, gchar const *format, ...) 
{
    va_list args;

    va_start(args, format);
    daemon_logv(priority, format, args);
    va_end(args);
}

void 
hnode_log_fatal_perror(gint priority, gchar const *prefix, gint status) 
{
    hnode_log_perror(priority, prefix);
    exit(status);
}

void 
hnode_log_fatal_msg(gint priority, gchar const *format, gint status,  ...) 
{
    va_list args;

    va_start(args, status);
    daemon_logv(priority, format, args);
    va_end(args);
    exit(status);
}

