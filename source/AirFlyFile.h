#ifndef AIRFLYFILE_H

#include <iostream>
#include <fstream>
#include <string>
#include <vector>
#include <utils.h>
#include <string.h>
#include <algorithm>
#include <math.h>

using namespace std;

// jiri kvita
class TSpike {
 private:
  unsigned int fval;
  unsigned int fnbins;
  unsigned int fbin0;
 public:
 TSpike(unsigned int val, unsigned int nbins, unsigned int bin0) : fval(val), fnbins(nbins), fbin0(bin0) {}
  ~TSpike() {};
  unsigned int GetVal() { return fval; }
  unsigned int GetNbins() { return fnbins; }
  unsigned int GetBin0() { return fbin0; }
  double GetSignificance() { if (fval > 0.) return sqrt(fval); return -1;}
  double GetSigma() { return this->GetSignificance();};
};


class AirFlyFile {
  public:
    static void SetFileVersion(uint32 version) {
      fFileForcedVersion = version;
    }
    static void SetBlockSize(uint32 size) {
      fNBlock = size;
    }
    static void SetPedestalRegion(unsigned int first, unsigned int last) {
      fPedestalFirst = first;
      fPedestalLast = last;
    }
    static void SetSignalRegion(unsigned int first, unsigned int last) {
      fSignalFirst = first;
      fSignalLast = last;
    }
    static void SetSignificanceThreshold(unsigned int sn) {
      fSignificanceThreshold = sn;
    }
    static void SetSignalWidthThreshold(unsigned int width) {
      fSignalWidthThreshold = width;
    }

    AirFlyFile(std::string filename = "");
    ~AirFlyFile();

    void Load(unsigned int event, unsigned int channel);
    void Read(unsigned int event, unsigned int channel);
    void Analyze(unsigned int event, unsigned int channel);

    void ReadHeader();

    std::vector<uint16> & GetData(unsigned int c) {
      return fData[c];
    }
    std::vector<int> & GetTime(unsigned int c) {
       return fTime[c];
    }

    bool IsOpen() {
      if (fFile)
        return (fFile->is_open());
      else
        return (fFile);
    }

    inline void ReadData(unsigned int c, unsigned int pos) {
      int sh = ((pos % 2) * 16); //0 or 16
      uint32 idx = 0;
      if (fFileVersion > 5) {
        idx = (fEvent % fNBlock) * (fEventSize); // without event header
        fData[c][pos] = ((fBlockBuffer[c][pos / 2 + idx]) & (0x3fff << sh)) >> sh; // 14 bit
        /** To fit 0 to 16383, we need to find a reason... */
        if (fData[c][pos] < 8192) {
          fData[c][pos] += 8192;
        } else {
          fData[c][pos] -= 8192;
        }

      } else {
        idx = ((fEvent % fNBlock) * fEventSize / 4) + 4; // add for time stamp
        fData[c][pos] = (bsw(fBlockBuffer[fChannel][pos / 2 + idx]) & (0xfff << sh)) >> sh;
      }
      /*
       cout << "FADC sh " << sh
       << " idx " << idx
       << " c " << c
       << " pos " << pos << endl;
       */
    }


    /**** new by Petr Hamal!!! ****/
    inline void ReadGPS()
    {
        /*
         * INFO: GPS time is read as it is, no seconds are subctracted or added
         *     : A starting date of FAST GPS is probably Jan 6th 1996, thus GPS offset to
         *       GPS epoch (Jan 6th 1980) is 504921611. One can check this on a website
         *       https://www.andrews.edu/~tzs/timeconv/timeconvert.php
         *       (input time Jan 6th 1996 00:00:00 24-hour UTC)
        */
        uint32 idx = (fEvent % fNBlock) * 16;
        if (fFileVersion <= 4)
        {
            idx = ((fEvent + 1) % fNBlock) * 16;
        }
        fYear = (fBlockBufferGPS[idx]);
        fDay = (fBlockBufferGPS[idx + 1]);
        fSecond = (fBlockBufferGPS[idx+2]);
        fNanoSecond = (fBlockBufferGPS[idx + 3]);
    }

    /**** new by Petr Hamal!!! ****/
    unsigned int GetGPSTimeInSeconds()
    {
        /*
         * Returns a number of GPS seconds since GPS epoch in 1980
         * No leap seconds are subtracted as the GPS time is absolute
         * time since its defined beginning.
         *
         * iOffset     a number of GPS second between Jan 6th 1980 and
         *             Jan 6th 1996 (a beginning of FAST GPS counting)
         *             Check it on https://www.andrews.edu/~tzs/timeconv/timeconvert.php
         *             (input time Jan 6th 1996 00:00:00 24-hour UTC)
        */

        unsigned int iFastGPS = GetFASTGPSInSeconds();
        unsigned int iOffset = 504921611;
        unsigned int iFASTGPSOffset = 14;
        unsigned int iGPS = iFastGPS + iOffset - iFASTGPSOffset;
//        std::cout << "fast, offset, sum: " << iFastGPS << "  " << iOffset << "  " << iGPS << std::endl;
        return iGPS;
    }

