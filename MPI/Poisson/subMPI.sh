#!/bin/bash
module load gcc/6.4.0 openmpi/2.1.1 extrae/3.5.2
source /mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/etc/extrae.sh
export EXTRAE_CONFIG_FILE=${HOME}/extraeCoursecodes/common/extraeMPIOMP.xml
export LD_PRELOAD=/mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/lib/libompitrace.so
srun --partition=cola-corta --time=00:15:00 -N 4 -n 16 ./poisson
mv poisson.prv ${STORE}/MPItraces/poisson.prv
mv poisson.pcf ${STORE}/MPItraces/poisson.pcf
mv poisson.row ${STORE}/MPItraces/poisson.row	


