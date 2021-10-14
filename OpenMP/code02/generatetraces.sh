#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -p cola-corta -c 24 ../../common/subOMP.sh trmm_v0 trmm_v1
