/*
 * Copyright (c) 2005 Jeff Francis <jeff@gritch.org>
 *
 * Permission to use, copy, modify, and distribute this software for any
 * purpose with or without fee is hereby granted, provided that the above
 * copyright notice and this permission notice appear in all copies.
 *
 * THE SOFTWARE IS PROVIDED "AS IS" AND THE AUTHOR DISCLAIMS ALL WARRANTIES
 * WITH REGARD TO THIS SOFTWARE INCLUDING ALL IMPLIED WARRANTIES OF
 * MERCHANTABILITY AND FITNESS. IN NO EVENT SHALL THE AUTHOR BE LIABLE FOR
 * ANY SPECIAL, DIRECT, INDIRECT, OR CONSEQUENTIAL DAMAGES OR ANY DAMAGES
 * WHATSOEVER RESULTING FROM LOSS OF USE, DATA OR PROFITS, WHETHER IN AN
 * ACTION OF CONTRACT, NEGLIGENCE OR OTHER TORTIOUS ACTION, ARISING OUT OF
 * OR IN CONNECTION WITH THE USE OR PERFORMANCE OF THIS SOFTWARE.
 */

/*
  Jeff Francis
  jeff@gritch.org
  $Id: cgps.c 3265 2006-03-13 00:55:08Z garyemiller $

  Kind of a curses version of xgps for use with gpsd.
*/

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include <math.h>
#include <errno.h>
#include <sys/select.h>
#include <sys/socket.h>

#include <ncurses.h>                                                         
#include <signal.h>

#include "gps.h"

/* Macro for declaring function arguments unused. */
#if defined(__GNUC__)
#  define UNUSED __attribute__((unused)) /* Flag variable as unused */
#else /* not __GNUC__ */
#  define UNUSED
#endif

static struct gps_data_t *gpsdata;
static time_t timer;	/* time of last state change */
static int state = 0;	/* or MODE_NO_FIX=1, MODE_2D=2, MODE_3D=3 */
static float altfactor = METERS_TO_FEET;
static float speedfactor = MPS_TO_MPH;
static char *altunits = "ft";
static char *speedunits = "mph";

static WINDOW *datawin, *satellites, *messages, *command, *status;

int silent_flag=0;
int ops_flag=0;

/* Function to call when we're all done.  Does a bit of clean-up. */
static void die(int sig UNUSED) 
{
    /* Ignore signals. */
    (void)signal(SIGINT,SIG_IGN);
    (void)signal(SIGHUP,SIG_IGN);

    /* Move the cursor to the bottom left corner. */
    (void)mvcur(0,COLS-1,LINES-1,0);

    /* Put input attributes back the way they were. */
    (void)echo();

    /* Done with curses. */
    (void)endwin();

    /* We're done talking to gpsd. */
    (void)gps_close(gpsdata);

    /* Bye! */
    exit(0);
}


static enum deg_str_type deg_type = deg_dd;

