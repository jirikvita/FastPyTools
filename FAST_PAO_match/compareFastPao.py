#!/snap/bin/pyroot
# was: #!/usr/bin/python3
# Út 7. března 2023, 19:58:08 CET

#from __future__ import print_function

import ROOT
from math import sqrt, pow, log, exp
import os, sys, getopt
from collections import OrderedDict

######################################################################
# globals
cans = []
stuff = []

######################################################################
class myVar:
    def __init__(self, name, val, stat, syst):
        self.name = name
        self.val = val
        self.stat = stat
        self.syst = syst
        self.relStat = stat / val
        self.relSyst = syst / val
        self.relTotUnc = sqrt (pow(syst,2) + pow(stat,2)) / val
        
######################################################################
class fitRes:
    def __init__(self, e, eStat, eSyst, xmax, xmaxStat, xmaxSyst):
        self.var = OrderedDict()
        self.var['E'] = myVar('E', e, eStat, eSyst)
        self.var['Xmax'] = myVar('Xmax', xmax, xmaxStat, xmaxSyst)
        

######################################################################
# event-by-event comparison between observatories
# and also individual observatories specrtra and resolution

class resCmp():
    def __init__(self, eventID, obsName1, fitRes1, obsName2, fitRes2):
        self.eventID = eventID
        self.fitRes = OrderedDict()
        self.fitRes[obsName1] = fitRes1
        self.fitRes[obsName2] = fitRes2

        comparison =  OrderedDict()
        individual =  OrderedDict()
        grPoints =  OrderedDict()

        individual[obsName1] = OrderedDict()
        individual[obsName2] = OrderedDict()
        grPoints[obsName1] = OrderedDict()
        grPoints[obsName2] = OrderedDict()
        
        # add error propagation?;)
        # use myVar class also for individual and comparison?;)
        
        for varname in fitRes1.var:

            # individual and individual graph points like sigma_E / E
            for obsName,fitRes in self.fitRes.items():
                individual[obsName][varname + '_indiv_' + obsName] = fitRes.var[varname].val
                grPoints[obsName]['relTotUnc' + varname + '_indiv_' + obsName] = [fitRes.var[varname].relTotUnc, fitRes.var[varname].val]

            # ratios and diffs
            comparison[varname + 'ratio'] = fitRes2.var[varname].val / fitRes1.var[varname].val
            comparison[varname + 'diff'] = fitRes2.var[varname].val - fitRes1.var[varname].val
            comparison[varname + 'reldiff'] = (fitRes2.var[varname].val - fitRes1.var[varname].val) / fitRes1.var[varname].val
            comparison[varname + 'diffSignifStat'] = (fitRes2.var[varname].val - fitRes1.var[varname].val) /  sqrt( pow(fitRes2.var[varname].stat, 2) + pow(fitRes1.var[varname].stat, 2) )

        self.comparisons = comparison
        self.individuals = individual
        self.grPoints = grPoints

######################################################################
# parse ascii data from observatories on one ascii line
def parseEventLine(results, tokens):

    PaoIndex = tokens.index('PAO:')
    e = float(tokens[PaoIndex + 9])
    estat = float(tokens[PaoIndex + 11])
    xmax = float(tokens[PaoIndex + 3])
    xmaxstat = float(tokens[PaoIndex + 5])
    resPAO = fitRes(e, estat, 0., xmax, xmaxstat, 0.)

    FastIndex = tokens.index('FAST:')
    e = float(tokens[FastIndex + 9])
    estat = float(tokens[FastIndex + 11])
    xmax = float(tokens[FastIndex + 3])
    xmaxstat = float(tokens[FastIndex + 5])
    resFAST = fitRes(e, estat, 0., xmax, xmaxstat, 0.)

    eventID = tokens[0]
    result = resCmp(eventID, 'PAO', resPAO, 'FAST', resFAST)
    
    results.append(result)

    return results

