#!/bin/bash

for i in `ls histos*Auger*.root` ; do
  ./plotFastEvents.py $i 0
done
