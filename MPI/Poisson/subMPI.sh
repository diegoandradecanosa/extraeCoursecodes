#!/bin/bash
module load gcc/6.4.0 openmpi/2.1.1 extrae/3.5.2
source /mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/etc/extrae.sh
export EXTRAE_CONFIG_FILE=${HOME}/extraeCoursecodes/common/extraeMPI.xml
export LD_PRELOAD=/mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/lib/libmpitrace.so
srun --partition=cola-corta --time=00:15:00 -N 4 -n 16 ./poisson_v0 
mv poisson_v0.prv ${STORE}/MPItraces/poisson_v0.prv
mv poisson_v0.pcf ${STORE}/MPItraces/poisson_v0.pcf
mv poisson_v0.row ${STORE}/MPItraces/poisson_v0.row	
srun --partition=cola-corta --time=00:15:00 -N 4 -n 16 ./poisson_v1
mv poisson_v1.prv ${STORE}/MPItraces/poisson_v1.prv
mv poisson_v1.pcf ${STORE}/MPItraces/poisson_v1.pcf
mv poisson_v1.row ${STORE}/MPItraces/poisson_v1.row