######################################################################
def makeComparisonHistos(results):

    cmpVars = []
    # just get the names of the comparison variables between observables
    for result in results:
        comparisons = result.comparisons
        for cmpVar in comparisons:
            cmpVars.append(cmpVar)
        break
    print('OK, will plot the following comparisons:')
    print(cmpVars)

    histos = OrderedDict()
    nb = len(results)
    
    for cmpVar in cmpVars:
        hname = 'h_{}'.format(cmpVar)
        htitle = hname.replace('h_','') + ';;' + cmpVar
        histos[cmpVar] = ROOT.TH1D(hname, htitle, nb, 0, nb)

    ires = 0
    for result in results:
        comparisons = result.comparisons
        for cmpVar in comparisons:
            h = histos[cmpVar]
            h.SetBinContent(ires + 1, comparisons[cmpVar])
            h.GetXaxis().SetBinLabel(ires + 1, result.eventID.replace('FAST_','').replace('_','/'))
        for cmpVar in comparisons:
            h = histos[cmpVar]
            histos[cmpVar].Scale(1.)

        ires = ires + 1

    return histos


######################################################################
def makeIndividualHistos(results, debug = 1):
    # histos from individual observatories,
    # but also scatter plots of var_obs2 vs var_obs1 ;-)
    
    obshistos = OrderedDict()
    obsNames = []
    for result in results:
        for obsName in result.fitRes:
            obsNames.append(obsName)
        break
    if debug: print('OK, go through the individual observatories: ')
    if debug: print(obsNames)

    iobs = -1
    for obsName in obsNames:
        iobs = iobs + 1
        if debug: print('processing observatory ', obsName)
        obshistos[obsName] = OrderedDict()

        cmpVars = []
        # just get the names of the comparison variables between observables
        for result in results:
            individuals = result.individuals
            for cmpVar in individuals[obsName]:
                cmpVars.append(cmpVar)
            break
        if debug: print('OK, will plot the following individual spectra:')
        if debug: print(cmpVars)

        nb = 10
        cmpVarBase = ''
        for cmpVar in cmpVars:

            hname = 'h_{}_{}'.format(obsName, cmpVar)
            varMin = 1.
            varMax = -1.
            if 'Xmax' in cmpVar:
                varMin = 100.
                varMax = 1700.
                nb = 12
            elif 'E' in cmpVar:
                varMin = 0.
                varMax = 3.
            htitle = hname.replace('h_','') + ';' + cmpVar
            obshistos[obsName][cmpVar] = ROOT.TH1D(hname, htitle, nb, varMin, varMax)

            if iobs == 0:
                cmpVarBase = cmpVar.replace(obsNames[0], '').replace('_indiv_','')
                hname = 'h2_{}_{}_vs_{}'.format(cmpVarBase, obsNames[1], obsNames[0])
                htitle = hname.replace('h2_','') + ';' + cmpVarBase + ' ' + obsNames[0] + ';' + cmpVarBase + ' ' + obsNames[1] 
                obshistos[obsName][cmpVarBase] = ROOT.TH2D(hname, htitle, nb, varMin, varMax, nb, varMin, varMax)

        ires = 0
        for result in results:
            individuals = result.individuals
            #cmpVarsExt  = cmpVars + [cmpVarBase]
            for cmpVar in cmpVars:
                if debug: print('  processing variable ', cmpVar)
                h = obshistos[obsName][cmpVar]
                hname = h.GetName()
                if debug: print('   hname:', hname)
                if debug: print('   ', obsName, cmpVar)
                if debug: print('   ', individuals[obsName][cmpVar])

                #if not 'h2' in hname:
                if debug: print('    filling 1d histo ', h.GetName())
                h.Fill(individuals[obsName][cmpVar])
                #else:
                cmpVarBase = cmpVar.replace(obsNames[0], '').replace('_indiv_','')
                hname = 'h2_{}_{}_vs_{}'.format(cmpVarBase, obsNames[1], obsNames[0])
                if debug: print('    filling 2d histo ', h.GetName())
                obsAlt = -1
                for obsAltName in obsNames:
                    obsAlt = obsAlt + 1
                    if iobs < obsAlt:
                        cmpVarAlt = cmpVar.replace(obsNames[0], obsNames[1])
                        obshistos[obsName][cmpVarBase].Fill(individuals[obsName][cmpVar], individuals[obsAltName][cmpVarAlt])
                
            for cmpVar in cmpVars:
                h = obshistos[obsName][cmpVar]
                h.Scale(1.)
            ires = ires + 1
                
        # todo: parse also to make TGraphs of sigma_E/E and same for Xmax

    # todo: return also graphs
    return obshistos
        
######################################################################

