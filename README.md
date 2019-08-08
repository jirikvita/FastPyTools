# FastPyTools

# to be blamed:
# jk 8.8.2019
# jiri.kvita@upol.cz

https://github.com/jirikvita/FastPyTools.git

cd YOUR_FAST_SVN/io
ln -s YOUR_GIT_DIR/FastPyTools/scripts/Setup.sh .
./Setup.sh
make clean && make


Example running on AirFly files:
Steering:
In AnalyzePhysics.cxx modify lines
  //!!!DEFAULT
  //double thr = 25.e3; // counts
should have nonzero events:
./AnalyzePhysics /data/FAST/Auger/run190512/FAST_2019_05_13_08h40m03s.data Auger
to be tested:
./AnalyzePhysics /data/FAST/Auger/run190805/FAST_2019_08_06_01h13m06s.data Auger


Example plotting:
should contain events:
./plotFastEvents.py histos_FAST_2019_05_13_08h40m03s_Auger.root 0
to be tested:
./plotFastEvents.py histos_FAST_2019_08_06_01h13m06s_Auger.root 0



Example running on ROOT format:
Steering:
TODO


