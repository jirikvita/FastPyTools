#!/usr/bin/python
# JK Tue 26 Mar 14:07:50 CET 2019

from __future__ import print_function

import ROOT
from math import sqrt, pow, log, exp
import os, sys
cans = []
stuff = []
Hists = []

col = [ROOT.kMagenta, ROOT.kCyan, ROOT.kYellow, ROOT.kOrange,
       ROOT.kPink]

##########################################
##########################################
##########################################


# https://www.tutorialspoint.com/python/python_command_line_arguments.htm
def main(argv):
    if len(sys.argv) < 2:
        print('Usage: {} histos.root [0/1 .. for nor/running TApplication]'.format(sys.argv[0]))
        return 1
    #ROOT.gStyle.SetOptStat(111111)
    ROOT.gStyle.SetOptFit(111)
    ROOT.gStyle.SetPalette(1)

    # MakeSeparateCanvases = True
    MakeSeparateCanvases = False
    
    filename = sys.argv[1]
    runapp = 1
    if len(sys.argv) > 2:
        runapp = int(sys.argv[2])
    if not runapp:
        ROOT.gROOT.SetBatch(1)
        
    rfile = ROOT.TFile(filename, 'read')

    hnames = ['h_Chi2OverNdf', 'h_Chi2OverNdfVsConstant', 'h_Chi2OverNdfVsMPV', 'h_Chi2OverNdfVsSigma']
   
    rfile.cd()
    
    print('Reading histos...')

    canname = 'Histos'
    can = ROOT.TCanvas(canname, canname,1400,1000)
    can.Divide(2,2)
    cans.append(can)

    ican = 1
    for hname in hnames:
        h = rfile.Get(hname)
        Hists.append(h)
        can.cd(ican)
        if h.InheritsFrom('TH2'):
            ROOT.gPad.SetLogz(1)
            h.SetStats(0)
            h.Draw('colz')
        else:
            h.SetLineColor(ROOT.kBlack)
            h.SetFillColor(col[ican-1])
            h.Draw('hist')
        ican = ican+1
        
    can.Print('Histos.png')

    if runapp:
        ROOT.gApplication.Run()
        
    return

###################################
###################################
###################################

if __name__ == "__main__":
    # execute only if run as a script"
    main(sys.argv)
    
###################################
###################################
###################################

