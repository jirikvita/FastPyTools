#!/usr/bin/python
# jk Thu 28 Mar 17:35:07 CET 2019

import ROOT
from math import sqrt, pow, log, exp
import os, sys

cans = []
stuff = []
funs = []
##########################################
def RebinFitAndDraw(histo, rebin = 10, opt = 'hist', range1 = 0, range2 = 0.5):
    x1 = histo.GetXaxis().GetXmin()
    x2 = histo.GetXaxis().GetXmax()
    width = x2 - x1
    if range1 + range2 > 1.:
        range1 = 0.
        range2 = 0.
    funname = 'fit_{}'.format(histo.GetName())
    fx1 = x1 + range1*width
    fx2 = x2 - range2*width
    fitfun = ROOT.TF1(funname, '[0]*exp(-(x-[1])^2/(2*[2]^2))', fx1, fx2)
    symfun = ROOT.TF1(funname + '_sym', '[0]*exp(-(x-[1])^2/(2*[2]^2))', fx2, x2)
    symfun.SetLineStyle(2)
    histo.Rebin(rebin)
    ROOT.gPad.SetLogy()
    histo.Draw(opt)

    fitfun.SetParameters(histo.Integral()/3., 0., histo.GetRMS())
    fitfun.FixParameter(1, 0.)
    histo.Fit(funname, '', '', fx1, fx2)
    for i in range(0,fitfun.GetNpar()):
        symfun.SetParameter(i, fitfun.GetParameter(i))
    
    fitfun.Draw('same')
    symfun.Draw('same')

    chi2 = fitfun.GetChisquare()
    ndf = fitfun.GetNDF()
    txt = ROOT.TLatex(0.14, 0.84, '#chi^{2}/ndf = ' + '{:1.2f}'.format(chi2/ndf))
    txt.SetNDC()
    txt.Draw()
    stuff.append(txt)
    #histo.Draw(opt)
    funs.append(fitfun)
    funs.append(symfun)
    return

    

##########################################

##########################################
##########################################
##########################################

# https://www.tutorialspoint.com/python/python_command_line_arguments.htm
def main(argv):
    if len(sys.argv) < 2:
        print('Usage: {} histos.root'.format(sys.argv[0]))
        return 1
    filename = sys.argv[1]
    filename = 'histos_FAST_2018_02_08_04h40m48s_trig0.root'
    rfile = ROOT.TFile(filename, 'read')
    hname = 'histo_h'


    for iPMT in range(-1,12):
        tag = 'all'
        if iPMT >= 0:
            tag = iPMT-1
      
        h_signal = rfile.Get('h_signal_PMT{}'.format(tag))
        h_integral = rfile.Get('h_integral_PMT{}'.format(tag))
        h_integral_vs_signal = rfile.Get('h_integral_vs_signal_PMT{}'.format(tag))

        try:

            print('Entries: {} {} {}'.format(h_signal.GetEntries(),  h_integral.GetEntries(),  h_integral_vs_signal.GetEntries() ) )

            canname = 'can_iPMT_{}'.format(tag)
            can = ROOT.TCanvas(canname, canname)
            can.Divide(2,2)
            cans.append(can)

            can.cd(1)
            RebinFitAndDraw(h_signal)
            can.cd(2)
            RebinFitAndDraw(h_integral)
            can.cd(3)
            h_integral_vs_signal.Draw("colz")

            stuff.append([h_signal, h_integral_vs_signal, h_integral])
            print('Plotted signal and integral stuff for {}'.format(tag))
        except:
            print('Failed plotting stiff fot PMT {}'.format(iPMT))
            pass
            
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

