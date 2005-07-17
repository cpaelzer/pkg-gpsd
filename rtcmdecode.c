#include <unistd.h>
#include <stdlib.h>
#include <string.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdarg.h>

#include "gpsd.h"

static int verbose = RTCM_ERRLEVEL_BASE;

void gpsd_report(int errlevel, const char *fmt, ... )
/* assemble command in printf(3) style, use stderr or syslog */
{
    if (errlevel <= verbose) {
	char buf[BUFSIZ];
	va_list ap;

	strcpy(buf, "rtcmdecode: ");
	va_start(ap, fmt) ;
	(void)vsnprintf(buf + strlen(buf), sizeof(buf)-strlen(buf), fmt, ap);
	va_end(ap);
	(void)fputs(buf, stdout);
    }
}

int main(int argc, char **argv)
{
    int             c;
    struct gps_device_t device;
    enum rtcmstat_t res;
    char buf[BUFSIZ];

    while ((c = getopt(argc, argv, "v:")) != EOF) {
	switch (c) {
	case 'v':		/* verbose */
	    verbose = RTCM_ERRLEVEL_BASE + atoi(optarg);
	    break;

	case '?':
	default:
	    (void)fputs("rtcmdecode [-v]\n", stderr);
	    exit(1);
	}
    }
    argc -= optind;
    argv += optind;

    rtcm_init(&device);

    while ((c = getchar()) != EOF) {
	res = rtcm_decode(&device, (unsigned int)c);
	if (res == RTCM_STRUCTURE) {
	    rtcm_dump(&device, buf, sizeof(buf));
	    (void)fputs(buf, stdout);
	}
    }
    exit(0);
}

/* rtcmdecode.c ends here */
