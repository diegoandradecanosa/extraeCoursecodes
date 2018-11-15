#!/bin/bash
srun --time=01:00:00  --partition=thinnodes -c 24 ../../common/subOMP.sh jacobi-2d-imper_v0 jacobi-2d-imper_v1 jacobi-2d-imper_v2
