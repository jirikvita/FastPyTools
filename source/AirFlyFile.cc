#include <stdlib.h>  
#include <cmath>
#include <iostream>
#include <vector>

#include "AirFlyFile.h"

using namespace std;

unsigned int AirFlyFile::fPedestalFirst = 0;
unsigned int AirFlyFile::fPedestalLast = 400;
unsigned int AirFlyFile::fSignalFirst = 400;
unsigned int AirFlyFile::fSignalLast = 5000; // !!! jiri kvita WAS: 1000
//unsigned int AirFlyFile::fYAPFirst=172;
//unsigned int AirFlyFile::fYAPLast=195;
//unsigned int AirFlyFile::fYAPLengthThr = 10;
unsigned int AirFlyFile::fYAPLengthThrMin = 5;
unsigned int AirFlyFile::fYAPLengthThrMax = 25;
double  AirFlyFile::fYAPThreshold = 3200.;
unsigned int AirFlyFile::fSignificanceThreshold = 10;
unsigned int AirFlyFile::fSignalWidthThreshold = 1;
unsigned int AirFlyFile::fAirplaneRange = 100;
unsigned int AirFlyFile::fAirplaneSignificance = 10;
unsigned int AirFlyFile::fSpikeSignificance = 5;
unsigned int AirFlyFile::fYAPSignificance = 10;

int32 AirFlyFile::fFileForcedVersion = -1;
uint32 AirFlyFile::fNBlock = 2000; // Please change smaller value if you have memory problem.

AirFlyFile::~AirFlyFile() {
  //cout << "AirFlyFile::~AirFlyFile()" << endl;

  for (int c = 0; c < (int) fNChannel; c++)
    free(fBlockBuffer[c]);
  //    delete []  fBlockBuffer[c]; 
  delete fBlockBufferGPS;

  if (fFile) {
    fFile->close();
    delete fFile;
  }

}
AirFlyFile::AirFlyFile(string filename) {

  if (filename.length() > 0) {
    cout << "Opening  " << filename.c_str() << endl;
    fFile = new ifstream(filename.c_str());
    fFilename = filename.c_str();

    if (!fFile->is_open()) {
      cout << "Not found!" << endl;
      fFile = 0;
      return;
      //exit(0);
    } else
      ReadHeader();
  }

}

