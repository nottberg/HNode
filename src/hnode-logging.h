// hnode-logging.h - interface for logging.c
#ifndef __HNODE_LOGGING_H__
#define __HNODE_LOGGING_H__

#include <glib.h>
#include <syslog.h>  // For the priority codes

void hnode_log_perror(gint priority, gchar const *prefix);
void hnode_log_msg(gint priority, gchar const *format, ...) G_GNUC_PRINTF (2, 3);
void hnode_log_fatal_perror(gint priority, gchar const *prefix, gint status) G_GNUC_NORETURN;
void hnode_log_fatal_msg(gint priority, gchar const *format, gint status, ...) G_GNUC_NORETURN G_GNUC_PRINTF (2, 4);

#endif  // __HNODE_LOGGING_H__
