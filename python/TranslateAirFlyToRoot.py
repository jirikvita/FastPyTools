#!/usr/bin/python

# jk 8.7.2019

from __future__ import print_function

import os, sys

# Auger:
#indir='/data/FAST/Auger/'
#outdir='/data/FAST/Auger_ROOT/'

#TA:
indir='/data/FAST/TA/'
outdir='/data/FAST/TA_ROOT/'

for lrun in os.popen('cd {} ; ls | grep run'.format(indir)):
    run = lrun[:-1]
    #print(run)
    os.system('mkdir -p {}/{}'.format(outdir, run))
    for linfile in os.popen('ls {}/{}/*.data*'.format(indir, run)):
        infile = linfile[:-1]
        #print('  {}'.format(infile))
        outfile = infile
        outfile = outfile.replace('.data', '.root').replace(indir, outdir).replace('.root.part', '.part.root')
        command='./AirFly2FASTEventFile {} {}'.format(infile, outfile)
        print(command)
        os.system(command)
