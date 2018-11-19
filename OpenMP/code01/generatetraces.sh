#!/bin/bash
srun --time=00:10:00 --partition=thinnodes  --reservation=PROFILING_20Nov -p shared --qos=shared -c 24 ../../common/subOMP.sh correlation_v0 correlation_v1