void AirFlyFile::ReadHeader() {

  char buf[11];
  fFile->read(buf, sizeof(buf));
  fFile->read((char*) &fFileVersion, sizeof(fFileVersion));

  cout << buf << " " << fFileVersion << endl;
  // file version 
  // 4 : SIS3350 ( header gps fadc[0] fadc[1] )
  // 5 : SIS3350 ( header fadc[0] fadc[1] gps )
  // 6 : SIS3316 ( header fadc[0] fadc[1] gps )

  fFile->read((char*) &fSeconds, 4);
  cout << "Seconds : " << fSeconds << endl;

  fFile->read((char*) &fN, sizeof(fN));

  fNChannel = 0;
  //  fFile->read((char*)&fMask,sizeof(fMask));
  /** check channel number for different format */
  if (fFileVersion > 5) {
    bool fMask[fMaxChannel];
    fFile->read((char*) &fMask, sizeof(fMask));
    for (int c = 0; c < fMaxChannel; c++) {
      if (fMask[c]) {
        fNChannel++;
        fModule.push_back(c / 16);
        fModuleChannel.push_back(c % 16);
        fAbsoluteChannel.push_back(c);
      }
    }
  } else {
    bool fMask[8];
    fFile->read((char*) &fMask, sizeof(fMask));
    for (int c = 0; c < 8; c++) {
      if (fMask[c]) {
        fNChannel++;
        fModule.push_back(c / 4);
        fModuleChannel.push_back(c % 4);
        fAbsoluteChannel.push_back(c);
      }
    }
  }

  if (fFileVersion > 5) {
    fFile->read((char*) &fTemperature, sizeof(fTemperature));
    cout << "Temperature(AD7314): " << fTemperature << " degC " << endl;
  }

  fFile->read((char*) &fFrequency, sizeof(fFrequency));
  fFile->read((char*) &fEventSize, sizeof(fEventSize));

  cout << "Event " << fN << " | " << dec << fFrequency << " MHz | size " << fEventSize << endl;

  if (fFileVersion > 5) {
    fEventSampleLength = (fEventSize) / 2 - (fEventHeader * 4);
  } else {
    fEventSampleSize = fEventSize - 16;
    fEventSampleLength = fEventSampleSize / 2;
    //  fEventSize = fEventSize+8; //P.P.GPS
  }

  cout << fN << " events ";
  cout << " (" << fEventSampleLength << " samples/event)" << endl;

  for (int c = 0; c < (int) fNChannel; c++)
    fData[c].resize(fEventSampleLength);

  fPosHeader = fFile->tellg();

  fFile->seekg(0, ios::end); //seek to the end 
  int size = fFile->tellg();        //tell the location of get pointer, ie.Find the size
  fFile->seekg(fPosHeader, ios::beg); // put the get pointer back to the end of Header

  int iteration = (size - fPosHeader) / (fEventSize * fNChannel + 16); //+16 (bytes) is for 4*32 bits GPS words
  int reminder = (size - fPosHeader) % (fEventSize * fNChannel + 16);
  if (reminder != 0)
    cout << "PROBLEM: incomplete file " << endl;
  fN = iteration;

  fBlockBufferGPS = new uint32[fN * 16];
  for (int c = 0; c < (int) fNChannel; c++) {
    if (fFileVersion > 5) {
      fBlockBuffer[c] = new uint32[fEventSize * fNBlock];
    } else {
      fBlockBuffer[c] = new uint32[fEventSize / 4 * fNBlock];
    }
    fTime[c].resize(fEventSampleLength);
    for (unsigned int i = 0; i < fEventSampleLength; i++)
      fTime[c][i] = i;
  }
  fEventBlock = -1;

}

void AirFlyFile::Load(unsigned int event, unsigned int channel) {
  fEvent = event;
  fChannel = channel;
  SetFilePointer();
  ReadEventHeader();
}

void AirFlyFile::Read(unsigned int event, unsigned int channel) {
  Load(event, channel);
  for (unsigned int i = 0; i < fEventSampleLength; i++)
    ReadData(channel, i);
}