def makeYprojections(histos):
    projs = OrderedDict()
    for cmpVar in histos:
        histo = histos[cmpVar]
        pname = histo.GetName().replace('h_','proj_')
        ptitle = 'proj. ' + pname.replace('proj_','') + ';' + histo.GetYaxis().GetTitle() + ';events;'
        nb = 11
        #ymin = histo.GetYaxis().GetXmin()
        #ymax = histo.GetYaxis().GetXmax()
        ymin = histo.GetMinimum()
        ymax = histo.GetMaximum()
        print("*** ", cmpVar, histo.GetName(), ymin, ymax)
        proj = ROOT.TH1D(pname, ptitle, nb, ymin, ymax)
        projs[cmpVar] = proj
        for ibin in range(1,histo.GetXaxis().GetNbins()+1):
            proj.Fill(histo.GetBinContent(ibin))
    return projs
    
######################################################################
def adjustAxes(h):
    hname = h.GetName()
    if 'ratio' in hname:
        h.GetYaxis().SetRangeUser(0., 4.)
    if 'reldiff' in hname:
        h.GetYaxis().SetRangeUser(-1.5, 1.5)
    if 'Signif' in hname:
        h.GetYaxis().SetRangeUser(-15, 15.)


    if 'Eratio' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Eratio', 'E ratio'))
    if 'Xmaxratio' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Xmaxratio', 'X_{max} ratio'))

        
    if 'Ediff' in hname and not 'Signif' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Ediff','#DeltaE') + ' [EeV] ')
    if 'Xmaxdiff' in hname and not 'Signif' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Xmaxdiff','#DeltaX_{max}') + ' [g/cm^{2}] ')

    if 'EdiffSignif' in hname and 'Signif' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('EdiffSignif','#DeltaE / #sigma_{E}^{') + '}')
    if 'XmaxdiffSignif' in hname and 'Signif' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('XmaxdiffSignif','#DeltaX_{max} / #sigma_{X_{max}}^{') + '}')


    if 'Ereldiff' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Ereldiff','#DeltaE / E'))
    if 'Xmaxreldiff' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Xmaxreldiff','#DeltaX_{max} / X_{max}'))

    return
######################################################################
def adjustIndivAxes(h):
    hname = h.GetName()
    if 'E' in hname:
        h.GetXaxis().SetTitle(h.GetXaxis().GetTitle().replace('_indiv_','^{') + '} [EeV] ')
    if 'Xmax' in hname:
        h.GetXaxis().SetTitle(h.GetXaxis().GetTitle().replace('Xmax','X_{max}').replace('_indiv_','^{') + '} [g/cm^{2}] ')

    return
######################################################################
def adjustIndivAxes2D(h):
    hname = h.GetName()
    if 'E' in hname:
        #print(h.GetXaxis().GetTitle())
        h.GetXaxis().SetTitle(h.GetXaxis().GetTitle().replace('E ','E^{') + '} [EeV] ')
    if 'Xmax' in hname:
        h.GetXaxis().SetTitle(h.GetXaxis().GetTitle().replace('Xmax','X_{max}^{') + '} [g/cm^{2}] ')
    if 'E' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('E ','E^{') + '} [EeV] ')
    if 'Xmax' in hname:
        h.GetYaxis().SetTitle(h.GetYaxis().GetTitle().replace('Xmax','X_{max}^{') + '} [g/cm^{2}] ')

    return

######################################################################
def printAllCanvases(cans):
    for c in cans:
        c.Update()
        c.Print(c.GetName() + '.png')
        c.Print(c.GetName() + '.pdf')
    return
######################################################################

def plotProjections(histos, projections, cols, cw, ch, nx, ny):
    canname = 'PAO_FAST_CmpProj'
    can = ROOT.TCanvas(canname, canname, 0, 400, cw, ch)
    can.Divide(nx,ny)
    cans.append(can)
    ican = 0
    for cmpVar in histos:
        ican = ican + 1
        can.cd(ican)
        ROOT.gPad.SetLeftMargin(0.15)
        proj = projections[cmpVar]
        #proj.SetStats(0)
        icol = (ican-1) % (nx)
        proj.SetFillColor(cols[icol])
        proj.SetStats(0)
        proj.Scale(1)
        proj.Draw('hist')
        
        pname = proj.GetName()
        yline = 0.
        if 'ratio' in pname:
            yline = 1.
        line = ROOT.TLine(yline, proj.GetYaxis().GetXmin(), yline, 1.1*proj.GetMaximum())
        line.SetLineColor(ROOT.kRed)
        line.Draw()
        stuff.append(line)

######################################################################

