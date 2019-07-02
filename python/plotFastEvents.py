#!/usr/bin/python
# JK Tue 26 Mar 14:07:50 CET 2019, April 2019

from __future__ import print_function

from fastUtils import *

"""
TODO: 
write TeX dumping code to prepare an event display over 1 or 2 A4's max.:
EventDispl_histo_FAST_2018_05_15_09h25m50s_trig0_evt126_Thr4.png
PMTTimes_histo_FAST_2018_05_15_09h25m50s_trig0_evt126_Thr4.png
ConvPMT_histo_FAST_2018_05_15_09h25m50s_trig0_evt126_Thr4.png
CorrPMT_histo_FAST_2018_05_15_09h25m50s_trig0_evt126_Thr4.png


DONE:
pmt correlation plot: TH2D of pairs of counts over time! ;-)

done:
Make autocorrelation functions and require their integral in at least two neighouring PMTs to be over some thr
Look at Aij/sqrt(Aii*Ajj) where Aij are areas of a peak of correlatoion functions.

TODO:
add timestamp from file name into all plots!!!

TBC:
sort PMTs by number in overall plot!
 --- done, but check the Event display now...

"""

EventsAnalyzed = {}

nMaxPMTs = 12

# based on https://air.uchicago.edu/fast/images/c/c0/FASTNUM.pdf
PMTPositionTelescopeViewTA = { 0: 7, 1: 1, 2: 8, 3: 2, 4: 9, 5: 3, 6: 10, 7: 4, 8: 11, 9: 5, 10: 12, 11: 6}
PMTPositionSkyViewTA =       { 0: 1, 1: 7, 2: 2, 3: 8, 4: 3, 5: 9, 6: 4, 7: 10, 8: 5, 9: 11, 10: 6, 11: 12}
PMTPositionSkyViewAuger =       { 0: 2, 1: 4, 2: 1, 3: 3 }

pngdir = 'png/'
pdfdir = 'pdf/'

# TODO:
NeighbouringMatrix = []


PMTcol = [ROOT.kBlack, ROOT.kRed, ROOT.kBlue, ROOT.kGreen + 2,
          ROOT.kCyan+2, ROOT.kMagenta+2, ROOT.kOrange, ROOT.kPink,
          ROOT.kTeal, ROOT.kGreen, ROOT.kBlue+2, ROOT.kYellow+3]

##########################################
def PlotPMTsOnSingleCanvas(hPMTs):
    canname = 'GlobalPMTcanvas'
    gcan = ROOT.TCanvas(canname, canname, 200, 200, 1000, 1000)
    cans.append(gcan)
    
    FindAndSetGlobalMax(hPMTs)
    gcan.cd()
    opt = ''

    pmtleg = ROOT.TLegend(0.17+0.55, 0.60, 0.35+0.55, 0.88)
    pmtleg.SetFillStyle(0)
    pmtleg.SetBorderSize(0)
    Legs.append(pmtleg)
        
    for ipmt in hPMTs:
        hpmt = hPMTs[ipmt]
        hpmt.SetLineColor(PMTcol[ipmt] )
        if not 'Thr1' in hpmt.GetTitle():
            hpmt.SetLineWidth(2)
            #hpmt.SetLineStyle(2)
        else:
            hpmt.SetLineWidth(3)
            hpmt.SetLineStyle(1)

        hpmt.SetFillStyle(0)
        hpmt.SetFillColor(0)
        datestr = hpmt.GetTitle().split('|')[-1]
        hpmt.SetTitle('UTC ' + datestr)
        hpmt.Draw('hist' + opt)
        opt = 'same'
        pmtleg.AddEntry(hpmt, 'PMT{}'.format(ipmt), 'L')

    pmtleg.Draw()
    return gcan

