#!/usr/bin/env python
#
# With -t, report on which status masks are used in the daemon vs. the
# client-side library.
#
# With -p, dump a Python status mask list translated from the C one.
#
# With -c, generare C code to dump masks for debugging purposes.

import sys, commands, glob, getopt

class SourceExtractor:
    def __init__(self):
        self.daemonfiles = ["gpsd.c", "libgpsd_core.c", "pseudonmea.c"] + glob.glob("driver_*.c")
        self.masks = []
        self.primitive_masks = []
        for line in file("gps.h"):
            if line.startswith("#define") and "_SET" in line:
                fields = line.split()
                self.masks.append((fields[1], fields[2]))
                if fields[2].endswith("u"):
                    self.primitive_masks.append((fields[1], fields[2]))
    
    def in_library(self, flag):
        (status, output) = commands.getstatusoutput("grep %s libgps.c libgps_json.c" % flag)
        return status == 0

    def in_daemon(self, flag):
        (status, output) = commands.getstatusoutput("grep %s %s" % (flag, " ".join(self.daemonfiles)))
        return status == 0

if __name__ == '__main__':
    try:
        (options, arguments) = getopt.getopt(sys.argv[1:], "ptc")
        tabulate = False
        pythonize = False
        codegen = False
        for (switch, val) in options:
            if (switch == '-t'):
                tabulate = True
            if (switch == '-p'):
                pythonize = True
            if (switch == '-c'):
                codegen = True

        source = SourceExtractor()

        if tabulate:
            print "%-14s	%8s %8s" % (" ", "Library", "Daemon")
            for (flag, value) in source.masks:
                print "%-14s	%8s %8s" % (flag, source.in_library(flag), source.in_daemon(flag))
        if pythonize:
            for (d, v) in source.masks:
                if v[-1] == 'u':
                    v = v[:-1]
                print "%-15s\t= %s" % (d, v)
        if codegen:
            maxout = 0
            for (d, v) in source.primitive_masks:
                if source.in_daemon(d):
                    stem = d
                    if stem.endswith("_SET"):
                        stem = stem[:-4]
                    maxout += len(stem) + 1
            print """/* This code is generated.  Do not hand-hack it! */
#include <stdio.h>
#include <string.h>

#include \"gpsd_config.h\"	/* for strlcat() */
#include \"gpsd.h\"

const char *gpsd_maskdump(gps_mask_t set)
{
    static char buf[%d];
    const struct {
	gps_mask_t      mask;
	const char      *name;
    } *sp, names[] = {"""  % (maxout + 3,)
            for (flag, value) in source.primitive_masks:
                stem = flag
                if stem.endswith("_SET"):
                    stem = stem[:-4]
                print "\t{%s,\t\"%s\"}," % (flag, stem)
            print '''\
    };

    memset(buf, '\0', sizeof(buf));
    buf[0] = '{';
    for (sp = names; sp < names + sizeof(names)/sizeof(names[0]); sp++)
	if ((set & sp->mask)!=0) {
	    (void)strlcat(buf, sp->name, sizeof(buf));
	    (void)strlcat(buf, "|", sizeof(buf));
	}
    if (buf[1] != \'\\0\')
	buf[strlen(buf)-1] = \'\\0\';
    (void)strlcat(buf, "}", sizeof(buf));
    return buf;
}
'''

    except KeyboardInterrupt:
        pass