/* This gets called once for each new sentence. */
static void update_panel(struct gps_data_t *gpsdata, 
			 char *message,
			 size_t len, 
			 int level UNUSED)
{
    int i;
    int newstate;
    char *s;

    /* This is for the satellite status display.  Lifted almost verbatim
       from xgps.c. */
    if (gpsdata->satellites) {
      for (i = 0; i < (MAXCHANNELS - 2); i++) {
	    (void)wmove(satellites, i+2, 1);
	    if (i < gpsdata->satellites) {
	      (void)wprintw(satellites," %3d    %02d    %03d    %02d      %c  ",
		       gpsdata->PRN[i],
		       gpsdata->elevation[i], gpsdata->azimuth[i], 
		       gpsdata->ss[i],	gpsdata->used[i] ? 'Y' : 'N');
	    } else {
	      (void)wprintw(satellites,"                              ");
	    }
	}
    }
  
  /* Print time/date. */
    (void)wmove(datawin, 1,17);
    if (isnan(gpsdata->fix.time)==0) {
	char scr[128];
	(void)wprintw(datawin,"%s",unix_to_iso8601(gpsdata->fix.time, scr, (int)sizeof(s)));
    } else
      (void)wprintw(datawin,"n/a         ");

    /* Fill in the latitude. */
    (void)wmove(datawin, 2,17);
    if (gpsdata->fix.mode >= MODE_2D) {
        s = deg_to_str(deg_type,  fabs(gpsdata->fix.latitude));
	(void)wprintw(datawin,"%s %c     ", s, (gpsdata->fix.latitude < 0) ? 'S' : 'N');
    } else
	(void)wprintw(datawin,"n/a         ");

    /* Fill in the longitude. */
    (void)wmove(datawin, 3,17);
    if (gpsdata->fix.mode >= MODE_2D) {
        s = deg_to_str(deg_type,  fabs(gpsdata->fix.longitude));
	(void)wprintw(datawin,"%s %c     ", s, (gpsdata->fix.longitude < 0) ? 'W' : 'E');
    } else
	(void)wprintw(datawin,"n/a         ");

    /* Fill in the altitude. */
    (void)wmove(datawin, 4,17);
    if (gpsdata->fix.mode == MODE_3D)
	(void)wprintw(datawin,"%.1f %s     ",gpsdata->fix.altitude*altfactor, altunits);
    else
	(void)wprintw(datawin,"n/a         ");

    /* Fill in the speed */
    (void)wmove(datawin, 5,17);
    if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.track)==0)
	(void)wprintw(datawin,"%.1f %s     ", gpsdata->fix.speed*speedfactor, speedunits);
    else
	(void)wprintw(datawin,"n/a         ");

    /* Fill in the heading. */
    (void)wmove(datawin, 6,17);
    if (gpsdata->fix.mode >= MODE_2D && isnan(gpsdata->fix.track)==0)
	(void)wprintw(datawin,"%.1f degrees     ", gpsdata->fix.track);
    else
	(void)wprintw(datawin,"n/a         ");

  /* Fill in the rate of climb. */
    (void)wmove(datawin, 7,17);
  if (gpsdata->fix.mode == MODE_3D && isnan(gpsdata->fix.climb)==0)
    (void)wprintw(datawin,"%.1f %s/min     "
		  , gpsdata->fix.climb * altfactor * 60, altunits);
  else
    (void)wprintw(datawin,"n/a         ");
  
  /* Fill in the estimated horizontal position error. */
  (void)wmove(datawin, 9,22);
    if (isnan(gpsdata->fix.eph)==0)
    (void)wprintw(datawin,"+/- %d %s     ", (int) (gpsdata->fix.eph * altfactor), altunits);
    else
	(void)wprintw(datawin,"n/a         ");

    /* Fill in the estimated vertical position error. */
  (void)wmove(datawin, 10,22);
    if (isnan(gpsdata->fix.epv)==0)
    (void)wprintw(datawin,"+/- %d %s     ", (int)(gpsdata->fix.epv * altfactor), altunits);
    else
	(void)wprintw(datawin,"n/a         ");

  /* Fill in the estimated track error. */
  (void)wmove(datawin, 11,22);
  if (isnan(gpsdata->fix.epd)==0)
    (void)wprintw(datawin,"+/- %.1f deg     ", (gpsdata->fix.epd));
    else
	(void)wprintw(datawin,"n/a          ");
  
  /* Fill in the estimated speed error. */
  (void)wmove(datawin, 12,22);
  if (isnan(gpsdata->fix.eps)==0)
    (void)wprintw(datawin,"+/- %d %s     ", (int)(gpsdata->fix.eps * speedfactor), speedunits);
  else
    (void)wprintw(datawin,"n/a            ");

  /* Fill in the GPS status and the time since the last state change. */
  (void)wmove(status, 1,10);
    if (gpsdata->online == 0) {
	newstate = 0;
    (void)wprintw(status,"OFFLINE          ");
    } else {
	newstate = gpsdata->fix.mode;
	switch (gpsdata->fix.mode) {
	case MODE_2D:
      (void)wprintw(status,"2D %sFIX (%d secs)   ",(gpsdata->status==STATUS_DGPS_FIX)?"DIFF ":"", (int) (time(NULL) - timer));
	    break;
	case MODE_3D:
      (void)wprintw(status,"3D %sFIX (%d secs)   ",(gpsdata->status==STATUS_DGPS_FIX)?"DIFF ":"", (int) (time(NULL) - timer));
	    break;
	default:
      (void)wprintw(status,"NO FIX (%d secs)     ", (int) (time(NULL) - timer));
	    break;
	}
    }

    /* Be quiet if the user requests silence. */
    if(silent_flag==0) {
      (void)wprintw(messages, "%s\n", message);
    }

  /* Update the screen.  This is admittedly not an optimal hack, and
     is for fixing the screen flashing when using NMEA data sources.
     We're watching for a GPGSA sentence in order to refresh the data
     window.  There is a small race condition here (ie, data
     structures can update before the refresh happens), but this
     rarely happens, and it doesn't matter much if it does (ie, can't
     corrupt anything important).  There's probably a cleaner way to
     do this, but it's better than the annoying flashing. */
  if((ops_flag==0) || (strstr(message,"GSA")!=NULL)) {
    (void)wrefresh(datawin);

    /* Reset the timer if the state has changed. */
    if (newstate != state) {
      timer = time(NULL);
      state = newstate;
    }
  }

  (void)wrefresh(status);
    (void)wrefresh(satellites);
    (void)wrefresh(messages);
    (void)wrefresh(command);
}

