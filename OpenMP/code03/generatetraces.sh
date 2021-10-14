#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -p cola-corta  -c 24 ../../common/subOMPINV.sh bicg_v0 bicg_v1
