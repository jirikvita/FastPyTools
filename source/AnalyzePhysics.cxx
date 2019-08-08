/* 

   jk created :        25.3.2019
   main modifications: 16.5.2019

*/

#include "AirFlyFile.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TString.h"
#include "TFile.h"

// FFT based on https://root.cern.ch/root/html/tutorials/fft/FFT.C.html
#include "TVirtualFFT.h"

#include <iostream>
#include <vector>

using std::cout;
using std::cerr;
using std::endl;
using std::vector;

// TODO:
// eventually a TTree with signal info?
//
// todo: variance of pedestals?
// variance of signal passing thr cut w.r.t. fit?
// pedestal and its vatiation after the signal pulse (in tail)
// time correlations between PMTs for signal over events: w/ and w/o thr. cuts
// time shift between pulses (edge? Landau peak?) of different PMTs in single events
// histogramme of signals ( = integrals)
// "evt display": show integrals in PMTs!:)
// change order of loops, same all PMTs in event if any PMT has signal!


// __________________________________________________________________________________________
TH1D *MakeYproj(TH1D *hh, int n = 250, double ymin = 300.)
{
  TString hname = "Yproj_" + TString(hh->GetName());
  TString htitle = "Yproj_" + TString(hh->GetTitle());
  TH1D *proj = new TH1D(hname, htitle, n, -ymin, ymin);
  for (int i = 1; i <= n; ++i) {
    double val = hh -> GetBinContent(i);
    proj -> Fill(val);
  }
  return proj;
    
}

// __________________________________________________________________________________________
// look if there is no other bin with more than 0.5 content w.r.t. maximum
// do it only for peaks above thr??
bool IsSingleSpike(TH1D *hh, double thr = 300, int nMin = 4, double frac = 0.5)
{
  int n = hh -> GetNbinsX();
  double gmax = -9e9;
  int imax = -1;
  for (int i = 1; i <= n; ++i) {
    double val = hh -> GetBinContent(i);
    if (val > thr && val > gmax) {
      gmax = val;
      imax = i;
    }
  }
  if (imax < 0)
    return true; // is just some noise

  int nNontriv = 0;
  for (int i = 1; i <= n; ++i) {
    if (i == imax) continue;
    double val = hh -> GetBinContent(i);
    if (val > thr && val > gmax*frac) { // too complicated/unnecessary to check also thr here??
      // ok, nontrivial content
      nNontriv++;
    } // 
  } // bins

  return (nNontriv < nMin);
}

// __________________________________________________________________________________________
// UNFINISHED, 21.5.2019
bool IsSingleSpikeBad(TH1D *hh, double thr = 400., double signif = 5.)
{
  int n = hh -> GetNbinsX();
  int nspikes = 0;
  for (int i = 2; i <= n-1; ++i) {
    double val = hh -> GetBinContent(i);
    double sigma = sqrt(fabs(val));
    if (val > thr) {
      double sigleft  = fabs(hh -> GetBinContent(i-1) - val) / sigma;
      double sigright = fabs(hh -> GetBinContent(i+1) - val) / sigma;
      //double sigleft  = fabs(hh -> GetBinContent(i-1)) / sigma;
      //double sigright = fabs(hh -> GetBinContent(i+1)) / sigma;
      if (sigleft > signif && sigright > signif) {
	// spike! found only small values around the value!
	nspikes++;
      }
    } // thr
  } // bins
  return (nspikes == 1);
}

// __________________________________________________________________________________________
// based on https://root-forum.cern.ch/t/split-tstring-by-delimeter-in-root-c/18228
void SplitString(TString x, TString delim, vector<TString> &v){
  v.clear();
  int stringLength = x.Length();
  int delimLength = delim.Length();
  int stop = 1;
  TString temp = "---";
  while(stop != -1){
    stop = x.First(delim);
    if(stop != -1){
      temp = x(0, stop);
      TSubString newString = x( stop + delimLength, stringLength );
      x = newString;
      stringLength = x.Length();
    }
    else{
      stringLength = x.Length();
      temp = x(0, stringLength);
    }
    v.push_back(temp);
  }
}

