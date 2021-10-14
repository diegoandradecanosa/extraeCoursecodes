#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00  -p cola-corta   -c 24 ../../common/subOMP.sh 2mm_v0 2mm_v1
