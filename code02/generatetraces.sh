#!/bin/bash
srun --time=00:05:00 --partition=cola-corta  -c 24 ../common/subOMP.sh trmm_v0 trmm_v1
