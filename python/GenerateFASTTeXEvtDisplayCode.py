#!/usr/bin/python
# Sat 29 Jun 16:45:17 CEST 2019

from __future__ import print_function

import ROOT
from math import sqrt, pow, log, exp
import os, sys, getopt

cans = []
stuff = []

##########################################
# https://www.tutorialspoint.com/python/python_command_line_arguments.htm
def main(argv):
    #if len(sys.argv) > 1:
    #  foo = sys.argv[1]

    ### https://www.tutorialspoint.com/python/python_command_line_arguments.htm
    ### https://pymotw.com/2/getopt/
    ### https://docs.python.org/3.1/library/getopt.html
    gBatch = False
    gTag=''
    print(argv[1:])
    try:
        # options that require an argument should be followed by a colon (:).
        opts, args = getopt.getopt(argv[2:], 'hbt:', ['help','batch','tag='])

        print('Got options:')
        print(opts)
        print(args)
    except getopt.GetoptError:
        print('Parsing...')
        print ('Command line argument error!')
        print('{:} [ -h -b --batch -tTag --tag="MyCoolTag"]]'.format(argv[0]))
        sys.exit(2)
    for opt,arg in opts:
        print('Processing command line option {} {}'.format(opt,arg))
        if opt == '-h':
            print('{:} [ -h -b --batch -tTag --tag="MyCoolTag"]'.format(argv[0]))
            sys.exit()
        elif opt in ("-b", "--batch"):
            gBatch = True
        elif opt in ("-t", "--tag"):
            gTag = arg
            print('OK, using user-defined histograms tag for output pngs {:}'.format(gTag,) )


    print('*** Settings:')
    print('tag={:},'.format(gTag, ))

    pdfdir = 'pdf/'
    texdir = 'tex/'
    os.system('mkdir -p {}'.format(texdir))

    zooms = ['', '_zoom', '_ultrazoom']

    for zoom in zooms:
    
        for sline in os.popen('cd {}/ ; ls PMTT*.pdf'.format(pdfdir)).readlines():
            line = sline[:-1]
            #print(line)
            timetag = line
            timetag = timetag.replace('PMTTimes_histo_FAST_','').replace('_ultrazoom','').replace('_zoom','').replace('.pdf','')
            print(timetag)
            textime = timetag
            textime = textime.replace('_',' ')

            fname = '{}evtdispl_{}{}.tex'.format(texdir,timetag, zoom)
            texfile = open(fname, 'w')
            texfile.write('% _____________________________________________________________________ %\n')
            texfile.write('\\frame{\n')
            texfile.write(' \\frametitle{Event ' + textime+ '}\n')
            texfile.write(' \\vskip-0.3cm\n')
            texfile.write(' \\begin{tabular}{cc}\n')
            texfile.write('   \\includegraphics[width=0.45\\textwidth]{pdf/PMTTimes_histo_FAST_' + timetag + zoom + '.pdf} &\n')
            texfile.write('   \\includegraphics[width=0.45\\textwidth]{pdf/EventDispl_histo_FAST_' + timetag + zoom + '.pdf} \\\\ \n')
            texfile.write(' \\end{tabular}\n')
            texfile.write('}\n')
            texfile.write('\n')
            texfile.close()

    
    
###################################
###################################
###################################

if __name__ == "__main__":
    # execute only if run as a script"
    main(sys.argv)
    
###################################
###################################
###################################