    /**** new by Petr Hamal!!! ****/
    unsigned int GetFASTGPSInSeconds()
    {
        /*
         * FAST GPS time in seconds
        */
        ReadGPS();
        return ((fYear * 365 * 24 + fDay * 24) * 3600 + fSecond);
    }

        /**** new by Petr Hamal!!! ****/
    unsigned int GetUTCtime()
    {
        /*
         *
         * iGPSoffset
         * iLeapSecond    A number of leap seconds between GPS and UTC, from
         *                GPS epoch in 1980 to 2019-05-23
         *                Has to be manually changed if a leap second will be
         *                counted.
        */
        unsigned int iGPSoffset = 315964800;
        unsigned int iLeapSeconds = 18;
        unsigned int iUnix = GetGPSTimeInSeconds() + iGPSoffset - iLeapSeconds;
//        std::cout << "leap, offset, utc: " << iLeapSeconds << "  " << iGPSoffset << "  "
//                  << iUnix << std::endl;
        return iUnix;
    }

        /**** new by Petr Hamal!!! ****/
    string GetEventTime()
    {
        time_t rawtime = (time_t)GetUTCtime();
        struct tm * ptm;
        ptm = gmtime(&rawtime);

        char c[30];
        sprintf(c, "%04d/%02d/%02d %02d:%02d:%02d.%09d",
                ptm->tm_year + 1900,
                ptm->tm_mon + 1,
                ptm->tm_mday,
                ptm->tm_hour,
                ptm->tm_min,
                ptm->tm_sec,
                GetGPSNanosecond());
        string s = c;
        return s;
    }

    /*** end of all new by Petr Hamal ***/

    string GetFilename() {
      return fFilename;
    }
    unsigned int GetGPSYear() {
      return fYear;
    }
    unsigned int GetGPSDay() {
      return fDay;
    }
    unsigned int GetGPSSecond() {
      return fSecond;
    }
    unsigned int GetGPSNanosecond() {
      return fNanoSecond * 10;
    } //  ok checked with pulser rate
      //  double GetUTCtime() { return((fYear*365*24+fDay*24)*3600.+fSecond*1.+fNanoSecond/1.e8); }

    // jiri kvita 2019
    int GetMedian(unsigned int channel);
    std::vector<TSpike> GetSpikes(unsigned int channel);
    void MakeSpikes();
    
    unsigned int GetEventLength() {
      return fEventSampleLength;
    }
    unsigned int GetN() {
      return fN;
    }

    double GetFrequency() {
      return fFrequency;
    }

    double GetTemperature() {
      return fTemperature;
    }

    bool IsSignal(unsigned int c) {
      return fIsSignal[c];
    }

    bool IsCLFSignal(unsigned int c) {
      return fIsCLFSignal[c];
    }

    unsigned int GetNChannel() {
      return fNChannel;
    }
    /*
     bool GetChannelStatus(unsigned int module, unsigned int module_channel)
     { return fMask[module*(nChannel/2)+module_channel];}
     bool GetChannelStatus(unsigned int absolute_channel)
     { return fMask[absolute_channel];}
     */

    unsigned int GetModule(unsigned int channel) {
      return fModule[channel];
    }
    unsigned int GetModuleChannel(unsigned int channel) {
      return fModuleChannel[channel];
    }
    unsigned int GetAbsoluteChannel(unsigned int channel) {
      return fAbsoluteChannel[channel];
    }

    int MinTime(unsigned int channel) {
      return 0;
    }
    int MaxTime(unsigned int channel) {
      return (fEventSampleLength);
    }

    uint64 GetTimeStamp() {
      return fTimeStamp;
    }

