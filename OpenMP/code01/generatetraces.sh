#!/bin/bash
srun --time=00:10:00 --partition=cola-corta  -c 24 ../../common/subOMP.sh correlation_v0 correlation_v1