void AirFlyFile::Analyze(unsigned int event, unsigned int channel) {
  Load(event, channel);
  ReadGPS();
  fPedestal[channel] = 0;
  fSignal[channel] = 0;
  fSignalWidth[channel] = 0;
  fSignalWidth_begin[channel] = 0;
  //if (channel == 0) fYAPSignal = 0;
  fIsSignal[channel] = false;
  fIsCLFSignal[channel] = false;
  fIsAirplane[channel] = false;
  fIsYAP[channel] = false;
  double startSignal = 0.0;
  double endSignal = 0.0;

  fVarianceOfPedestal[channel] = 0;
  fSignalError[channel] = 0;

  fMaxCount[channel] = 0;
  fMinCount[channel] = 1e5;
  fMaxBin[channel] = 0;
  fMinBin[channel] = 0;
  // jiri kvita
  fSpikes[channel].clear();
  
  fDeltaPedestal = fPedestalLast - fPedestalFirst;
  fDeltaSignal = fSignalLast - fSignalFirst;


  //if (channel == 0) fYAPLength = 0;
  for (unsigned int i = 0; i < fEventSampleLength; i++) {
    ReadData(channel, i);

    if (i >= fPedestalFirst && i < fPedestalLast)
      fPedestal[channel] += double(fData[channel][i]);
    if (i >= fSignalFirst && i < fSignalLast)
      fSignal[channel] += double(fData[channel][i]);

    //if (channel == 0 && i>= fYAPFirst && i<= fYAPLast) {
    //   fYAPSignal += double(fData[channel][i]);
    //   if (double(fData[channel][i]) < fYAPThreshold) fYAPLength += 1;
    //}

    if (i >= fPedestalFirst && i < fPedestalLast)
      fVarianceOfPedestal[channel] += double(fData[channel][i]) * double(fData[channel][i]);

    /** for airplane judgement */
    if (i >= 0 && i < (unsigned int) fAirplaneRange )
      startSignal += double(fData[channel][i]);
    if (i >= (fEventSampleLength - fAirplaneRange) && i < fEventSampleLength )
      endSignal += double(fData[channel][i]);


    //if(i>=fSignalFirst&&i<fSignalLast) {
    if (double(fData[channel][i]) < fMinCount[channel]) {
      fMinCount[channel] = double(fData[channel][i]);
      fMinBin[channel] = i;
    }
    if (double(fData[channel][i]) > fMaxCount[channel]) {
      fMaxCount[channel] = double(fData[channel][i]);
      fMaxBin[channel] = i;
    }
    //}

  }

  fPedestal[channel] /= fDeltaPedestal;
  fSignal[channel] -= (fPedestal[channel] * fDeltaSignal);
  fSignal[channel] *= -1.;
  //if (channel == 0) {
  //  fYAPSignal -= (fPedestal[channel]*(fYAPLast-fYAPFirst));
  //  fYAPSignal *= -1.;
  //}

  //P.P.  fSignal[channel] *= -1./fDeltaSignal;

  fVarianceOfPedestal[channel] = fVarianceOfPedestal[channel] / fDeltaPedestal
      - fPedestal[channel] * fPedestal[channel]; // valiance;

  if (fSignal[channel] < 0) {
    fSignalError[channel] = sqrt(-1 * fSignal[channel])
        + sqrt(fVarianceOfPedestal[channel]) * sqrt(fDeltaSignal);
  } else {
    fSignalError[channel] = sqrt(fSignal[channel])
        + sqrt(fVarianceOfPedestal[channel]) * sqrt(fDeltaSignal);
  }

  unsigned int peak = 0;
  if (fMaxCount[channel] - fPedestal[channel] > fPedestal[channel] - fMinCount[channel]) {
    fSignificance[channel] = (fMaxCount[channel] - fPedestal[channel])
        / sqrt(fVarianceOfPedestal[channel]);
    peak = fMaxBin[channel];
  } else {
    fSignificance[channel] = (fPedestal[channel] - fMinCount[channel])
        / sqrt(fVarianceOfPedestal[channel]);
    peak = fMinBin[channel];
  }

  startSignal /= (double) fAirplaneRange;
  endSignal /= (double) fAirplaneRange;
  startSignal -= fPedestal[channel];
  endSignal -= fPedestal[channel];
  double significance = (endSignal - startSignal) / sqrt(fVarianceOfPedestal[channel]) * sqrt(fAirplaneRange);
  // cout << significance << " " << startSignal << " " << endSignal << " " << sqrt(fVarianceOfPedestal[channel]) << endl;
  if(fabs(significance) >= fAirplaneSignificance)
    fIsAirplane[channel] = true;

  this -> MakeSpikes();
  // jiri kvita 2019
  for (unsigned int isp = 0; isp < fSpikes[channel].size(); ++isp) {
    // double spikeSignificance = -1;
    double val = fSpikes[channel][isp].GetVal();
    // double var = fSpikes[channel][isp].GetSigma();
    //if (var > 0) {
    //  spikeSignificance = (fSpikes[channel][isp].val - this->GetMedian(channel)) / (var);
    //  if (spikeSignificance > fYAPSignificance) {
    if (val > fYAPThreshold) {
      fIsYAP[channel] = true;
      break;
      }
    // }
  } // spikes
  

  
  
  // Signal width search with continuous width (average of 5 bins)
  double coef = 1.0;
  if (channel % 2 == 0)
    coef = -1.0;

  if (fFileVersion > 4) {
    coef = -1.0;
  }

  double sigma = 0;
  unsigned int movingBin = 40; // 25 ns * 40 bin = 1.0 us 

  double signal_peak = 0;
  for (unsigned int i = 0; i < fEventSampleLength; i++) {
    if (i >= fSignalFirst && i < fSignalLast) {
      double f_5sum = 0;
      for (unsigned int j = 0; j < movingBin; j++) {
        f_5sum = f_5sum + fData[channel][i + j] - fPedestal[channel];
      }
      sigma = coef * f_5sum / sqrt(movingBin * fVarianceOfPedestal[channel]);
      if (sigma > fSignificanceThreshold) {
        bool width_tag = true;
        unsigned int s_width = 0;
        double width_peak = 0;
        while (width_tag == true && (i + s_width) < fSignalLast) {
          double sigma_later = 0;
          double f_5sum_l = 0;
          for (unsigned int j = 0; j < movingBin; j++) {
            f_5sum_l = f_5sum_l + fData[channel][i + s_width + j] - fPedestal[channel];
          }
          sigma_later = coef * f_5sum_l / sqrt(movingBin * fVarianceOfPedestal[channel]);
          if (sigma_later > fSignificanceThreshold) {
            s_width++;
            if (sigma_later >= width_peak)
              width_peak = sigma_later;
          } else {
            width_tag = false;
          }
        }
        if (fSignalWidth[channel] < s_width) {
          fSignalWidth[channel] = s_width;
          fSignalWidth_begin[channel] = i;
          signal_peak = width_peak;
          fSignificance[channel] = width_peak;
        } else if (fSignalWidth[channel] == s_width) {
          if (width_peak > signal_peak) {
            fSignalWidth_begin[channel] = i;
            signal_peak = width_peak;
          }
        }
      }
    }
  }

  if (event == 0) {
    fTotalSignal[channel] = 0.0;
    fTotalPedestal[channel] = 0.0;
    fTotalVarianceOfPedestal[channel] = 0.0;
    fTotalSignalSelected[channel] = 0.0;
    fTotalSignalSquareSelected[channel] = 0.0;
    fNEventSelected[channel] = 0;
  }
  fTotalSignal[channel] += fSignal[channel];
  fTotalPedestal[channel] += fPedestal[channel];
  fTotalVarianceOfPedestal[channel] += fVarianceOfPedestal[channel];

  if (((((fNanoSecond * 10) % 100000000) <= 500000) || ((fNanoSecond * 10) % 100000000) >= 99100000)
      && ((fSecond % 1800) <= 30)) {
    fIsCLFSignal[channel] = true;
  }

  // Selection by s/n , peak position and width
  if (fSignificance[channel] >= fSignificanceThreshold
      &&
      //fSignificance[channel] <= 20 &&
      fSignalWidth[channel] >= fSignalWidthThreshold && fSignalFirst <= peak && peak <= fSignalLast
      && (!fIsCLFSignal[channel])) { //not including CLF events
    fTotalSignalSelected[channel] += fSignal[channel];
    fTotalSignalSquareSelected[channel] += fSignal[channel] * fSignal[channel];
    fNEventSelected[channel]++;
    fIsSignal[channel] = true;
  }

  return;

}