static void usage( char *prog) 
{
	    (void)fprintf(stderr, 
		"Usage: %s [-h] [-v] [-V] [-l {d|m|s}] [server[:port:[device]]]\n\n"
		"  -h          Show this help, then exit\n"
		"  -v          Show version, then exit\n"
		"  -V          Show version, then exit\n"
		"  -s          Be silent (don't print raw dgps data)\n"
		"  -o          Fix screen flash for NMEA (experimental)\n"
		"  -l {d|m|s}  Select lat/lon format\n"
		"                d = DD.dddddd\n"
		"                m = DD MM.mmmm'\n"
		"                s = DD MM' SS.sss\"\n"
                , prog);

	    exit(1);
}

int main(int argc, char *argv[])
{
    int option;
    char *arg = NULL, *colon1, *colon2, *device = NULL, *server = NULL, *port = DEFAULT_GPSD_PORT;
    char *err_str = NULL;
    int c;

    struct timeval timeout;
    fd_set rfds;
    int data;

    /* Process the options.  Print help if requested. */
  while ((option = getopt(argc, argv, "hVl:so")) != -1) {
	switch (option) {
    case 'o':
      ops_flag=1;
      break;
	case 's':
	  silent_flag=1;
	  break;
	case 'V':
	    (void)fprintf(stderr, "SVN ID: $Id: cgps.c 3265 2006-03-13 00:55:08Z garyemiller $ \n");
	    exit(0);
	case 'l':
	    switch ( optarg[0] ) {
	    case 'd':
		    deg_type = deg_dd;
		    continue;
	    case 'm':
		    deg_type = deg_ddmm;
		    continue;
	    case 's':
		    deg_type = deg_ddmmss;
		    continue;
            default:
		    (void)fprintf(stderr, "Unknown -l argument: %s\n", optarg);
		    /*@ -casebreak @*/
            }
	case 'h': default:
	    usage(argv[0]);
	    break;
	}
    }

    /* Grok the server, port, and device. */
    /*@ -branchstate @*/
    if (optind < argc) {
	arg = strdup(argv[optind]);
	/*@i@*/colon1 = strchr(arg, ':');
	server = arg;
	if (colon1 != NULL) {
	    if (colon1 == arg)
		server = NULL;
	    else
		*colon1 = '\0';
	    port = colon1 + 1;
	    colon2 = strchr(port, ':');
	    if (colon2 != NULL) {
		if (colon2 == port)
		    port = NULL;
		else
		    *colon2 = '\0';
		device = colon2 + 1;
	    }
	}
	colon1 = colon2 = NULL;
    }
    /*@ +branchstate @*/

    /*@ -observertrans @*/
    switch (gpsd_units())
    {
    case imperial:
	altfactor = METERS_TO_FEET;
	altunits = "ft";
	speedfactor = MPS_TO_MPH;
	speedunits = "mph";
	break;
    case nautical:
	altfactor = METERS_TO_FEET;
	altunits = "ft";
	speedfactor = MPS_TO_KNOTS;
	speedunits = "knots";
	break;
    case metric:
	altfactor = 1;
	altunits = "m";
	speedfactor = MPS_TO_KPH;
	speedunits = "kph";
	break;
    default:
	/* leave the default alone */
	break;
    }
    /*@ +observertrans @*/

    /* Open the stream to gpsd. */
    /*@i@*/gpsdata = gps_open(server, port);
    if (!gpsdata) {
	switch ( errno ) {
	case NL_NOSERVICE: 	err_str = "can't get service entry"; break;
	case NL_NOHOST: 	err_str = "can't get host entry"; break;
	case NL_NOPROTO: 	err_str = "can't get protocol entry"; break;
	case NL_NOSOCK: 	err_str = "can't create socket"; break;
	case NL_NOSOCKOPT: 	err_str = "error SETSOCKOPT SO_REUSEADDR"; break;
	case NL_NOCONNECT: 	err_str = "can't connect to host"; break;
	default:             	err_str = "Unknown"; break;
	}
	(void)fprintf( stderr, 
		       "cgps: no gpsd running or network error: %d, %s\n", 
		       errno, err_str);
	exit(2);
    }

    /* Update the timestamp (used to keep track of time since last state
       change). */
    timer = time(NULL);

    /* Set up the curses screen (if using curses). */
    (void)initscr();
    (void)noecho();
    (void)signal(SIGINT,die);
    (void)signal(SIGHUP,die);

    /*@ -onlytrans @*/
    datawin    = newwin(15, 45, 1, 0);
    satellites = newwin(15, 35, 1, 45);
    command    = newwin(3,  45,  16, 0);
    status     = newwin(3,  35,  16, 45);
    messages   = newwin(0,  0,  19, 0);
    /*@ +onlytrans @*/
    (void)scrollok(messages, true);
    (void)wsetscrreg(messages, 0, LINES-13);
    (void)nodelay(messages,(bool)TRUE);

    (void)mvprintw(0, 31, "CGPS Test Client");
    (void)refresh();

    /* Do the initial field label setup. */
    (void)mvwprintw(datawin, 1,5, "Time:");
    (void)mvwprintw(datawin, 2,5, "Latitude:");
    (void)mvwprintw(datawin, 3,5, "Longitude:");
    (void)mvwprintw(datawin, 4,5, "Altitude:");
    (void)mvwprintw(datawin, 5,5, "Speed:");
    (void)mvwprintw(datawin, 6,5, "Heading:");
    (void)mvwprintw(datawin, 7,5, "Climb:");
    (void)mvwprintw(datawin, 9,5, "Horizontal Err:");
    (void)mvwprintw(datawin, 10,5, "Vertical Err:");
    (void)mvwprintw(datawin, 11,5, "Course Err:");
    (void)mvwprintw(datawin, 12,5, "Speed Err:");
    (void)mvwprintw(status, 1,1, "Status:");
    (void)wborder(datawin, 0, 0, 0, 0, 0, 0, 0, 0);
    (void)mvwprintw(satellites, 1,1, "PRN:   Elev:  Azim:  SNR:  Used:");
    (void)wborder(satellites, 0, 0, 0, 0, 0, 0, 0, 0);
    (void)mvwprintw(command, 1,1, "Command:  ");
    (void)wborder(command, 0, 0, 0, 0, 0, 0, 0, 0);
    (void)wborder(status, 0, 0, 0, 0, 0, 0, 0, 0);

    /* Here's where updates go. */
    gps_set_raw_hook(gpsdata, update_panel);

    /* If the user requested a specific device, try to change to it. */
    if (device) {
	char *channelcmd = (char *)malloc(strlen(device)+3);

	if (channelcmd) {
	    /*@i@*/(void)strcpy(channelcmd, "F=");
	    (void)strcpy(channelcmd+2, device);
	    (void)gps_query(gpsdata, channelcmd);
	    (void)free(channelcmd);
	}
    }

    /* Request "w+x" data from gpsd. */
    (void)gps_query(gpsdata, "w+x\n");

    for (;;) { /* heart of the client */
    
        /* watch to see when it has input */
        FD_ZERO(&rfds);
        FD_SET(gpsdata->gps_fd, &rfds);

	/* wait up to five seconds. */
	timeout.tv_sec = 5;
	timeout.tv_usec = 0;

	/* check if we have new information */
	data = select(gpsdata->gps_fd + 1, &rfds, NULL, NULL, &timeout);
	
	if (data == -1) {
	    fprintf( stderr, "cgps: socket error\n");
	    exit(2);
	}
	else if( data ) {
	    /* code that calls gps_poll(gpsdata) */
	  (void)gps_poll(gpsdata);
	}

        /* Check for user input. */
        c=wgetch(messages);
        
	switch ( c ) {
	  /* Quit */
	case 'q':
	  die(0);
	  break;

	  /* Toggle "once per second" update. */
	case 'o':
	  if(ops_flag==0) {
	    ops_flag=1;
	  } else {
	    ops_flag=0;
	  }
        break;

	  /* Toggle spewage of raw gpsd data. */
	case 's':
	  if(silent_flag==0) {
	    silent_flag=1;
	  } else {
	    silent_flag=0;
	  }
	  break;

	  /* Clear the spewage area. */
	case 'c':
	    (void)werase(messages);
	  break;

	default:
	  break;
	}

    }
 
}
