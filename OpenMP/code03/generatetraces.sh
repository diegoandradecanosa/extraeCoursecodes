#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -c 24 --mem=32G ../../common/subOMPINV.sh bicg_v0 bicg_v1
