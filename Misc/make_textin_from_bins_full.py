#!/usr/bin/env python3
import os
import sys
import numpy
import matplotlib.pyplot as plt

if len(sys.argv) != 3:
    print("usage: make_textin_from_bins.py [bin dir] [output_txtin_bin]")
    sys.exit(1)


file = open(sys.argv[2],"wb")

print(len(sys.argv))

for binfile in os.listdir(sys.argv[1]):
    bytesnew = binfile.replace('.bin','')
    b = bytearray.fromhex(bytesnew)
    file.write(b)
	
