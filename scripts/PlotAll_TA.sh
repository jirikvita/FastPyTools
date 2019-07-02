#!/bin/bash

for i in `ls histos*TA*.root` ; do
  ./plotFastEvents.py $i 0
done
