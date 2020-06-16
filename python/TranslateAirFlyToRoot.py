#!/usr/bin/python

# jk 8.7.2019

from __future__ import print_function

import os, sys

# Auger:
indir='/data/FAST/Auger/'
outdir='/data/FAST/Auger_ROOT/'

#TA:
#indir='/data/FAST/TA/'
#outdir='/data/FAST/TA_ROOT/'

rebin = 1
evtId = 0

for lrun in os.popen('cd {} ; ls | egrep "run191020" '.format(indir)):
    # for lrun in os.popen('cd {} ; ls | egrep "19090|19091" '.format(indir)):
    # for lrun in os.popen('cd {} ; ls | egrep "run1910|1911|19092" '.format(indir)):
    run = lrun[:-1]
    #print(run)
    os.system('mkdir -p {}/{}'.format(outdir, run))
    for linfile in os.popen('ls {}/{}/*.data*'.format(indir, run)):
        # HACK to get only flasher files!!!
        if not 'flasher' in linfile:
            continue
        infile = linfile[:-1]
        #print('  {}'.format(infile))
        outfile = infile
        outfile = outfile.replace('.data', '.root').replace(indir, outdir).replace('.root.part', '.part.root')
        logfile = outfile + ''
        logfile = logfile.replace('.root', '.log')
        command='./AirFly2FASTEventFile {} {} {} {} > {}'.format(infile, evtId, rebin, outfile, logfile)
        print('Running: {}'.format(command))
        os.system(command)