##########################################
def GetMaxPeakArea(h, n = 1.):
    x = h.GetXaxis().GetBinCenter(h.GetMaximumBin())
    x1 = h.GetXaxis().GetXmin()
    x2 = h.GetXaxis().GetXmax()
    funname = 'gfit_' + h.GetName()
    fun = ROOT.TF1(funname, '[0]*exp(-(x-[1])^2/(2*[2]^2)) + [3]', x1, x2)
    fun.SetLineStyle(2)
    Funs.append(fun)
    rms = 10. # h.GetRMS()/2.
    fun.SetParameters(h.GetMaximum()/(sqrt(2.*pi)), x, rms, 1.)
    print('GetMaxPeakArea Initial fit pars: ', end='')
    for ip in range(0,fun.GetNpar()):
        print('{} '.format(fun.GetParameter(ip)), end='')
    print('')
    y1 = x - n*rms
    y2 = x + n*rms
    h.Fit(funname, '', '', y1, y2)
    h.Fit(funname, '', '', y1, y2)
    h.Fit(funname, '', '', y1, y2)
    chi2 = fun.GetChisquare()
    ndf = fun.GetNDF()
    print('GetMaxPeakArea Final fit pars: ', end='')
    for ip in range(0,fun.GetNpar()):
        print('{} '.format(fun.GetParameter(ip)), end='')
    print(' chi2/ndf={}/{}'.format(chi2,ndf))

    fun.Draw('same')
    area = -1.
    print('GetMaxPeakArea x={}, [{},{}], chi2/ndf={:3.2f}/{}'.format(x,y1,y2,fun.GetChisquare(),fun.GetNDF()))

    fitOK = fun.GetParameter(0) > 1000. and fun.GetParameter(1) > x1 and fun.GetParameter(1) < x2 and fun.GetParameter(2) < rms*10
    chi2ndf = chi2
    if ndf > 0:
        chi2ndf = chi2 / ndf
    if fun.GetChisquare() > 0: # and fun.GetChisquare()/fun.GetNDF() < 5.:
        # compute the peak area : over the constant??
        area = fun.Integral(y1,y2)# - (y2-y1)*fun.GetParameter(3)
    return area, chi2ndf, fitOK

##########################################
def MakePMTConvText(h, ipmt, jpmt, size = 0.16, col = ROOT.kBlack, x = 0.16, y = 0.75, drawDetails = False):
    txt = ROOT.TLatex(x, y, '{}*{}'.format(ipmt, jpmt) )
    txt.SetNDC()
    txt.SetTextSize(size)
    txt.Draw()
    Txts.append(txt)
    val = h.GetBinContent(h.GetMaximumBin())
    if val > 0:
        val = sqrt(val)
    area,chi2ndf,fitOK = GetMaxPeakArea(h)
    sqrtarea = sqrt(fabs(area))
    if drawDetails:
        txt2 = ROOT.TLatex(x, 0.17, '#sqrt{m}=' + '{:.0f}'.format(val) )
        txt2.SetNDC()
        txt2.SetTextSize(size)
        txt2.Draw()
        Txts.append(txt2)

        txt3 = ROOT.TLatex(x, 0.17*2, '#sqrt{A}' + '={:.0f}'.format(sqrtarea) )
        txt3.SetNDC()
        txt3.SetTextSize(size)
        txt3.Draw()
        Txts.append(txt3)
    
    return val,area,chi2ndf,fitOK

##########################################
# get the value of the autocorrelation peak:
def FindArea(k, areas):
    for area in areas:
        i = area[1]
        j = area[2]
        if i == k and j == k:
            return area[0]
    return -1.

##########################################
def PlotConvolutionsOnSingleCanvas(hPMTs, x = 0.17, size = 0.16):
    print('In PlotConvolutionsOnSingleCanvas...')
    canname = 'ConvPMTcanvas'
    ccan = ROOT.TCanvas(canname, canname, 0, 0, 1000, 1000)
    n = len(hPMTs)
    ccan.Divide(n,n)
    cans.append(ccan)
    areas = []
    for i in hPMTs:
        for j in hPMTs:
            if i > j:
                continue
            print('   ...processing {}:{}'.format(i,j))
            conv = hPMTs[i].Convolve(hPMTs[j])
            conv.SetLineStyle(1)
            ccan.cd(i*n + j + 1)
            conv.Draw('hist')
            maxval,area,chi2ndf,fitOK = MakePMTConvText(conv, i, j)
            areas.append([area, i, j, chi2ndf, fitOK, maxval])
            Convs.append(conv)
    for area in areas:
        i = area[1]
        j = area[2]
        #if i == j:
        #    continue
        ai = FindArea(i, areas)
        aj = FindArea(j, areas)
        chi2ndf = area[3]
        fitOK = area[4]
        A = area[0]
        if fitOK and ai > 1000 and aj > 1000 and A > 0.: # and chi2ndf < 1e3:
            asig = A / sqrt(ai*aj)
            ccan.cd(i*n + j + 1)
            txt = ROOT.TLatex(0.43, 0.75, 'S=' + '{:1.2f}'.format(asig) )
            txt.SetNDC()
            txt.SetTextSize(size)
            txt.Draw()
            Txts.append(txt)

    return ccan,areas

