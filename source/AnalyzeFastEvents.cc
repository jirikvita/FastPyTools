#include "FASTEventFile.h"
#include "FASTEvent.h"
#include "TFile.h"
#include "TTree.h"

#include "TH1D.h"
#include "TH2D.h"
#include "TString.h"



#include <iostream>

// JK:
// FFT based on https://root.cern.ch/root/html/tutorials/fft/FFT.C.html
#include "TVirtualFFT.h"

#include <iostream>
#include <vector>


using namespace std;


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

// __________________________________________________________________________________________
// __________________________________________________________________________________________
// __________________________________________________________________________________________


int main(int argc, char *argv[]) {

  if (argc < 3) {
    cout << "Usage: " << argv[0] << " path/filename.root siteTag=TA/Auger" << endl;
    cout << "    example: " << argv[0] << " /data/FAST/Auger_ROOT/run190507/FAST_2019_05_07_23h43m33s.root Auger" << endl;
    return 1;
  }


  std::string infilename = argv[1];
  TString InFileName = infilename.c_str();

  TString siteTag = TString("_") + TString(argv[2]);
  

  std::vector< std::vector<TH1D*> > Hists;
  std::vector< std::vector<TH1D*> > Projs;


  double ch1 = 0;
  double ch2 = 100;
  int nbins = 5000; //int(ch2-ch1);
  int nh = -1;
  
  double thr = 2300.; //!!!25.e3; // counts
  int irand = nh / 4;

  //  vector<TH1D*> h_signal;
  vector<TH1D*> h_integral;
  // vector<TH2D*> h_integral_vs_signal;
  //vector<TH1D*> h_spikeSize;
  //vector<TH1D*> h_spikeIntegral;

  // HACK!!!
  int nPix = 4;
  
  for (int ipix = -1; ipix < nPix; ipix++) {
    TString tag = "all";
    if (ipix >= 0)
      tag = Form("%i", ipix);
    
    //    h_spikeIntegral.push_back( new TH1D("h_spikeIntegral_PMT" + tag, "h_spikeIntegral_PMT" + tag + ";spike integral", 800, 2000, 10000) );
    //    h_spikeSize.push_back( new TH1D("h_spikeSize_PMT" + tag, "h_spikeSize_PMT" + tag + ";spike size", 20, 0, 20) );
    
    //    h_signal.push_back( new TH1D("h_signal_PMT" + tag, "h_signal_PMT" + tag + ";signal", 1000, -1e4, 1e4) );
    h_integral.push_back( new TH1D("h_integral_PMT" + tag, "h_integral_PMT" + tag + ";integral", 1000, -1.5e5, 1.5e5) );
    //    h_integral_vs_signal.push_back( new TH2D("h_integral_vs_signal_PMT" + tag, "h_integral_vs_signal" + tag + ";signal;integral;", 200, -1e4, 1e4, 200, -1e5, 1e5) );
  }

  FASTEvent* myEvent = new FASTEvent();

  //  for (int iFile = 1; iFile < argc; ++iFile)
  //  {
      // read filename as argument
      FASTEventFile myFile(infilename, FASTEventFile::eRead);
      myFile.SetBuffers(myEvent);
      cout << "reading file " << myFile.GetFileName() << " with " << myFile.GetNEvents() << " events." << endl;

      for (int iEvent = 0; iEvent < myFile.GetNEvents(); ++iEvent)
        {
	  // if (myFile.ReadEvent(iEvent) != FASTEventFile::eSuccess)
	  //     continue;
	  if (myFile.ReadEvent(myEvent) != FASTEventFile::eSuccess)
	    continue;

	  cout << "Processing evt " << iEvent << endl;
	  std::vector<TH1D*> hists;
	  std::vector<TH1D*> projs;
	  bool evtPassed = false; 
	  
	  std::vector<FASTPixel> pixels = myEvent -> GetPixels();
	  for (int ipix = 0; ipix < pixels.size(); ++ipix) {
	    bool isYAP = false; //mf -> IsYAP(ipix);
	    bool isAirplane = false; //mf -> IsAirplane(ipix);
	    bool isCLF = false; //mf -> IsCLFSignal(ipix);
	    bool randSel = false;//(ih % irand == 0);
	    
	    std::vector< double > trace = pixels[ipix].GetTrace ();
	    // fill histo here;)
	    TString name = Form("hist_PMT%i_evt%i", ipix, iEvent);
	    if (isAirplane)
	      name += "_plane";
	    if (isYAP)
	      name += "_yap";
	    TString title = name;
	    nh = nh+1;

	    // fill!
	    TH1D *hh = new TH1D(name, title.ReplaceAll("hist_", ""), nbins, ch1, ch2);
	    for (int ibin = 0; ibin < trace.size(); ++ibin) {
	      double val =  trace[ibin]; // -1*(....  - pedestal); 
	      if (fabs(val) > 0.) {
		hh -> SetBinContent(ibin, val);
		hh -> SetBinError(ibin, sqrt(fabs(val)));
	      }
	    } // bins
	    TH1D *hYproj = MakeYproj(hh);

	    double integral = hh -> Integral(); // range???
	    
	    //h_integral_vs_signal[0] -> Fill(signal, integral);
	    //h_signal[0] -> Fill(signal);
	    h_integral[0] -> Fill(integral);
	    //h_integral_vs_signal[ipix+1] -> Fill(signal, integral);
	    //h_signal[ipix+1] -> Fill(signal);
	    h_integral[ipix+1] -> Fill(integral);
      
	    // !!??
	    bool passedThr = (integral > thr); // && !IsSingleSpike(hh);
	    //      bool passedThr = integral > thr;

	    // HACK for special famous TA-matched events!;-)
	    /*
	      evtPassed =  (InFileName.Contains("FAST_2019_01_11_08h09m36s_trig0.data") && iEvent == 324) ||
	      (InFileName.Contains("FAST_2018_01_18_05h52m55s_trig0.data") && iEvent == 283) || 
	      (InFileName.Contains("FAST_2018_02_08_05h11m10s_trig0.data") && iEvent == 421) || 
	      (InFileName.Contains("FAST_2018_05_11_06h39m45s_trig0.data") && iEvent == 283) || 
	      (InFileName.Contains("FAST_2018_05_15_09h25m50s_trig0.data") && iEvent == 126);
	    */

	    // default: just passed the threshold:
	    if (passedThr) {
	      evtPassed = true;
	    }
      
	    // bool passedThr = iEvent == 324; //283;
	    // true; //
	    // WAS: mf -> IsSignal(ipix);
	    if (fabs(integral) > thr)
	      cout << "     passed thr! " << ipix << " integral: " << integral << ": " << endl;
	    cout << "        " << ipix
		 << " integral=" << integral
	      //<< " signal=" << signal
	      //<< " median=" << median
	      //<< " signal=" << signal
		 << endl;

	    /*
	    ///!!!! we want the change title for all PMT histos;)
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
	    */

	    //	TString titletag = Form(";bin;counts;median=%f|integral=%f|Airplane/YAP/CLF/Thr=%i%i%i|",
	    //	TString titletag = Form(" | median=%.0f|integral=%.0f|Plane%i/YAP%i/CLF%i/Thr%i|",
	    TString titletag = Form("|integral%.0f|Plane%i|YAP%i|CLF%i|Thr%i|",
				    // median,
				    integral, isAirplane, isYAP, isCLF, passedThr);
	    /*
	    if (spikes.size() > 0)
	      titletag += "spikesAt";
	    for (unsigned int isp = 0; isp < spikes.size(); ++isp) {
	      titletag += spikes[isp].GetBin0();
	      if (isp < spikes.size() -1 )
		titletag += ",";
	      else
		titletag += "|";
	    }	
	    */
	    
	    TString datestr = ""; // TODO!!!  // mf -> GetEventTime();
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

      /*
	FASTShowerData showerData = myEvent->GetShowerSimData();
	double zenith = showerData.GetZenith();
	double azimuth = showerData.GetAzimuth();
	double xmax = showerData.GetXmax();
	double energy = showerData.GetEnergy();
	
	// print out event id stored in file
	cout << "iEvent: " << iEvent << " Event Id: " << myEvent->GetEventId() << " Energy: " << energy << 
	" Xmax: " << xmax << " Zenith: " << zenith << " Azimuth: " << azimuth << endl;
      */
      // }
  
  delete myEvent;





  vector<TString> svec;
  SplitString(infilename.c_str(), "/", svec);
  TString outfilename = TString("histos_") + svec[svec.size()-1].ReplaceAll(".root", siteTag + "_new.root");
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

  for (int ipix = -1; ipix < nPix; ipix++) {   
    //h_integral_vs_signal[ipix+1] -> Write();
    //h_signal[ipix+1] -> Write();
    h_integral[ipix+1] -> Write();
    //h_spikeIntegral[ipix+1] -> Write();
    //h_spikeSize[ipix+1] -> Write();
  }
  outfile->Write();
  outfile -> Close();
  cout << "DONE!" << endl;

  return 0;
}
