#!/usr/bin/env python3
import sys
import os
import struct
import numpy
import scipy.io.wavfile

if len(sys.argv) < 3:
    print("usage: csv2bin.py [csv dir] [bin dir]")
    sys.exit(1)

i = 0
for filename in os.listdir(sys.argv[1]):
    fname, ext = os.path.splitext(filename)
    chan0, chan1 = numpy.loadtxt(os.path.join(sys.argv[1], filename), delimiter=',', skiprows=5, unpack=True)
    outfile = open(os.path.join(sys.argv[2], str(i) + "_" + fname + ".bin"), "wb")
    i= i+1
    for line in chan0:
        outfile.write(struct.pack('f', float(line)))
    outfile.close()

