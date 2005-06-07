/* 
 * ntpshm.c - put time information in SHM segment for xntpd
 * struct shmTime and getShmTime from file in the xntp distribution:
 *	sht.c - Testprogram for shared memory refclock
 */

#include <sys/types.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <math.h>

#include "gpsd.h"
#ifdef NTPSHM_ENABLE

#if defined(HAVE_SYS_TIME_H)
#include <sys/time.h>
#endif

#include <sys/ipc.h>
#include <sys/shm.h>

#define PPS_MAX_OFFSET	100000		/* microseconds the PPS can 'pull' */
#define PUT_MAX_OFFSET	400000		/* microseconds for lost lock */

#define NTPD_BASE	0x4e545030	/* "NTP0" */
#define SHM_UNIT	0		/* SHM driver unit number (0..3) */

struct shmTime {
    int    mode; /* 0 - if valid set
		  *       use values, 
		  *       clear valid
		  * 1 - if valid set 
		  *       if count before and after read of values is equal,
		  *         use values 
		  *       clear valid
		  */
    int    count;
    time_t clockTimeStampSec;
    int    clockTimeStampUSec;
    time_t receiveTimeStampSec;
    long   receiveTimeStampUSec;
    int    leap;
    int    precision;
    int    nsamples;
    int    valid;
    int    pad[10];
};

static /*@null@*/ struct shmTime *getShmTime(int unit)
{
    int shmid=shmget ((key_t)(NTPD_BASE+unit), 
		      sizeof (struct shmTime), IPC_CREAT|0700);
    if (shmid == -1) {
	gpsd_report(1, "shmget failed\n");
	return NULL;
    } else {
	struct shmTime *p=(struct shmTime *)shmat (shmid, 0, 0);
	if ((int)(long)p == -1) {
	    gpsd_report(1, "shmat failed\n");
	    p=0;
	}
	return p;
    }
}

int ntpshm_init(struct gps_context_t *context, bool enablepps)
/* attach all NTP SHM segments.  called once at startup, while still root */
{
    int i;

    for (i = 0; i < NTPSHMSEGS; i++)
	context->shmTime[i] = getShmTime(i);

    memset(context->shmTimeInuse,0,sizeof(context->shmTimeInuse));
# ifdef PPS_ENABLE
    context->shmTimePPS = enablepps;
# endif /* PPS_ENABLE */
    return 1;
}

int ntpshm_alloc(struct gps_context_t *context)
/* allocate NTP SHM segment.  return its segment number, or -1 */
{
    int i;

    for (i = 0; i < NTPSHMSEGS; i++)
	if (context->shmTime[i] != NULL && !context->shmTimeInuse[i]) {
	    context->shmTimeInuse[i] = true;

	    memset((void *)context->shmTime[i],0,sizeof(struct shmTime));
	    context->shmTime[i]->mode = 1;
	    context->shmTime[i]->precision = -1; /* initially 0.5 sec */
	    context->shmTime[i]->nsamples = 3;	/* stages of median filter */

	    return i;
	}

    return -1;
}


bool ntpshm_free(struct gps_context_t *context, int segment)
/* free NTP SHM segment */
{
    if (segment < 0 || segment >= NTPSHMSEGS)
	return false;

    context->shmTimeInuse[segment] = false;
    return true;
}


int ntpshm_put(struct gps_device_t *session, double fixtime)
/* put a received fix time into shared memory for NTP */
{
    struct shmTime *shmTime = NULL;
    struct timeval tv;
    double seconds,microseconds;

    if (session->shmTime < 0 ||
	(shmTime = session->context->shmTime[session->shmTime]) == NULL)
	return 0;

    (void)gettimeofday(&tv,NULL);
    microseconds = 1000000.0 * modf(fixtime,&seconds);

    shmTime->count++;
    shmTime->clockTimeStampSec = (time_t)seconds;
    shmTime->clockTimeStampUSec = (int)microseconds;
    shmTime->receiveTimeStampSec = (time_t)tv.tv_sec;
    shmTime->receiveTimeStampUSec = tv.tv_usec;
    shmTime->count++;
    shmTime->valid = 1;

    return 1;
}

#ifdef PPS_ENABLE
/* put NTP shared memory info based on received PPS pulse */

int ntpshm_pps(struct gps_device_t *session, struct timeval *tv)
{
    struct shmTime *shmTime,*shmTimeP;
    time_t seconds;
    double offset;

    if (session->shmTime < 0 || session->shmTimeP < 0 ||
	(shmTime = session->context->shmTime[session->shmTime]) == NULL ||
	(shmTimeP = session->context->shmTime[session->shmTimeP]) == NULL)
	return 0;

    /* check if received time messages are within locking range */

    if (abs((shmTime->receiveTimeStampSec-shmTime->clockTimeStampSec)*1000000 +
	     shmTime->receiveTimeStampUSec-shmTime->clockTimeStampUSec)
	    > PUT_MAX_OFFSET)
	return -1;

    if (tv->tv_usec < PPS_MAX_OFFSET) {
	seconds = tv->tv_sec;
	offset = tv->tv_usec / 1000000.0;
    } else {
	if (tv->tv_usec > (1000000 - PPS_MAX_OFFSET)) {
	    seconds = tv->tv_sec + 1;
	    offset = 1 - (tv->tv_usec / 1000000.0);
	} else {
	    shmTimeP->precision = -1;	/* lost lock */
	    gpsd_report(2, "ntpshm_pps: lost PPS lock\n");
	    return -1;
	}
    }

    shmTimeP->count++;
    shmTimeP->clockTimeStampSec = seconds;
    shmTimeP->clockTimeStampUSec = 0;
    shmTimeP->receiveTimeStampSec = tv->tv_sec;
    shmTimeP->receiveTimeStampUSec = tv->tv_usec;
    shmTimeP->precision = offset? (ceil(log(offset) / M_LN2)) != 0 : -20;
    shmTimeP->count++;
    shmTimeP->valid = 1;

    gpsd_report(5, "ntpshm_pps: precision %d\n",shmTimeP->precision);
    return 1;
}
#endif /* PPS_ENABLE */
#endif /* NTPSHM_ENABLE */
