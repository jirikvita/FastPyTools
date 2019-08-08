#!/bin/bash

# to be blamed:
# jk 8.8.2019
# jiri.kvita@upol.cz

gitdir=/home/qitek/work/github/FastPyTools/
#OLD: gitdir=../../mg5analysis/fast/

mv AirFlyFile.cc AirFlyFile.cc.orig
mv AirFlyFile.h AirFlyFile.h.orig
mv Makefile Makefile.orig

for i in RunAll_Auger.sh RunAll_TA.sh PlotAll_TA.sh PlotAll_Auger.sh ; do
   ln -s ${gitdir}/scripts/${i} .
done


for i in AirFlyFile.cc AnalyzePhysics.cxx AirFlyFile.h Makefile ; do
   ln -s ${gitdir}/source/${i} .
done


for i in `ls ${gitdir}/python/*.py` ; do
   ln -s ${i} .
done




