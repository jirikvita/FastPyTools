#!/bin/bash

#jk 28.3.2019

basepath=/data/FAST/Auger/

for subdir in `cd $basepath ; ls | grep run19` ; do
    echo "$subdir "
    if [ -d ${basepath}/$subdir ] ; then
	for j in `cd ${basepath}/${subdir}  ; ls *.data  ` ; do
	    echo "   $j"
	    file=${basepath}/${subdir}/${j}
	    echo $file
            ./AnalyzePhysics $file Auger
      done
    fi
done
