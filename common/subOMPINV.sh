#!/bin/bash
module load cesga/2018 gcccore/6.4.0 openmpi-runtime libunwind binutils
EXTRAE_HOME=${HOME}/installextrae
source ${EXTRAE_HOME}/etc/extrae.sh
export EXTRAE_CONFIG_FILE=${HOME}/extraeCoursecodes/common/extraeINV.xml
export LD_PRELOAD=${EXTRAE_HOME}/lib/libomptrace.so
#export LD_LIBRARY_PATH=/home/ulc/es/dac/papiinstall/lib:/usr/lib64
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${EXTRAE_HOME}/../installpapi/lib

for var in "$@"
do
    ./$var
done


