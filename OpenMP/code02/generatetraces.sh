#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -c 24 --mem=32G ../../common/subOMP.sh trmm_v0 trmm_v1
