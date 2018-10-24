#!/bin/bash
srun --time=00:05:00  --partition=cola-corta -c 24 ../common/subOMPINV.sh bicg_v0 bicg_v1
