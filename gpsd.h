/* gpsd.h -- fundamental types and structures for the GPS daemon */

#include "config.h"
#include "gps.h"
#include "gpsutils.h"

/* Some internal capabilities depend on which drivers we're compiling. */
#ifdef EARTHMATE_ENABLE
#define ZODIAC_ENABLE	
#endif
#if defined(ZODIAC_ENABLE) || defined(SIRFII_ENABLE) || defined(GARMIN_ENABLE)
#define BINARY_ENABLE	
#endif
#if defined(TRIPMATE_ENABLE) || defined(BINARY_ENABLE)
#define NON_NMEA_ENABLE	
#endif

#define NMEA_MAX	82		/* max length of NMEA sentence */
#define NMEA_BIG_BUF	(2*NMEA_MAX+1)	/* longer than longest NMEA sentence */

/*
 * User Equivalent Range Error
 * UERE is the square root of the sum of the squares of individual
 * errors.  We compute based on the following error budget for
 * satellite range measurements.  Note: this is only used if the
 * GPS doesn't report estimated position error itself.
 *
 * From R.B Langley's 1997 "The GPS error budget". 
 * GPS World , Vol. 8, No. 3, pp. 51-56
 *
 * Atmospheric error -- ionosphere                 7.0m
 * Atmospheric error -- troposphere                0.7m
 * Clock and ephemeris error                       3.6m
 * Receiver noise                                  1.5m
 * Multipath effect                                1.2m
 *
 * From Hoffmann-Wellenhof et al. (1997), "GPS: Theory and Practice", 4th
 * Ed., Springer.
 *
 * Code range noise (C/A)                          0.3m
 * Code range noise (P-code)                       0.03m
 * Phase range                                     0.005m
 *
 * Carl Carter of SiRF says: "Ionospheric error is typically corrected for 
 * at least in large part, by receivers applying the Klobuchar model using 
 * data supplied in the navigation message (subframe 4, page 18, Ionospheric 
 * and UTC data).  As a result, its effect is closer to that of the 
 * troposphere, amounting to the residual between real error and corrections.
 *
 * "Multipath effect is dramatically variable, ranging from near 0 in
 * good conditions (for example, our roof-mounted antenna with few if any
 * multipath sources within any reasonable range) to hundreds of meters in
 * tough conditions like urban canyons.  Picking a number to use for that
 * is, at any instant, a guess."
 *
 * "Using Hoffman-Wellenhoff is fine, but you can't use all 3 values.
 * You need to use one at a time, depending on what you are using for
 * range measurements.  For example, our receiver only uses the C/A
 * code, never the P code, so the 0.03 value does not apply.  But once
 * we lock onto the carrier phase, we gradually apply that as a
 * smoothing on our C/A code, so we gradually shift from pure C/A code
 * to nearly pure carrier phase.  Rather than applying both C/A and
 * carrier phase, you need to determine how long we have been using
 * the carrier smoothing and use a blend of the two."
 *
 * Incorporating Carl's advice, we construct an error budget including the
 * troposphere correction twice and the most conservative of the 
 * Hoffmann-Wellenhof numbers.  We have no choice but to accept that this
 * will be an underestimate for urban-canyon conditions, because we
 * don't know anything about the distribution of multipath errors.
 * 
 * Taking the square root of the sum of squares...
 * UERE=sqrt(0.7^2 + 0.7^2 + 3.6^2 + 1.5^2 + 1.2^2 + 0.3^2)
 *
 * Note: we're assuming these are 1-sigma error ranges. This needs to
 * be checked in the sources.
 *
 * See http://www.seismo.berkeley.edu/~battag/GAMITwrkshp/lecturenotes/unit1/
 * for discussion.
 *
 * DGPS corrects for atmospheric distortion, ephemeris error, and satellite/
 * receiver clock error.  Thus:
 * UERE =  sqrt(1.5^2 + 1.2^2 + 0.3^2)
 */
#define UERE_NO_DGPS	4.2095
#define UERE_WITH_DGPS	1.9442
#define UERE(session)	((session->dsock==-1) ? UERE_NO_DGPS : UERE_WITH_DGPS)

struct gps_context_t {
    int valid;
#define LEAP_SECOND_VALID	0x01
    int leap_seconds;
};

struct gps_device_t;

struct gps_type_t {
/* GPS method table, describes how to talk to a particular GPS type */
    char *typename, *trigger;
    int (*probe)(struct gps_device_t *session);
    void (*initializer)(struct gps_device_t *session);
    int (*get_packet)(struct gps_device_t *session, int waiting);
    int (*parse_packet)(struct gps_device_t *session);
    int (*rtcm_writer)(struct gps_device_t *session, char *rtcmbuf, int rtcmbytes);
    int (*speed_switcher)(struct gps_device_t *session, int speed);
    void (*mode_switcher)(struct gps_device_t *session, int mode);
    void (*wrapup)(struct gps_device_t *session);
    int cycle;
};

#if defined (HAVE_SYS_TERMIOS_H)
#include <sys/termios.h>
#else
#if defined (HAVE_TERMIOS_H)
#include <termios.h>
#endif
#endif

