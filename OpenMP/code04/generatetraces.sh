#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -c 24 --mem=32G ../../common/subOMP.sh jacobi-2d-imper_v0 jacobi-2d-imper_v1 jacobi-2d-imper_v2
