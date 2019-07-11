# jk 2019

import ROOT
from math import sqrt, pow, log, exp, pi, fabs
import os, sys


cans = []
stuff = []
Hists = []
cpHists = []
Txts = []
Legs = []
Convs = []
Corrs = []
Funs = []

def CheckSameBins(h1, h2):
    return  h1.GetNbinsX() == h2.GetNbinsX()

def MakeYProjection(h):
    return 

# 30.6.2019:
def TranslateToMicroseconds(ch1):
    # conversion from bins to microseconds:
    # one bin is 20 ns
    x1 = ch1.GetXaxis().GetXmin()# * 20. / 1000. not needed anymore after moving to Justin's format!
    x2 = ch1.GetXaxis().GetXmax()# * 20. / 1000.
    h1 = ROOT.TH1D(ch1.GetName() + '_mus', ch1.GetTitle() + ';t [#mus];N_{pe}', ch1.GetNbinsX(), x1, x2)
    n = ch1.GetNbinsX()
    for i in range(1,n+1):
        h1.SetBinContent(i, ch1.GetBinContent(i))
        h1.SetBinError(i, ch1.GetBinError(i))
    h1.SetLineColor(ch1.GetLineColor())
    h1.SetLineStyle(ch1.GetLineStyle())
    h1.SetLineWidth(ch1.GetLineWidth())
    h1.SetMarkerColor(ch1.GetMarkerColor())
    h1.SetMarkerSize(ch1.GetMarkerSize())
    h1.SetFillStyle(ch1.GetFillStyle())
    stuff.append(h1)
    h1.Scale(1.)
    return h1

##########################################
# https://docs.scipy.org/doc/numpy/reference/generated/numpy.convolve.html
def Convolve(self, h2):
    #print('  In Convolve...')
    if not CheckSameBins(self, h2):
        print('Convolve ERROR: histograms of nonequal number of bins provided!')
        return
    n = self.GetNbinsX()
    conv = self.Clone(self.GetName() + '_conv_' + h2.GetName())
    conv.Reset()
    for i in range(0,n):
        sum = 0.
        err = 0.
        for j in range(0,n):
            m = i-j+1
            if m < 1 or m > n:
                continue
            val1 = self.GetBinContent(j+1)
            val2 = h2.GetBinContent(m)
            sum = sum + val1*val2
            if fabs(val1)>0 and fabs(val2) > 0:
                # err = err + fabs(val1)/pow(val2*val2, 2) + fabs(val2)/pow(val1*val1, 2)
                err = err + fabs(val1)*pow(val2, 2) + fabs(val2)*pow(val1, 2)
        conv.SetBinContent(i+1, sum)
        if fabs(err) > 0:
            conv.SetBinError(i+1, sqrt(fabs(err)))
    conv.Scale(1.)
    return conv

##########################################
# https://stackoverflow.com/questions/972/adding-a-method-to-an-existing-object-instance
ROOT.TH1D.Convolve = Convolve

##########################################

def FindAndSetGlobalMax(hs, sf = 1.2):
    print('IN FUNCTION FindAndSetGlobalMax')
    maxval = -1e3
    minval = +1e3
    for i in range(0, len(hs)):
        h = hs[i]
        try:
            h.Scale(1.)
        except:
            print('ERROR scaling histo {}'.format(i))
            continue
        print('   checking histo {}'.format(h.GetName()))
        valmax = h.GetMaximum()
        valmin = h.GetMinimum()
        if valmax > maxval:
            maxval = valmax
        if valmin < minval:
            minval = valmin
    for i in range(0, len(hs)):
        h = hs[i]
        try:
            h.SetMaximum(maxval*sf)
            if minval < 0.:
                h.SetMinimum(minval*sf)
            else:
                h.SetMinimum(minval/sf)
        except:
            print('ERROR scaling histo {}'.format(i))

    return 
