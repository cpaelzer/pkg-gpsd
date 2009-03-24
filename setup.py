# $Id$
# Creates build/lib.linux-${arch}-${pyvers}/gpspacket.so,
# where ${arch} is an architecture and ${pyvers} is a Python version.

from distutils.core import setup, Extension

import os
import sys

# For VPATH builds, this script must be run from $(srcdir) with the
# abs_builddir environment variable set to the location of the build
# directory.  This is necessary because Python's distutils package
# does not have built-in support for VPATH builds.

# These dependencies are enforced here and not in the Makefile to make
# it easier to build the Python parts without building everything else
# (the user can run 'python setup.py' without having to run 'make').
needed_files = ['gpsd.h', 'packet_names.h', 'gpsfake.1', 'gpsprof.1']
created_files = []

if not 'clean' in sys.argv:
    abs_builddir = os.environ["abs_builddir"] if "abs_builddir" in os.environ else ""
    if not os.path.exists(os.path.join(abs_builddir, 'gpsd_config.h')):
        sys.stderr.write('\nPlease run configure first!\n')
        sys.exit(1)

    cdcmd = ("cd '" + abs_builddir + "' && ") if abs_builddir else ""
    for f_name in needed_files:
        # TODO:  Shouldn't make be run unconditionally in case a
        # dependency of f_name has been updated?
        if not os.path.exists(os.path.join(abs_builddir, f_name)):
            cmd = cdcmd + "make '" + f_name + "'"
            print cmd
            make_out = os.popen(cmd)
            print make_out.read()
            if make_out.close():
                sys.exit(1)
            created_files.append(f_name)

gpspacket_sources = ["gpspacket.c", "packet.c", "isgps.c",
            "driver_rtcm2.c", "strl.c", "hex.c", "crc24q.c"]

setup( name="gpsd",
       version="1.0",
       ext_modules=[
    	Extension("gpspacket", gpspacket_sources),
    	Extension("gpslib", ["gpslib.c", "geoid.c"])
        ],
       py_modules = ['gpsfake','gps', 'leapsecond'],
       data_files=[('bin', ['gpsfake','gpsprof']),
           ('share/man/man1', ['gpsfake.1','gpsprof.1'])]
     )
