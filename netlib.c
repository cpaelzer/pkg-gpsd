#include "config.h"
#include <stdlib.h>
#include <string.h>

#if defined (HAVE_STRINGS_H)
#include <strings.h>
#endif

#include <sys/types.h>
#include <sys/socket.h>
#include <netinet/in.h>
#include <netdb.h>
#include <stdio.h>
#include <arpa/inet.h>

#if defined (HAVE_SYS_PARAM_H)
#include <sys/param.h>
#endif

#include "outdata.h"
#include "nmea.h"
#include "gpsd.h"


#if !defined (INADDR_NONE)
#define INADDR_NONE   ((in_addr_t)-1)
#endif

static char mbuf[128];

int passivesock(char *service, char *protocol, int qlen)
{
    struct servent *pse;
    struct protoent *ppe;
    struct sockaddr_in sin;
    int s, type;
    int one = 1;

    bzero((char *) &sin, sizeof(sin));
    sin.sin_family = AF_INET;
    sin.sin_addr.s_addr = INADDR_ANY;

    if ( (pse = getservbyname(service, protocol)) )
	sin.sin_port = htons(ntohs((u_short) pse->s_port));
    else if ((sin.sin_port = htons((u_short) atoi(service))) == 0) {
	sprintf(mbuf, "Can't get \"%s\" service entry.\n", service);
	gps_gpscli_errexit(mbuf);
    }
    if ((ppe = getprotobyname(protocol)) == 0) {
	sprintf(mbuf, "Can't get \"%s\" protocol entry.\n", protocol);
	gps_gpscli_errexit(mbuf);
    }
    if (strcmp(protocol, "udp") == 0)
	type = SOCK_DGRAM;
    else
	type = SOCK_STREAM;

    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
	gps_gpscli_errexit("Can't create socket:");

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1) {
        sprintf(mbuf, "%s", "Error: SETSOCKOPT SO_REUSEADDR");
	gps_gpscli_errexit(mbuf);
    }
    if (bind(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	sprintf(mbuf, "Can't bind to port %s", service);
	gps_gpscli_errexit(mbuf);
    }
    if (type == SOCK_STREAM && listen(s, qlen) < 0) {
	sprintf(mbuf, "Can't listen on %s port:", service);
	gps_gpscli_errexit(mbuf);
    }
    return s;
}

int netlib_passiveTCP(char *service, int qlen)
{
    return passivesock(service, "tcp", qlen);
}


int netlib_connectsock(char *host, char *service, char *protocol)
{
    struct hostent *phe;
    struct servent *pse;
    struct protoent *ppe;
    struct sockaddr_in sin;
    int s, type;
    int one = 1;

    bzero((char *) &sin, sizeof(sin));
    sin.sin_family = AF_INET;

    if ( (pse = getservbyname(service, protocol)) )
	sin.sin_port = htons(ntohs((u_short) pse->s_port));
    else if ((sin.sin_port = htons((u_short) atoi(service))) == 0) {
	sprintf(mbuf, "Can't get \"%s\" service entry.\n", service);
	gps_gpscli_errexit(mbuf);
    }
    if ( (phe = gethostbyname(host)) )
	bcopy(phe->h_addr, (char *) &sin.sin_addr, phe->h_length);
    else if ((sin.sin_addr.s_addr = inet_addr(host)) == INADDR_NONE) {
	sprintf(mbuf, "Can't get host entry: \"%s\".\n", host);
	gps_gpscli_errexit(mbuf);
    }
    if ((ppe = getprotobyname(protocol)) == 0) {
	sprintf(mbuf, "Can't get \"%s\" protocol entry.\n", protocol);
	gps_gpscli_errexit(mbuf);
    }
    if (strcmp(protocol, "udp") == 0)
	type = SOCK_DGRAM;
    else
	type = SOCK_STREAM;

    s = socket(PF_INET, type, ppe->p_proto);
    if (s < 0)
	gps_gpscli_errexit("Can't create socket:");

    if (setsockopt(s, SOL_SOCKET, SO_REUSEADDR, (char *)&one, sizeof(one)) == -1) {
        sprintf(mbuf, "%s", "Error: SETSOCKOPT SO_REUSEADDR");
	gps_gpscli_errexit(mbuf);
    }

    if (connect(s, (struct sockaddr *) &sin, sizeof(sin)) < 0) {
	sprintf(mbuf, "Can't connect to %s.%s", host, service);
	gps_gpscli_errexit(mbuf);
    }
    return s;
}

int netlib_connectTCP(char *host, char *service)
{
    return netlib_connectsock(host, service, "tcp");
}
