#!/bin/bash
srun --time=00:05:00  --partition=thinnodes -c 24 ../common/subOMPINV.sh bicg_v0 bicg_v1