// __________________________________________________________________________________________
// __________________________________________________________________________________________
// __________________________________________________________________________________________


int main(int argc, char **argv) {


  if (argc < 3) {
    cout << "Usage: " << argv[0] << " path/filename.data siteTag" << endl;
    cout << "    example: " << argv[0] << " /data/FAST/run190114/FAST_2019_01_14_10h15m39s_trig0.data TA" << endl;
    cout << "    example: " << argv[0] << " ../data/FAST_2017_09_25_10h56m27s_trig0.data TA" << endl;
    cout << "    example: " << argv[0] << " /data/FAST/Auger/run190507/FAST_2019_05_07_23h43m33s.data Auger" << endl;
    return 1;
  }
  
  //std::string infilename = "../data/FAST_2018_05_15_09h25m50s_trig0.data";
  //  std::string infilename = "../data/FAST_2017_09_25_10h56m27s_trig0.data";
  //std::string infilename = "../data/FAST_2014_04_25_08h30m50s.data";
  // BAD?  std::string infilename = "../data/FAST_2018_01_18_05h52m55s_trig0_283.data";

  // IGA:
  //  std::string infilename = "/data/FAST/run190114/FAST_2019_01_14_10h15m39s_trig0.data";

  std::string infilename = argv[1];
  TString InFileName = infilename.c_str();

  TString siteTag = TString("_") + TString(argv[2]);
  
  AirFlyFile *mf = new AirFlyFile(infilename);
  cout << infilename.c_str() << endl;

  int nPMTs = mf -> GetNChannel();
  std::vector< std::vector<TH1D*> > Hists;
  std::vector< std::vector<TH1D*> > Projs;
  
  int n = mf -> GetN();
  cout << "Channels : " << nPMTs << endl;
  cout << "Events   : " << n << endl;
  double ch1 = 0;
  double ch2 = 5000;
  int nbins = int(ch2-ch1);
  int nh = -1;

  //!!!DEFAULT
  //double thr = 25.e3; // counts
  // HACK FOR AUG 219 VERTICAL SEARCH!!!
  double thr = 1.e2; // counts
  int irand = nh / 4;

  vector<TH1D*> h_signal;
  vector<TH1D*> h_integral;
  vector<TH2D*> h_integral_vs_signal;
  vector<TH1D*> h_spikeSize;
  vector<TH1D*> h_spikeIntegral;
  
  for (int iPMT = -1; iPMT < nPMTs; iPMT++) {
    TString tag = "all";
    if (iPMT >= 0)
      tag = Form("%i", iPMT);
    
    h_spikeIntegral.push_back( new TH1D("h_spikeIntegral_PMT" + tag, "h_spikeIntegral_PMT" + tag + ";spike integral", 800, 2000, 10000) );
    h_spikeSize.push_back( new TH1D("h_spikeSize_PMT" + tag, "h_spikeSize_PMT" + tag + ";spike size", 20, 0, 20) );
    
    h_signal.push_back( new TH1D("h_signal_PMT" + tag, "h_signal_PMT" + tag + ";signal", 1000, -1e5, 1e5) );
    h_integral.push_back( new TH1D("h_integral_PMT" + tag, "h_integral_PMT" + tag + ";integral", 1000, -1.5e5, 1.5e5) );
    h_integral_vs_signal.push_back( new TH2D("h_integral_vs_signal_PMT" + tag, "h_integral_vs_signal" + tag + ";signal;integral;", 200, -1e4, 1e4, 200, -1e5, 1e5) );
  }
  
  // Evt loop!
  for (int ievt = 0; ievt < n; ++ievt) {
    cout << "Processing evt " << ievt << endl;
    std::vector<TH1D*> hists;
    std::vector<TH1D*> projs;
    
    // check whether event has at least one PMT with signal over threshold
    bool evtPassed = false;

    // PMT loop!
    for (int iPMT = 0; iPMT < nPMTs; iPMT++) {
      // cout << "  processing PMT " << iPMT << endl;
      mf -> Analyze(ievt, iPMT);

      bool isYAP = mf -> IsYAP(iPMT);
      bool isAirplane = mf -> IsAirplane(iPMT);
      bool isCLF = mf -> IsCLFSignal(iPMT);
      bool randSel = false;//(ih % irand == 0);
      
      std::vector<TSpike> spikes = mf -> GetSpikes(iPMT);
      for (unsigned int isp = 0; isp < spikes.size(); ++isp) {
	h_spikeIntegral[0] -> Fill(spikes[isp].GetVal());
	h_spikeIntegral[iPMT+1] -> Fill(spikes[isp].GetVal());
	h_spikeSize[0] -> Fill(spikes[isp].GetNbins());
	h_spikeSize[iPMT+1] -> Fill(spikes[isp].GetNbins());
      }
      
      TString name = Form("hist_PMT%i_evt%i", iPMT, ievt);
      if (isAirplane)
	name += "_plane";
      if (isYAP)
	name += "_yap";
      TString title = name;
      nh = nh+1;
      TH1D *hh = new TH1D(name, title.ReplaceAll("hist_", ""), nbins, ch1, ch2);
      std::vector<uint16> Data = mf -> GetData(iPMT);
      double pedestal = 1.*mf -> GetPedestal(iPMT);
      double median = 1.*mf -> GetMedian(iPMT);

      for (int ibin = 0; ibin < Data.size(); ++ibin) {
	double val =  -1.*(Data[ibin] - pedestal); 
	hh -> Fill(ibin, val);
      }

      TH1D *hYproj = MakeYproj(hh);
      
      double signal = mf -> GetSignal(iPMT);
      // this should well match the above;-)
      double integral = hh -> Integral(400, 1000);

      h_integral_vs_signal[0] -> Fill(signal, integral);
      h_signal[0] -> Fill(signal);
      h_integral[0] -> Fill(integral);
      h_integral_vs_signal[iPMT+1] -> Fill(signal, integral);
      h_signal[iPMT+1] -> Fill(signal);
      h_integral[iPMT+1] -> Fill(integral);
      
      // !!??
      // DEFAULT!!!
      bool passedThr = (signal > thr) && !IsSingleSpike(hh);
      // HACKs!!!
      //bool passedThr = (signal > thr);
      // bool passedThr = integral > thr;

      // HACK for special famous TA-matched events!;-)
      /*
      evtPassed =  (InFileName.Contains("FAST_2019_01_11_08h09m36s_trig0.data") && ievt == 324) ||
	(InFileName.Contains("FAST_2018_01_18_05h52m55s_trig0.data") && ievt == 283) || 
	(InFileName.Contains("FAST_2018_02_08_05h11m10s_trig0.data") && ievt == 421) || 
	(InFileName.Contains("FAST_2018_05_11_06h39m45s_trig0.data") && ievt == 283) || 
	(InFileName.Contains("FAST_2018_05_15_09h25m50s_trig0.data") && ievt == 126);
      */

      // default: just passed the threshold:
      if (passedThr) {
      	evtPassed = true;
      }
      
      
      
      // bool passedThr = ievt == 324; //283;
      // true; //
      // WAS: mf -> IsSignal(iPMT);
      if (fabs(signal) > thr)
	cout << "     passed thr! " << iPMT << " signal: " << signal << ": " << endl;
      cout << "        " << iPMT
	   << " integral=" << integral
	   << " signal=" << signal
	   << " median=" << median
	//<< " signal=" << signal
	   << endl;
      
      ///!!!! we want the change title for all PMT histos;)
      /// if (evtPassed || randSel) {
      if (randSel && ! evtPassed) {
	hh -> SetName(TString(hh -> GetName()) + "_rnd");
	hh -> SetTitle(TString(hh -> GetTitle()) + " random");
      }
      if (isCLF) {
	hh -> SetName(TString(hh -> GetName()) + "_clf");
	hh -> SetTitle(TString(hh -> GetTitle()) + " CLF");
      } else if (passedThr) {
	hh -> SetName(TString(hh -> GetName()) + "_thr");
	//hh -> SetTitle(TString(hh -> GetTitle()) + " over threshold");
      }
      //	TString titletag = Form(";bin;counts;median=%f|signal=%f|Airplane/YAP/CLF/Thr=%i%i%i|",
      //	TString titletag = Form(" | median=%.0f|signal=%.0f|Plane%i/YAP%i/CLF%i/Thr%i|",
      TString titletag = Form("|signal%.0f|Plane%i|YAP%i|CLF%i|Thr%i|",
			      // median,
			      signal, isAirplane, isYAP, isCLF, passedThr);
      if (spikes.size() > 0)
	titletag += "spikesAt";
      for (unsigned int isp = 0; isp < spikes.size(); ++isp) {
	titletag += spikes[isp].GetBin0();
	if (isp < spikes.size() -1 )
	  titletag += ",";
	else
	  titletag += "|";
      }	

      TString datestr = mf -> GetEventTime();
      titletag += datestr;
      hh -> SetTitle(TString(hh -> GetTitle()) + titletag);
      ///!!!} // passed
      
      // hh -> Rebin(2);
      hists.push_back(hh);
      projs.push_back(hYproj);
    } // loop over PMTs
    if (evtPassed) {
      Hists.push_back(hists);
      Projs.push_back(projs);
    }
  }
  delete mf;

  vector<TString> svec;
  SplitString(infilename.c_str(), "/", svec);
  TString outfilename = TString("histos_") + svec[svec.size()-1].ReplaceAll(".data", siteTag + ".root");
  cout << "Creating output file " << outfilename.Data() << endl;
  TFile *outfile = new TFile(outfilename, "recreate");

  cout << "Writing and transforming (FFT) " << Hists.size() << " events." << endl;
   for (int iH = 0 ; iH < Hists.size(); ++iH) {
    for (int ih = 0 ; ih < Hists[iH].size(); ++ih) {

      TH1D *hh = Hists[iH][ih];
      //if (ih % 10 == 0) {
      cout << "  writting " << hh -> GetName() << endl;
      //      }
      hh -> Write();
      
      TH1D *proj = Projs[iH][ih];
      proj -> Write();

      TVirtualFFT::SetTransform(0);
      TH1 *hm = 0;
      hm = hh -> FFT(hm, "MAG");
      hm -> SetName(TString("FFT_mag_") + hh -> GetName());
      hm -> SetTitle(TString("FFT_mag_") + hh -> GetTitle());
      TH1 *hph = 0;
      hph = hh -> FFT(hph, "PH");
      hph -> SetName(TString("FFT_ph_") + hh -> GetName());
      hph -> SetTitle(TString("FFT_ph_") + hh -> GetTitle());
    } // loop over individual PMT histograms
  } // loop over event lists of histograms

  for (int iPMT = -1; iPMT < nPMTs; iPMT++) {   
    h_integral_vs_signal[iPMT+1] -> Write();
    h_signal[iPMT+1] -> Write();
    h_integral[iPMT+1] -> Write();
    h_spikeIntegral[iPMT+1] -> Write();
    h_spikeSize[iPMT+1] -> Write();
  }
  outfile->Write();
  outfile -> Close();
  cout << "DONE!" << endl;

  return 0;

}

// __________________________________________________________________________________________
// __________________________________________________________________________________________
// __________________________________________________________________________________________