/*
 * The packet buffers need to be as long than the longest packet we
 * expect to see in any protocol, because we have to be able to hold
 * an entire packet for checksumming.  Thus, in particular, they need
 * to be as long as a SiRF MID 4 packet, 188 bytes payload plus eight bytes 
 * of header/length/checksum/trailer. 
 */
#define MAX_PACKET_LENGTH	196	/* 188 + 8 */

struct gps_device_t {
/* session object, encapsulates all global state */
    struct gps_data_t gpsdata;
    struct gps_type_t *device_type;
    int dsock;		/* socket to DGPS server */
    int sentdgps;	/* have we sent a DGPS correction? */
    int fixcnt;		/* count of good fixes seen */
    struct termios ttyset, ttyset_old;
    /* packet-getter internals */
    int packet_full;	/* do we presently see a full packet? */
    int	packet_type;
#define BAD_PACKET	-1
#define NMEA_PACKET	0
#define SIRF_PACKET	1
#define ZODIAC_PACKET	2
    unsigned int packet_state;
    unsigned int packet_length;
    unsigned char inbuffer[MAX_PACKET_LENGTH*2+1];
    unsigned short inbuflen;
    unsigned char *inbufptr;
    unsigned char outbuffer[MAX_PACKET_LENGTH+1];
    unsigned short outbuflen;
    unsigned long counter;
#ifdef BINARY_ENABLE
    struct gps_fix_t lastfix;	/* use to compute uncertainties */
    unsigned int driverstate;	/* for private use */
    double separation;		/* Geoidal separation, MSL - WGS84 (Meters) */
#define NO_SEPARATION	-99999	/* must be out of band */
    double mag_var;		/* Magnetic variation in degrees */  
#ifdef GARMIN_ENABLE	/* private housekeeping stuff for the Garmin driver */
    void *GarminBuffer; /* Pointer Garmin packet buffer 
                           void *, to keep the packet details out of the 
                           gloabl contect and save spave */
    long GarminBufferLen;                  /* current GarminBuffer Length */
#endif /* GARMIN_ENABLE */
#ifdef ZODIAC_ENABLE	/* private housekeeping stuff for the Zodiac driver */
    unsigned short sn;		/* packet sequence number */
    /*
     * Zodiac chipset channel status from PRWIZCH. Keep it so raw-mode 
     * translation of Zodiac binary protocol can send it up to the client.
     */
    int Zs[MAXCHANNELS];	/* satellite PRNs */
    int Zv[MAXCHANNELS];	/* signal values (0-7) */
#endif /* ZODIAC_ENABLE */
#endif /* BINARY_ENABLE */
#ifdef NTPSHM_ENABLE
    struct shmTime *shmTime;
    unsigned int time_seen;
#define TIME_SEEN_GPS_1	0x01	/* Seen GPS time variant 1? */
#define TIME_SEEN_GPS_2	0x02	/* Seen GPS time variant 1? */
#define TIME_SEEN_UTC_1	0x04	/* Seen UTC time variant 1? */
#define TIME_SEEN_UTC_2	0x08	/* Seen UTC time variant 1? */
#define TIME_SEEN_PPS	0x10	/* Seen PPS signal? */
#endif /* NTPSHM_ENABLE */
    double poll_times[FD_SETSIZE];	/* last daemon poll time */

    struct gps_context_t	*context;
};

#define IS_HIGHEST_BIT(v,m)	!(v & ~((m<<1)-1))

/* here are the available GPS drivers */
extern struct gps_type_t **gpsd_drivers;

/* GPS library internal prototypes */
extern int nmea_parse(char *, struct gps_data_t *);
extern int nmea_send(int, const char *, ... );
extern void nmea_add_checksum(char *);

extern int sirf_parse(struct gps_device_t *, unsigned char *, int);

extern int packet_get(struct gps_device_t *, int);
extern int packet_sniff(struct gps_device_t *);

extern int gpsd_open(struct gps_device_t *);
extern int gpsd_switch_driver(struct gps_device_t *, char *);
extern int gpsd_set_speed(struct gps_device_t *, unsigned int, unsigned int);
extern int gpsd_get_speed(struct termios *);
extern void gpsd_close(struct gps_device_t *);

extern void gpsd_raw_hook(struct gps_device_t *, char *);
extern void gpsd_zero_satellites(struct gps_data_t *);
extern void gpsd_binary_fix_dump(struct gps_device_t *, char *);
extern void gpsd_binary_satellite_dump(struct gps_device_t *, char *);
extern void gpsd_binary_quality_dump(struct gps_device_t *, char *);

extern int netlib_connectsock(const char *, const char *, const char *);

extern int ntpshm_init(struct gps_device_t *);
extern int ntpshm_put(struct gps_device_t *, double);

extern void ecef_to_wgs84fix(struct gps_fix_t *,
			     double, double, double, 
			     double, double, double);
extern void dop(int, struct gps_data_t *);

/* External interface */
extern int gpsd_open_dgps(char *);
extern struct gps_device_t * gpsd_init(struct gps_context_t *, char *device);
extern int gpsd_activate(struct gps_device_t *);
extern void gpsd_deactivate(struct gps_device_t *);
extern int gpsd_poll(struct gps_device_t *);
extern void gpsd_wrap(struct gps_device_t *);

/* caller should supply this */
void gpsd_report(int, const char *, ...);



