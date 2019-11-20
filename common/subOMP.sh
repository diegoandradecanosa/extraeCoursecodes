#!/bin/bash
#module load gcc/6.4.0 openmpi/2.1.1 extrae/3.5.2
source /usr/local/etc/extrae.sh
#source /mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/etc/extrae.sh
export EXTRAE_CONFIG_FILE=/usr/local/share/example/OMP/extrae.xml
#export EXTRAE_CONFIG_FILE=/mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/share/example/OMP/extrae.xml
export LD_PRELOAD=/usr/local/lib/libomptrace.so
#export LD_PRELOAD=/mnt/netapp1/Optcesga_FT2/opt/cesga/easybuild-cesga/software/MPI/gcc/6.4.0/openmpi/2.1.1/extrae/3.5.2/lib/libomptrace.so
for var in "$@"
do
    ./$var
done


