#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 --reservation=PROFILING_${day}Nov -p cola-corta -c 24 ../../common/subOMP.sh correlation_v0 correlation_v1