def plotComparisonHistos(histos, results, cols, cw, ch, nx, ny):
    canname = 'PAO_FAST_Cmp'
    can = ROOT.TCanvas(canname, canname, 0, 0, cw, ch)
    can.Divide(nx,ny)
    cans.append(can)

    ican = 0
    for cmpVar in histos:
        ican = ican + 1
        can.cd(ican)
        ROOT.gPad.SetLeftMargin(0.15)
        h = histos[cmpVar]
        h.SetStats(0)
        hname = h.GetName()

        adjustAxes(h)
        
        icol = (ican-1) % (nx)
        h.SetFillColor(cols[icol])
        opt = 'hist'
        if 'ratio' in hname:
            h.SetMarkerStyle(20)
            h.SetMarkerSize(1.2)
            h.SetMarkerColor(ROOT.kBlack)
            opt = 'P'
        h.Draw(opt) # pfc

        yline = 0.
        if 'ratio' in hname:
            yline = 1.
            
        line = ROOT.TLine(h.GetXaxis().GetXmin(), yline, h.GetXaxis().GetXmax(), yline)
        line.SetLineColor(ROOT.kRed)
        line.Draw()
        stuff.append(line)

    can.Update()

######################################################################
def plotIndividualHistos(obsIndividualHistos, debug = 1):
    if debug: print('* In plotIndividualHistos:')
    # observatory name vs. histos dict.
    iobs = 0
    cans2d = []
    
    for obsName, indHistos in obsIndividualHistos.items():
        iobs = iobs + 1
        cname = 'can_{}_individual'.format(obsName)
        can = ROOT.TCanvas(cname, cname, iobs*200, iobs*200, 800, 400)
        can.Divide(2,1)
        cans.append(can)
        
        
        ih = 0
        ih2 = 0
        print(indHistos)
        for cmpVar, histo in indHistos.items():
            hname = histo.GetName()
            if not 'h2' in hname:
                if debug: print('  drawing individual', hname)
                ih = ih + 1
                can.cd(ih)
                histo.SetFillColor(ROOT.kPink)
                adjustIndivAxes(histo)
                histo.Draw('hist')
            else:
                ih2 = ih2 + 1
                if len(cans2d) < 1:
                    cmpVarBase = cmpVar.replace(obsName, '').replace('_indiv_','')
                    cname = 'can2d_observatoriesScatter'
                    can2d = ROOT.TCanvas(cname, cname, (iobs+3*ih2)*200, (iobs+3*ih2)*200, 470, 800)
                    can2d.Divide(1,2)
                    cans.append(can2d)
                    cans2d.append(can2d)
                cans2d[-1].cd(ih2)
                ROOT.gPad.SetLeftMargin(0.15)
                adjustIndivAxes2D(histo)
                histo.SetStats(0)
                x1, y1 = histo.GetXaxis().GetXmin(),  histo.GetYaxis().GetXmin()
                x2, y2 = histo.GetXaxis().GetXmax(),  histo.GetYaxis().GetXmax()
                line = ROOT.TLine(x1, y1, x2, y2)
                stuff.append(line)
                histo.Draw('colz')
                line.Draw()
                    
    return

######################################################################


######################################################################
######################################################################
######################################################################

def main(argv):

    debug = 1    
    print('*** Settings:')

    # to store events reconstructed by both observatories
    results = []

    ROOT.gStyle.SetPalette(ROOT.kSolar)
    
    infilename = 'data.txt'
    infile = open(infilename)
    for xline in infile.readlines():
        line = xline[:-1]
        print(line)
        tokens = line.split(' ')
        if debug: print(tokens)
        print(tokens[1])
        parseEventLine(results, tokens)

    cw, ch = 1800, 800
    nx, ny = 4, 2
    cols = [ ROOT.kYellow, ROOT.kGreen, ROOT.kCyan, ROOT.kMagenta, ROOT.kBlue-2, ROOT.kPink, ROOT.kGreen, ROOT.kPink, ROOT.kTeal ]
        
    histos = makeComparisonHistos(results)
    plotComparisonHistos(histos, results, cols, cw, ch, nx, ny)
    
    projections = makeYprojections(histos)
    plotProjections(histos, projections, cols, cw, ch, nx, ny)

    obsIndividualHistos = makeIndividualHistos(results)
    plotIndividualHistos(obsIndividualHistos)
    
    printAllCanvases(cans)
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

