#!/bin/bash

#jk 28.3.2019

basepath=/data/FAST/

for subdir in `cd $basepath ; ls | grep run19` ; do
    echo "$subdir "
    if [ -d ${basepath}/$subdir ] ; then
	for j in `cd ${basepath}/${subdir}  ; ls *trig0*.data ` ; do
	    echo "   $j"
	    file=${basepath}/${subdir}/${j}
	    echo $file
            ./AnalyzePhysics $file TA
      done
    fi
done
