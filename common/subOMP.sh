#!/bin/bash
#module load gcc/6.4.0 openmpi/2.1.1 extrae/3.5.2
module load gcc/6.4.0 
module load gcccore/6.4.0  
module load openmpi 
module load libunwind/1.2.1 
module load libxml2/2.9.7 
module load glibc/2.28
EXTRAE_HOME=${HOME}/extrae/extraeinstall
source ${EXTRAE_HOME}/etc/extrae.sh
export EXTRAE_CONFIG_FILE=${EXTRAE_HOME}/share/example/OMP/extrae.xml
export LD_PRELOAD=${EXTRAE_HOME}/lib/libomptrace.so
#export LD_LIBRARY_PATH=/home/ulc/es/dac/papiinstall/lib:/usr/lib64
export LD_LIBRARY_PATH=${LD_LIBRARY_PATH}:${EXTRAE_HOME}/../papiinstall/lib
for var in "$@"
do
    ./$var
done


