#!/bin/bash
srun --time=00:05:00 --partition=thinnodes  -c 24 ../../common/subOMP.sh trmm_v0 trmm_v1
