/* $Id: gpsd_report.c 5327 2009-03-03 00:21:00Z esr $ */
#include <sys/types.h>
#include <stdio.h>
#include <stdarg.h>
#include "gpsd_config.h"
#include "gpsd.h"

void gpsd_report(int errlevel UNUSED, const char *fmt, ... )
/* stub logger for clients that don't supply one */
{
    va_list ap;

    va_start(ap, fmt);
    (void)vfprintf(stderr, fmt, ap);
    va_end(ap);
}

