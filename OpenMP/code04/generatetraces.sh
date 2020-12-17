#!/bin/bash
day=`date '+%d'`
srun --time=00:10:00 -p cola-corta -c 24 ../../common/subOMP.sh jacobi-2d-imper_v0 jacobi-2d-imper_v1 jacobi-2d-imper_v2
