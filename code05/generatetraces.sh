#!/bin/bash
srun --time=00:05:00  --partition=cola-corta -c 24 ../common/subOMP.sh 2mm_v0 2mm_v1