void AirFlyFile::ReadEventHeader() {
  uint32 idx = 0;
  if (fFileVersion > 5) {
    idx = ((fEvent % fNBlock) * (fEventSize));
  } else {
    idx = ((fEvent % fNBlock) * (fEventSize) / 4);
  }

  volatile uint16 timestamp[4];

  if (fFileVersion > 5) {
    // TODO    
  } else {

    //fFile->read((char*)&timestamp,sizeof(timestamp));
    timestamp[0] = fBlockBuffer[fChannel][idx];
    timestamp[1] = ((fBlockBuffer[fChannel][idx] & 0xffff0000) >> 16);
    timestamp[2] = fBlockBuffer[fChannel][idx + 1];
    timestamp[3] = ((fBlockBuffer[fChannel][idx + 1] & 0xffff0000) >> 16);

    /*  
     int tlow = (bsw(timestamp[2])<<12) + 
     (bsw(timestamp[3]));
     int tup = (bsw(timestamp[0])<<12) + 
     (bsw(timestamp[1]));
     double time = (double)(16777216*tup)+(double)tlow;
     */

    fTimeStamp = ((bsw(timestamp[0]) & 0xfffLLU) << 36) + ((bsw(timestamp[1]) & 0xfffLLU) << 24)
        + ((bsw(timestamp[2]) & 0xfffLLU) << 12) + ((bsw(timestamp[3]) & 0xfffLLU));

    //cout  << " Time stamp = " << fTimeStamp << endl ;

  }

  volatile uint16 header[4];

  //fFile->read((char*)&header,sizeof(header));
  header[0] = fBlockBuffer[fChannel][idx + 2];
  header[1] = ((fBlockBuffer[fChannel][idx + 2] & 0xffff0000) >> 16);
  header[2] = fBlockBuffer[fChannel][idx + 3] & 0xfff;
  header[3] = ((fBlockBuffer[fChannel][idx + 3] & 0xffff0000) >> 16);

  unsigned int l;

  if (fFileVersion > 5) {
    l = (header[0] & 0x3ffffff) * 2;
  } else {
    l = bsw(header[3]) | (bsw(header[2]) << 12) | (bsw(header[1]) >> 24);
  }

  //  cout << dec << "Read Header  " << idx << " " << l
  //       << " fEvent " << fEvent << " Channel " << fChannel 
  //       << " Trigger counter  " << (header[0]) << " " << (header[0]&0x0f0>>4) << endl ;

  if (l != fEventSampleLength) {
    /*
     cout << "Warning reading event " << dec << fEvent << " channel " 
     << fChannel << endl;
     cout << "Sample length from event header " << l 
     << " inconsistent with file header " <<  fEventSampleLength << endl; 
     */
  }

}