##########################################
def MakeCorr2D(h, g, nbins = 50):
    hname = h.GetName() + '_vs_' + g.GetName()
    corr = ROOT.TH2D(hname, hname, nbins, h.GetMinimum(), h.GetMaximum(), nbins, g.GetMinimum(), g.GetMaximum())
    nbh = h.GetXaxis().GetNbins()
    nbg = g.GetXaxis().GetNbins()
    if nbh != nbg:
        print('MakeCorr2D::ERROR: histograms of different binning!')
        return
    for i in range(0, nbh):
        corr.Fill(h.GetBinContent(i+1), g.GetBinContent(i+1))
    corr.SetStats(0)
    return corr
##########################################
def PlotCorrelationsOnSingleCanvas(hPMTs, x = 0.17, size = 0.16):
    canname = 'CorrPMTcanvas'
    ccan = ROOT.TCanvas(canname, canname, 0, 0, 1000, 1000)
    n = len(hPMTs)
    ccan.Divide(n,n)
    cans.append(ccan)
    for i in hPMTs:
        for j in hPMTs:
            if i > j:
                continue
            corr = MakeCorr2D(hPMTs[i],hPMTs[j])
            ccan.cd(i*n + j + 1)
            corr.Draw('colz')
            ROOT.gPad.SetLogz()
            Corrs.append(corr)
            rho = corr.GetCorrelationFactor()
            txt = ROOT.TLatex(0.14, 0.75, '{}:{}'.format(j,i) + ' #rho=' + '{:1.2f}'.format(rho) )
            if rho > 0.10 and i != j:
                txt.SetTextColor(ROOT.kRed)
            txt.SetTextSize(0.03)
            txt.SetNDC()
            txt.SetTextSize(size)
            txt.Draw()
            Txts.append(txt)

    return ccan

##########################################


def MakePMTText(ipmt, h, col = ROOT.kBlack, x = 0.13, y = 0.85):
    txt = ROOT.TLatex(x, y, 'PMT{}'.format(ipmt))
    txt.SetNDC()
    txt.Draw()
    Txts.append(txt)
    txt = ROOT.TLatex(x + 0.55, y, 'I = {:4.0f}'.format(h.Integral()))
    txt.SetNDC()
    txt.Draw()
    Txts.append(txt)

##########################################
# https://root-forum.cern.ch/t/loop-over-all-objects-in-a-root-file/10807
def GetKeyNames(self, dirname = ''):
    self.cd(dirname)
    return [key.GetName() for key in ROOT.gDirectory.GetListOfKeys()]
ROOT.TFile.GetKeyNames = GetKeyNames

##########################################
def getPMTnumber(tag):
    # we deal with names like 'PMT3_evt619_thr'
    stag = tag.split('_')[0]
    return int(stag.replace('PMT',''))
##########################################
def getEvtnumber(tag):
    # we deal with names like 'PMT3_evt619_thr'
    stag = tag.split('_')[1]
    return int(stag.replace('evt',''))

##########################################
def DrawScaledRebinned(leg,hh, rebins, lstyle, lsize, cols, opt = 'hist'):
    rebinned = []
    #print("IN FUNCTION: DrawScaledRebinned")
    leg.AddEntry(hh, 'original', 'L')
    for rebin,lst,lsz,col in zip(rebins,lstyle,lsize,cols):
        #print('   making rebin{} from original histo {} with {} bin.'.format(rebin, hh.GetName(), hh.GetNbinsX()))
        h = hh.Clone(hh.GetName() + 'rebin{:}'.format(rebin))
        #print('        ...made {} histo with {} bins.'.format(h.GetName(), h.GetNbinsX(),))
        h.SetLineColor(col)
        h.SetMarkerColor(col)
        h.Rebin(rebin)
        h.Scale(1./rebin)
        h.SetLineStyle(lst)
        h.SetFillStyle(1111)
        h.SetFillColor(col)
        #h.SetLineWidth(lsz)
        h.Draw('hist same')
        rebinned.append(h)
        leg.AddEntry(h, 'rebinned {}x'.format(rebin), 'L')
    stuff.append(rebinned)
    #print("DONE!")
    return  rebinned


