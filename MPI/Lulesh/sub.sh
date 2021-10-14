srun --time=00:15:00 -p cola-corta -N 4 -n 8 -c 12 ./subMPI.sh

mv luleshMPI_OMP.prv ${STORE}/MPItraces/luleshMPI_OMP.prv
mv luleshMPI_OMP.pcf ${STORE}/MPItraces/luleshMPI_OMP.pcf
mv luleshMPI_OMP.row ${STORE}/MPItraces/luleshMPI_OMP.row



