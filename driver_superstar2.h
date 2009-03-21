/* $Id: driver_superstar2.h 5317 2009-03-02 20:04:36Z ckuethe $ */
#ifndef _GPSD_SUPERSTAR2_H_
#define _GPSD_SUPERSTAR2_H_

#define SUPERSTAR2_BASE_SIZE 4
#define SUPERSTAR2_TYPE_OFFSET 1

/* input-only */
#define SUPERSTAR2_RESET 2
#define SUPERSTAR2_LINKUP 63
#define SUPERSTAR2_CHANNEL_INHIBIT 64
#define SUPERSTAR2_TIME_PARAMS 69
#define SUPERSTAR2_ALMANAC_INCREMENT 77
#define SUPERSTAR2_ALMANAC_UPLOAD 79
#define SUPERSTAR2_SET_OPMODE 80
#define SUPERSTAR2_SET_MASK 81
#define SUPERSTAR2_SET_DGPS 83
#define SUPERSTAR2_SET_IONOMODEL 84
#define SUPERSTAR2_SET_MSLMODEL 86
#define SUPERSTAR2_SET_HEIGHT_MODE 87
#define SUPERSTAR2_SET_DATUM 88
#define SUPERSTAR2_SATELLITE_INHIBIT 90
#define SUPERSTAR2_BASE_CONFIG 91
#define SUPERSTAR2_SATELLITE_TRACK 95
#define SUPERSTAR2_NVM_ERASE 99
#define SUPERSTAR2_SET_TIME 103
#define SUPERSTAR2_MESSAGE_CONFIG 105
#define SUPERSTAR2_SERIAL_CONFIG 110

/* output-only */
#define SUPERSTAR2_CHANINF2 7
#define SUPERSTAR2_LINKERR 125
#define SUPERSTAR2_ACK 126

/* bidirectional */
#define SUPERSTAR2_DUMMY 0
#define SUPERSTAR2_CHANINF 6
#define SUPERSTAR2_NAVSOL_LLA 20
#define SUPERSTAR2_NAVSOL_ECEF 21
#define SUPERSTAR2_EPHEMERIS 22
#define SUPERSTAR2_MEASUREMENT 23
#define SUPERSTAR2_RECV_CONFIG 30
#define SUPERSTAR2_SVINFO 33
#define SUPERSTAR2_DGPSCONFIG 43
#define SUPERSTAR2_VERSION 45
#define SUPERSTAR2_BASE_STATUS 47
#define SUPERSTAR2_DGPS_STATUS 48
#define SUPERSTAR2_RECV_STATUS 49
#define SUPERSTAR2_SAT_HEALTH 50
#define SUPERSTAR2_SELFTEST 51
#define SUPERSTAR2_RTCM_DATA 65
#define SUPERSTAR2_SBAS_DATA 67
#define SUPERSTAR2_SBAS_STATUS 68
#define SUPERSTAR2_IONO_UTC 75
#define SUPERSTAR2_ALMANAC_DATA 76
#define SUPERSTAR2_ALMANAC_STATUS 78
#define SUPERSTAR2_TIMING 113

#endif /* _GPSD_SUPERSTAR2_H_ */
