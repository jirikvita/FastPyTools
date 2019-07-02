#!/bin/bash

for i in histos_FAST_2019_01_11_08h09m36s_trig0.root histos_FAST_2018_01_18_05h52m55s_trig0.root histos_FAST_2018_02_08_05h11m10s_trig0.root histos_FAST_2018_05_11_06h39m45s_trig0.root histos_FAST_2018_05_15_09h25m50s_trig0.root ; do
    echo "./plotFastEvents.py $i"
    j=`basename $i .root`
    ./plotFastEvents.py $i 0 >& log_${j}.txt
done
