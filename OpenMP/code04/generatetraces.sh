#!/bin/bash
srun --time=00:10:00 --partition=thinnodes  --reservation=PROFILING_20Nov -p shared --qos=shared -c 24 ../../common/subOMP.sh jacobi-2d-imper_v0 jacobi-2d-imper_v1 jacobi-2d-imper_v2
