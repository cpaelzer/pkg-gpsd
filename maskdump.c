/* This code is generated.  Do not hand-hack it! */
#include <stdio.h>
#include <string.h>

#include "gpsd.h"

const char *gpsd_maskdump(gps_mask_t set)
{
    static char buf[173];
    const struct {
	gps_mask_t      mask;
	const char      *name;
    } *sp, names[] = {
	{ONLINE_SET,	"ONLINE"},
	{TIME_SET,	"TIME"},
	{TIMERR_SET,	"TIMERR"},
	{LATLON_SET,	"LATLON"},
	{ALTITUDE_SET,	"ALTITUDE"},
	{SPEED_SET,	"SPEED"},
	{TRACK_SET,	"TRACK"},
	{CLIMB_SET,	"CLIMB"},
	{STATUS_SET,	"STATUS"},
	{MODE_SET,	"MODE"},
	{DOP_SET,	"DOP"},
	{VERSION_SET,	"VERSION"},
	{HERR_SET,	"HERR"},
	{VERR_SET,	"VERR"},
	{PERR_SET,	"PERR"},
	{POLICY_SET,	"POLICY"},
	{SATELLITE_SET,	"SATELLITE"},
	{RAW_SET,	"RAW"},
	{USED_SET,	"USED"},
	{SPEEDERR_SET,	"SPEEDERR"},
	{TRACKERR_SET,	"TRACKERR"},
	{CLIMBERR_SET,	"CLIMBERR"},
	{DEVICE_SET,	"DEVICE"},
	{DEVICELIST_SET,	"DEVICELIST"},
	{DEVICEID_SET,	"DEVICEID"},
	{ERROR_SET,	"ERROR"},
	{RTCM2_SET,	"RTCM2"},
	{RTCM3_SET,	"RTCM3"},
	{AIS_SET,	"AIS"},
	{PACKET_SET,	"PACKET"},
	{CLEAR_SET,	"CLEAR"},
	{REPORT_SET,	"REPORT"},
    };

    memset(buf, '\0', sizeof(buf));
    buf[0] = '{';
    for (sp = names; sp < names + sizeof(names)/sizeof(names[0]); sp++)
	if ((set & sp->mask)!=0) {
	    (void)strlcat(buf, sp->name, sizeof(buf));
	    (void)strlcat(buf, "|", sizeof(buf));
	}
    if (buf[1] != '\0')
	buf[strlen(buf)-1] = '\0';
    (void)strlcat(buf, "}", sizeof(buf));
    return buf;
}

