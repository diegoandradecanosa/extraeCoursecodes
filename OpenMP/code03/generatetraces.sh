#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 --reservation=PROFILING_${day}Nov -p shared --qos=shared -c 24 ../common/subOMPINV.sh bicg_v0 bicg_v1
