#!/bin/bash

for i in `ls histos*20*Auger*.root | egrep "FAST_2020_03|FAST_2020_04" | egrep -v -i "sphere|yap"` ; do
#for i in `ls histos*2019*Auger*.root | egrep "FAST_2019_09_|FAST_2019_10_|FAST_2019_11" | egrep -v -i "sphere|yap"` ; do
    echo "${i}"
    ./plotFastEvents.py $i 0
done
