#!/bin/bash
module load gcc/6.4.0 openmpi/2.1.1 extrae/3.5.2
source /mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/etc/extrae.sh
export EXTRAE_CONFIG_FILE=${HOME}/extraeCoursecodes/common/extraeMPIOMP.xml
export LD_PRELOAD=/mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/lib/libompitrace.so
day=`date '+%d'`
srun --time=00:15:00 --reservation=PROFILING_${day}Nov -p shared --qos=shared -N 4 -n 8 -c 12 ./luleshMPI_OMP
mv luleshMPI_OMP.prv ${STORE}/MPItraces/luleshMPI_OMP.prv
mv luleshMPI_OMP.pcf ${STORE}/MPItraces/luleshMPI_OMP.pcf
mv luleshMPI_OMP.row ${STORE}/MPItraces/luleshMPI_OMP.row