void AirFlyFile::SetFilePointer() {
  //cout << fEvent << " " << fChannel ;

  uint32 *fBlockTemp;
  if (fFileVersion > 5) {
    fBlockTemp = new uint32[fEventSize];
  } else {
    fBlockTemp = new uint32[fEventSize / 4];
  }

  uint32 *fBlockTempGPS = new uint32[16];
  int nblock = fEvent / fNBlock;
  if (nblock != fEventBlock) {
    fEventBlock = nblock;
    int max = fNBlock;
    if (fN < ((fEventBlock + 1) * fNBlock))
      max = fN - fEventBlock * fNBlock;
    for (int iev = 0; iev < max; iev++) {
      ifstream::pos_type fPosGPS;
      ifstream::pos_type fPos;
      if (fFileVersion > 4) {
        fPosGPS = fPosHeader
            + (ifstream::pos_type) ((fEvent + iev + 1) * fEventSize * fNChannel
                + 16 * (fEvent + iev));
      } else {
        fPosGPS = fPosHeader
            + (ifstream::pos_type) ((fEvent + iev) * fEventSize * fNChannel + 16 * (fEvent + iev));
      }
      fFile->seekg(fPosGPS, ios::beg);
      fFile->read((char*) fBlockTempGPS, 16);
      for (unsigned int i = 0; i < 16; i++)
        fBlockBufferGPS[i + 16 * iev] = fBlockTempGPS[i];

      for (int c = 0; c < (int) fNChannel; c++) {
        if (fFileVersion > 4) {
          fPos = fPosHeader
              + (ifstream::pos_type) ((fEvent + iev) * fEventSize * fNChannel + c * fEventSize
                  + 16 * (fEvent + iev));
          fFile->seekg(fPos, ios::beg);
        } else {
          /*
           ifstream::pos_type fPos = fPosHeader
           + (ifstream::pos_type)((fEvent+iev)*fEventSize*fNChannel
           + c*fEventSize+16*(fEvent+iev));
           fFile->seekg(fPos,ios::beg);
           */
        }
        fFile->read((char*) fBlockTemp, fEventSize);

        int size = 0;
        if (fFileVersion > 5) {
          size = fEventSize;
          for (unsigned int i = 0; i < (unsigned int) (size - fEventHeader); i++)
            fBlockBuffer[c][i + size * iev] = fBlockTemp[i + fEventHeader];
        } else {
          size = fEventSize / 4;
          for (unsigned int i = 0; i < (unsigned int) size; i++)
            fBlockBuffer[c][i + size * iev] = fBlockTemp[i];
        }

      }
    }
  }
  delete fBlockTemp;
  delete fBlockTempGPS;
}


