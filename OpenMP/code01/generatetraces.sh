#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -c 24 --mem=32G ../../common/subOMP.sh correlation_v0 correlation_v1
