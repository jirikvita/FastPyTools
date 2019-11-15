#!/bin/bash

#jk 28.3.2019

basepath=/data/FAST/TA

for subdir in `cd $basepath ; ls | egrep "run1904|run1905|run1906|run1907|run1908|run1909"` ; do
    echo "$subdir "
    if [ -d ${basepath}/$subdir ] ; then
	for j in `cd ${basepath}/${subdir}  ; ls *trig0*.data* ` ; do
	    echo "   $j"
	    file=${basepath}/${subdir}/${j}
	    echo $file
            ./AnalyzePhysics $file TA
      done
    fi
done