##########################################

def FitHisto(htofit, tag, x1 = 39., x2 = 41.):
        hfit = htofit.DrawCopy()
        hfit.GetXaxis().SetRangeUser(x1, x2)
        hfit.SetMaximum(hfit.GetMaximum()*1.5)
        hfit.SetLineColor(ROOT.kBlack)
        hfit.SetLineWidth(2)
        hfit.SetMarkerColor(ROOT.kBlack)
        hfit.SetMarkerSize(1)
        hfit.SetMarkerStyle(20)
        hfit.SetStats(1)
        fname = 'fitfun' + tag
        fun = ROOT.TF1(fname, 'landau', hfit.GetXaxis().GetXmin(),hfit.GetXaxis().GetXmax() )
        # Const, MPV, sigma
        fun.SetParameters(1000., 39.5, 0.2)
        fun.SetLineWidth(3)
        hfit.Fit(fname,'', '', x1, x2)
        stuff.append([fun,hfit])
        return fun

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
    #ROOT.gStyle.SetPalette(ROOT.kLightTemperature)
    #ROOT.gStyle.SetPalette(ROOT.kSolar)
    #ROOT.gStyle.SetPalette(ROOT.kRainBow)
    ROOT.gStyle.SetPalette(ROOT.kDeepSea)
    #ROOT.gStyle.SetPalette(ROOT.kBlueRedYellow)

    MakeSeparateCanvases = True
    #MakeSeparateCanvases = False
    doDetailedCorrs = False #True
    performFit = False #!!!!

    os.system('mkdir -p {}'.format(pngdir))
    os.system('mkdir -p {}'.format(pdfdir))
    
    filename = sys.argv[1]

    # ZOOM STEERING!
    zoomid = 1 # 0,1,2
    zoom = False
    zoomtag = ''
        
    if zoomid == 1:
        zoom = True
        # do not zoom events from 6h in the morning, which are usually CLF shots;-)
        # zoom = not ('6h' in filename)
        zb1 = 1500
        zb2 = 2500
        zoomtag = '_zoom'
    elif zoomid == 2:
        # ultra zoom:)
        zoom = True
        zb1 = 1950
        zb2 = 2100
        zoomtag = '_ultrazoom'
    
    runapp = 1
    if len(sys.argv) > 2:
        runapp = int(sys.argv[2])
    if not runapp:
        ROOT.gROOT.SetBatch(1)
        
    rfile = ROOT.TFile(filename, 'read')
    keys = rfile.GetKeyNames()

    outfilename = filename
    outfilename = outfilename.replace('histos_', 'analyzed_')
    outfile = ROOT.TFile(outfilename, 'recreate')
    
    h_Chi2 = ROOT.TH1D("h_Chi2OverNdf", "h_Chi2OverNdf;#chi2/ndf", 100, 0, 10)
    h_Chi2OverNdfVsConstant = ROOT.TH2D("h_Chi2OverNdfVsConstant", "h_Chi2OverNdfVsConstant;Landau Const.;#chi2/ndf", 200, 0, 2000, 100, 0, 10)
    h_Chi2OverNdfVsMPV = ROOT.TH2D("h_Chi2OverNdfVsMPV", "h_Chi2OverNdfVsMPV;Landau MPV;#chi2/ndf", 200, 0, 2000, 100, 0, 10)
    h_Chi2OverNdfVsSigma = ROOT.TH2D("h_Chi2OverNdfVsSigma", "h_Chi2OverNdfVsSigma;Landau #sigma;#chi2/ndf", 100, 0, 300, 100, 0, 10)

    rfile.cd()
    
    #print('Reading histos...')
    for key in keys:
        #if 'plane' in key or
        if 'all' in key or 'integral' in key or 'signal' in key or 'spike' in key or 'Yproj' in key: ### HACK!!! or not ('_thr' in key or '_clf' in key):
            continue
        ch1 = rfile.Get(key)
        h1 = TranslateToMicroseconds(ch1)
        if h1.InheritsFrom("TH1") and 'FFT' not in key:
            #print(h1.GetName())
            name = key
            name = name.replace('hist_', 'FFT_mag_hist_')
            fftmag = rfile.Get(name)
            name = key
            name = name.replace('hist_', 'FFT_ph_hist_')
            fftph = rfile.Get(name)
            #print('Appending histos triade binned {} {} {}'.format(h1.GetNbinsX(), fftmag.GetNbinsX(), fftph.GetNbinsX()))
            Hists.append([h1, fftmag, fftph])
            
    #print('Processing {} histos...'.format(len(Hists)))

    can = ROOT.TObject()
    if not MakeSeparateCanvases:
        canname = 'PMTcanvas'
        can = ROOT.TCanvas(canname, canname,1900,1000)
        can.Divide(2,2)
        cans.append(can)
        can = cans[-1]
    
    canname = 'Evtcanvas'
    nFasts = 3
    PMTPosition = PMTPositionSkyViewTA
    if 'Auger' in filename:
        nFasts = 1
        PMTPosition = PMTPositionSkyViewAuger
    square = 325
    canw = 2*nFasts*square
    ecan = ROOT.TCanvas(canname, canname, 0, 0, canw, 2*square)
    ecan.Divide(2*nFasts,2)

    cans.append(ecan)

    
    """
    gmax = -1
    gmin = 999e99
    for hists in Hists:
        val = hists[0].GetMaximum() 
        if val > gmax:
            gmax = val*1.5
        if val < gmin:
            gmin = val*1.5
    """

    rebins = [1, 2, 4, 10]#,25]
    # which version of the rebinned histos to take for PMTs-in-one-plot
    evtRebinIndex = 0 # 1

    lstyles = [1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1]
    lsizes = [2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2]
    cols = [ROOT.kBlue, ROOT.kGreen+2, ROOT.kRed, ROOT.kYellow, ROOT.kCyan, ROOT.kMagenta, ROOT.kPink]

    #print('*** OK, loop over triades! ***')
    
    for hists in Hists:

        for h in hists:
            h.SetStats(0)
        hh,fftmag,fftph =hists[0],hists[1],hists[2]
        #print('Got histos triade named {} {} {}'.format(hh.GetName(), fftmag.GetName(), fftph.GetName()))
        #print('Got histos triade binned {} {} {}'.format(hh.GetNbinsX(), fftmag.GetNbinsX(), fftph.GetNbinsX()))
        hh.SetLineColor(ROOT.kBlack)
        tag = hh.GetName()
        tag = tag.replace('hist_', '')
        print(tag)
        iPMTtrig = getPMTnumber(tag)
        ievt = getEvtnumber(tag)
        print('--- processing evt {} iPMTtrig {} ---'.format(ievt, iPMTtrig))
        
        if  MakeSeparateCanvases:
            canname = 'PMTcanvas_{}_{}'.format(ievt,iPMTtrig)
            can = ROOT.TCanvas(canname, canname,1400,1000)
            can.Divide(2,2)
            cans.append(can)
            can = cans[-1]
        
        can.cd(1)
        leg = ROOT.TLegend(0.65, 0.60, 0.85, 0.88)
        leg.SetBorderSize(0)
        Legs.append(leg)
        hh.SetMaximum(hh.GetMaximum()*1.5)
        #hh.SetMaximum(gmax)
        #hh.SetMinimum(gmin)

        # THIS
        hcop = hh.DrawCopy('hist')
        # OR THIS?
        #hh.Draw('hist')
        #hcop = hh
        
        RebinnedHistos = DrawScaledRebinned(leg, hcop, rebins, lstyles, lsizes, cols)
        Ihisto = hh.Integral()
        txt = ROOT.TLatex(0.14, 0.84, 'I = {:.0f} nbins={}'.format(Ihisto,hh.GetNbinsX()))
        txt.SetNDC()
        txt.Draw()
        leg.Draw()
        stuff.append(txt)
        
        can.cd(2)
        fftmag.Draw('hist')

        can.cd(4)
        fftph.Draw('hist')

        can.cd(3)
        fittag = 0
        if performFit:
            #print('FITTING!')
            fitfun = FitHisto(RebinnedHistos[-2], tag) # -1
            # Get chi2/ndf and flag if better than 2!
            # make histogramme of the chi2/ndf!
            # remove histos/events with too few bins?
            # to check: iPMT over 11?!

            Ifit = fitfun.Integral(hh.GetXaxis().GetXmin(),hh.GetXaxis().GetXmax())
            txt2 = ROOT.TLatex(0.14, 0.84, 'I={:.0f}'.format(Ifit) + ';   I_{fit} / I_{histo}' + '={:1.2f}'.format(Ifit/Ihisto))
            txt2.SetTextColor(ROOT.kRed)
            txt2.SetNDC()
            txt2.Draw()
            stuff.append(txt2)

            konst = fitfun.GetParameter(0)
            mpv = fitfun.GetParameter(1)
            sigma = fitfun.GetParameter(2)
            chi2ndf = -1;
            ndf = fitfun.GetNDF()
            chi2 = fitfun.GetChisquare()
            if ndf > 0:
                chi2ndf = chi2 / (1.*ndf)
            if chi2ndf > 0:
                h_Chi2.Fill(chi2ndf)
                h_Chi2OverNdfVsConstant.Fill(konst, chi2ndf)
                h_Chi2OverNdfVsMPV.Fill(mpv, chi2ndf)
                h_Chi2OverNdfVsSigma.Fill(sigma, chi2ndf)

            fittagged = chi2ndf < 3. and chi2 > 0. and konst > 250. and sigma > 40 and mpv > 200. # sigma < 200  and konst < 800.
            fittag = 'fittag0'
            if fittagged:
                fittag = 'fittag1'



        can.cd(2)
        padname = 'zoompad_' + tag
        zoompad = ROOT.TPad(padname, padname, 0.4, 0.4, 0.88, 0.90)
        stuff.append(zoompad)
        zoompad.Draw()
        zoompad.cd()
        fftmagcop = fftmag.Clone(fftmag.GetName()+'_clone')
        fftmagcop.GetXaxis().SetRangeUser(0, 20)
        fftmagcop.SetLineWidth(3)
        fftmagcop.Draw()
        stuff.append(fftmagcop)

        runtag = filename.split('/')[-1]
        runtag = runtag.replace('histos_','histo_')
        runtag = runtag.replace('.root','')
        
        can.Print(pngdir + runtag + '_' + tag + '_{}'.format(fittag) + zoomtag + '.png')
        can.Print(pdfdir + runtag + '_' + tag + '_{}'.format(fittag) + zoomtag + '.pdf')

        try:
            if EventsAnalyzed[ievt]:
                continue
        except:
            EventsAnalyzed[ievt] = True
            
        # now draw all the PMTs in the event!
        # prepare a dictionary o iPMT : histogramme
        hPMTs = {}
        hYprojs = {} # projections onto y axis to compute variances from
        nSig = 0

        hToClone = RebinnedHistos[evtRebinIndex]
        triggerPMT = hToClone.Clone(hToClone.GetName() + '_trig')
        hPMTs[iPMTtrig] = triggerPMT
        hYprojs[iPMTtrig] = MakeYProjection(hPMTs[iPMTtrig])
        col = PMTcol[iPMTtrig]
        #col = ROOT.kBlue
        #if '_yap' in tag:
        #    col = ROOT.kRed
        triggerPMT.SetLineColor(col)
        triggerPMT.SetFillColor(col)
        triggerPMT.SetLineWidth(1) ### HM, not sure why needed here, but otherwise the lw is 2...
        triggerPMT.SetMarkerColor(col)
        ymax = triggerPMT.GetMaximum()*1.5
        ymin = triggerPMT.GetMinimum()
        triggerPMT.SetMaximum(ymax)
        if zoom:
            triggerPMT.GetXaxis().SetRange(zb1, zb2)

        #print('Drawing histo titled {}'.format(triggerPMT.GetTitle()))
        ecan.cd(PMTPosition[iPMTtrig])
        ROOT.gPad.SetGridy(1)
        ROOT.gPad.SetGridx(1)
        triggerPMTcp = triggerPMT.DrawCopy('hist');
        cpHists.append(triggerPMTcp)
        print('Making trigPMT text for {} which should result in {}'.format(iPMTtrig, PMTPosition[iPMTtrig]))
        MakePMTText(iPMTtrig, triggerPMTcp, col)
            
        nSig = nSig+1
        print('OK, will loop over the other PMTs;-)')
        # Now find and plot all the other PMT histos for the same event!
        for key2 in keys:
            if not 'hist_' in key2:
                #print('...skipping1 {}'.format(key2))
                continue
            if 'FFT' in key2 or 'Yproj' in key2:
                #print('...skipping2 {}'.format(key2))
                continue
            tag2 = key2
            tag2 = tag2.replace('hist_', '')
            #print('tag2={}'.format(tag2))
            jevt = getEvtnumber(tag2)
            if ievt != jevt:
                continue
            iPMT  = getPMTnumber(tag2)
            if iPMT > nMaxPMTs-1:
                print('ERROR: GOT PMT NUMBER {}, THIS SHOULD ***NEVER*** HAPPEN!'.format(iPMT))
            if iPMT == iPMTtrig:
                #print('...skipping {} iPMT: {} as trigPMT: {}'.format(key2, iPMT, iPMTtrig))
                continue
            hname = key2
            print('...working on {}'.format(key2))
            chPMTorig = rfile.Get(hname )
            # translate to mus:
            hPMTorig = TranslateToMicroseconds(chPMTorig)
            hPMT = hPMTorig.Clone(hname + '_clone')
            hPMT.Scale(1.)
            col = PMTcol[iPMT]
            #col = ROOT.kBlack
            if '_thr' in key2:
            #    col = ROOT.kBlue
                nSig = nSig+1
            #if '_yap' in key2:
            #    col = ROOT.kRed
            #if '_clf' in key2:
            #    col = ROOT.kGreen+2
            hPMT.SetLineColor(col)
            hPMT.SetMarkerColor(col)

            print('...rebinning iPMT {} binned as {} by factor {}'.format(hPMT.GetName(), hPMT.GetNbinsX(), rebins[evtRebinIndex]))
            hPMT.Rebin(rebins[evtRebinIndex])
            hPMT.SetStats(0)
            ecan.cd(PMTPosition[iPMT])
            ROOT.gPad.SetGridy(1)
            ROOT.gPad.SetGridx(1)

            #hPMT.SetMaximum(ymax)
            #hPMT.SetMinimum(ymin)
            print('...drawing histo titled {}'.format(hPMT.GetTitle()))
            if zoom:
                hPMT.GetXaxis().SetRange(zb1, zb2)
            hPMTcp = hPMT.DrawCopy('hist');
            hPMTcp.SetFillStyle(1111)
            hPMTcp.SetFillColor(hPMTcp.GetLineColor())
            print('Making PMT text for {} which should result in {}'.format(iPMT, PMTPosition[iPMT]))
            MakePMTText(iPMT, hPMTcp, col)
            try:
                print(hPMTs[iPMT])
                print('Wheeeea, we already got this one and should not!')
            except:
                hPMTs[iPMT] = hPMT

            hYprojs[iPMT] = MakeYProjection(hPMTs[iPMT])
            stuff.append(hPMT)
            cpHists.append(hPMTcp)

            # TODO! analyze the projections and compute variance, and cut on it!
            # fill and save global hitos of variance of each PMT;-)

        FindAndSetGlobalMax(cpHists)
        ecan.Update()
        ecan.Print('{}EventDispl_{}_evt{}_Thr{}{}.png'.format(pngdir,runtag,ievt,nSig,zoomtag))
        ecan.Print('{}EventDispl_{}_evt{}_Thr{}{}.pdf'.format(pdfdir,runtag,ievt,nSig,zoomtag))

        gcan = PlotPMTsOnSingleCanvas(hPMTs)
        gcan.Update()
        gcan.Print('{}PMTTimes_{}_evt{}_Thr{}{}.png'.format(pngdir,runtag,ievt,nSig,zoomtag))
        gcan.Print('{}PMTTimes_{}_evt{}_Thr{}{}.pdf'.format(pdfdir,runtag,ievt,nSig,zoomtag))

        if doDetailedCorrs:
            corrcan = PlotCorrelationsOnSingleCanvas(hPMTs)
            corrcan.Update()
            corrcan.Print(pngdir + 'CorrPMT_{}_evt{}_Thr{}.png'.format(runtag,ievt,nSig))
            corrcan.Print(pdfdir + 'CorrPMT_{}_evt{}_Thr{}.pdf'.format(runtag,ievt,nSig))
            #ccan,areas = PlotConvolutionsOnSingleCanvas(hPMTs)
            #ccan.Update()
            #ccan.Print(pngdir + 'ConvPMT_{}_evt{}_Thr{}.png'.format(runtag,ievt,nSig))
            #ccan.Print(pdfdir + 'ConvPMT_{}_evt{}_Thr{}.pdf'.format(runtag,ievt,nSig))
        
        print('END of loop block!')
        
    print('END of loop!')
    outfile.Write()
    outfile.Close()
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

