#!/bin/bash

# jk 8.8.2019

gitdir=/home/qitek/work/github/FastPyTools/source
#OLD: gitdir=../../mg5analysis/fast/

mv AirFlyFile.cc AirFlyFile.cc.orig
mv AirFlyFile.h AirFlyFile.h.orig
mv Makefile Makefile.orig

for i in RunAll_Auger.sh RunAll_TA.sh PlotAll_TA.sh PlotAll_Auger.sh AirFlyFile.cc AnalyzePhysics.cxx AirFlyFile.h Makefile ; do
   ln -s ${gitdir}/${i} .
done