    double GetPedestal(unsigned int c) {
      return (fPedestal[c]);
    }
    double GetVarianceOfPedestal(unsigned int c) {
      return (fVarianceOfPedestal[c]);
    }
    double GetTotalPedestal(unsigned int c) {
      return (fTotalPedestal[c]);
    }
    double GetTotalVarianceOfPedestal(unsigned int c) {
      return (fTotalVarianceOfPedestal[c]);
    }
    double GetDeltaPedestal() {
      return (fDeltaPedestal);
    }
    double GetSignal(unsigned int c) {
      return (fSignal[c]);
    }
    double GetSignalWidth(unsigned int c) {
      return (fSignalWidth[c]);
    }
    unsigned int GetSignalWidth_begin(unsigned int c) {
      return (fSignalWidth_begin[c]);
    }
    double GetSignalError(unsigned int c) {
      return (fSignalError[c]);
    }
    double GetTotalSignal(unsigned int c) {
      return (fTotalSignal[c]);
    }
    double GetDeltaSignal() {
      return (fDeltaSignal);
    }
    double GetTotalSignalSelected(unsigned int c) {
      return (fTotalSignalSelected[c]);
    }
    double GetTotalSignalSquareSelected(unsigned int c) {
      return (fTotalSignalSquareSelected[c]);
    }
    double GetNEventSelected(unsigned int c) {
      return (fNEventSelected[c]);
    }
    double GetSignificance(unsigned int c) {
      return (fSignificance[c]);
    }
    double GetMinBin(unsigned int c) {
      return (fMinBin[c]);
    }
    double GetMaxBin(unsigned int c) {
      return (fMaxBin[c]);
    }

    int GetFileVersion() {
      return fFileVersion;
    }

    bool IsAirplane(unsigned int c) {
      return fIsAirplane[c];
    }
    bool IsYAP(unsigned int c) {
      return fIsYAP[c];
    }

  protected:

    void SetFilePointer();
    void ReadEventHeader();
    void ReadEventData() {
      ;
    }

  private:
    static int32 fFileForcedVersion;
    static uint32 fNBlock;

    int32 fFileVersion;

    static const int fMaxChannel = 32;  // need to change 32 in case of SIS3316

    //volatile uint32 fBuffer[4096];
    //  uint32 *fBlockBuffer[8];

    uint32 *fBlockBuffer[fMaxChannel];
    uint32 *fBlockBufferGPS;

    unsigned int fEvent;
    unsigned int fChannel; //relative, from 0 to fNChannel

    uint32 fSeconds;

    unsigned int fN;
    unsigned int fEventSize;
    unsigned int fEventSampleSize;
    unsigned int fEventSampleLength;

    unsigned long long fTimeStamp;

    double fFrequency;
    double fTemperature;

    //absolute channel number; all the rest are relative
    //bool fMask[fMaxChannel];

    bool fIsSignal[fMaxChannel];
    bool fIsCLFSignal[fMaxChannel];
    int fIsAirplane[fMaxChannel];
    int fIsYAP[fMaxChannel];

    std::vector<TSpike> fSpikes[fMaxChannel];

    unsigned int fNChannel; // Total number of channels in file

    //vectors of size fNChannel
    std::vector<int> fModule; //0 or 1
    std::vector<int> fModuleChannel; //0 to 4 -> 16
    std::vector<int> fAbsoluteChannel; //0 to 8 -> 32

    std::vector<uint16> fData[fMaxChannel];
    std::vector<int> fTime[fMaxChannel];

    double fPedestal[fMaxChannel];
    static unsigned int fPedestalFirst, fPedestalLast;
    static unsigned int fSignalFirst, fSignalLast;
    unsigned int fDeltaPedestal;
    unsigned int fDeltaSignal;

    double fSignal[fMaxChannel];
    double fTotalSignal[fMaxChannel];
    unsigned int fSignalWidth[fMaxChannel];
    unsigned int fSignalWidth_begin[fMaxChannel];

    double fVarianceOfPedestal[fMaxChannel];
    double fTotalPedestal[fMaxChannel];
    double fTotalVarianceOfPedestal[fMaxChannel];
    double fSignalError[fMaxChannel];
    double fSignificance[fMaxChannel];
    double fTotalSignalSelected[fMaxChannel];
    double fTotalSignalSquareSelected[fMaxChannel];
    unsigned int fNEventSelected[fMaxChannel];
    double fMaxCount[fMaxChannel];
    double fMinCount[fMaxChannel];
    unsigned int fMaxBin[fMaxChannel];
    unsigned int fMinBin[fMaxChannel];

    string fFilename;

    static unsigned int fSignificanceThreshold;
    static unsigned int fSignalWidthThreshold;
    static unsigned int fAirplaneRange;
    static unsigned int fAirplaneSignificance;
    static unsigned int fSpikeSignificance;
    static unsigned int fYAPSignificance;

    std::ifstream *fFile;
    std::ifstream::pos_type fPos;
    std::ifstream::pos_type fPosHeader;

    int fEventBlock;

    static const int fEventHeader = 3;

    uint32 fYear;
    uint32 fDay;
    uint32 fSecond;
    uint32 fNanoSecond;

    //static unsigned int fYAPFirst;
    //static unsigned int fYAPLast;
    static double fYAPThreshold;
    // int fYAPLength;
    // static unsigned int fYAPLengthThr;
    static unsigned int fYAPLengthThrMin;
    static unsigned int fYAPLengthThrMax;
    //double fYAPSignal;

};

#endif