// jiri kvita 2019
std::vector<TSpike> AirFlyFile::GetSpikes(unsigned int channel)
{
  return fSpikes[channel];
}
void AirFlyFile::MakeSpikes() {
  // so far define the spike not as a few-bin YAP signal
  // but simply as the bin with most deviating content from median, and save its stat error
  for (int channel = 0; channel < (int) fNChannel; channel++) {
    /*
    std::vector<bool> lookedAt;
    for (unsigned int i = 0; i < fEventSampleLength; i++) {
      lookedAt.push_back(false);
    }
    */
    
    unsigned int median = this -> GetMedian(channel);
    bool inChainOfContinuousPositiveVals = false;
    unsigned int nChannel = 0;
    double accumulated = 0.;
    unsigned int NinChain = 0;
    unsigned int bin0 = 0;
    for (unsigned int i = 0; i < fEventSampleLength; i++) {
      double val = -1.*(fData[channel][i] - fPedestal[channel]);
      double sigma = -1;
      double significance = -1;
      if (val > 0.) {
	sigma = sqrt(1.*val);
	// significance =  (val - median) / sigma;
	significance =  val / sigma;
      }
      if (val > 0. && sigma > 0 && significance > fSpikeSignificance) {
	NinChain++;
	if (!inChainOfContinuousPositiveVals)
	  bin0 = i;
	inChainOfContinuousPositiveVals = true;
	//  fSpikes[channel].push_back( std::make_pair(val, sigma) );
	accumulated += val;
	//}
      } else {
	// end of chain, let's analyze the cluster we accumulated:
	if (inChainOfContinuousPositiveVals &&
	    NinChain >= fYAPLengthThrMin &&
	    NinChain <= fYAPLengthThrMax &&
	    accumulated > fYAPThreshold) {
	  TSpike spike(accumulated, NinChain,bin0);
	  significance = spike.GetSignificance();
	  cout << "Found spike channel=" << channel << " bin0=" << bin0 << " N=" << NinChain << " sum=" << accumulated << " signif=" << significance << endl;
	  fSpikes[channel].push_back(spike);
	}
	// let's reset and continue;)
	inChainOfContinuousPositiveVals = false;
	NinChain = 0;
	accumulated = 0.;
	bin0 = 0;
      }
    } // data
  } // channel
}



// jiri kvita 2019
// http://www.cplusplus.com/forum/general/30804/
int AirFlyFile::GetMedian(unsigned int channel) {
  std::vector<double> arr;
  for (unsigned int i = 0; i < fEventSampleLength; i++) {
    arr.push_back(-1.*( fData[channel][i] - fPedestal[channel]) );
  }
  unsigned int size = arr.size(); //sizeof(arr)/sizeof(unsigned int);
  std::sort(&arr[0], &arr[size]);
  int median = size % 2 ? 1.*arr[size / 2] : (1.*arr[size / 2 - 1] + 1.*arr[size / 2]) / 2.;
  return median;
}
