#!/bin/bash
module load gcc/6.4.0 openmpi/2.1.1 extrae/3.5.2
source /mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/etc/extrae.sh
export EXTRAE_CONFIG_FILE=${HOME}/extraeCoursecodes/common/extraeMPI.xml
export LD_PRELOAD=/mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/lib/libmpitrace.so
srun --partition=thinnodes --time=00:15:00 -N 4 -n 16 ./prime_v0
mv prime_v0.prv ${STORE}/MPItraces/prime_v0.prv
mv prime_v0.pcf ${STORE}/MPItraces/prime_v0.pcf
mv prime_v0.row ${STORE}/MPItraces/prime_v0.row	
srun --partition=thinnodes --time=00:15:00 -N 4 -n 16 ./prime_v1
mv prime_v1.prv ${STORE}/MPItraces/prime_v1.prv
mv prime_v1.pcf ${STORE}/MPItraces/prime_v1.pcf
mv prime_v1.row ${STORE}/MPItraces/prime_v1.row
srun --partition=thinnodes --time=00:15:00 -N 4 -n 16 ./prime_v2
mv prime_v2.prv ${STORE}/MPItraces/prime_v2.prv
mv prime_v2.pcf ${STORE}/MPItraces/prime_v2.pcf
mv prime_v2.row ${STORE}/MPItraces/prime_v2.row



