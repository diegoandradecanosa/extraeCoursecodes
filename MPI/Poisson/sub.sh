srun --time=00:15:00 -p cola-corta -N 4 -n 8 -c 12 ./subMPI.sh

srun --time=00:15:00 --reservation=PROFILING_${day}Nov -p shared --qos=shared 4 -n 16 ./poisson_v0
mv poisson_v0.prv ${STORE}/MPItraces/poisson_v0.prv
mv poisson_v0.pcf ${STORE}/MPItraces/poisson_v0.pcf
mv poisson_v0.row ${STORE}/MPItraces/poisson_v0.row
day=`date '+%d'`
srun --time=00:15:00 --reservation=PROFILING_${day}Nov -p shared --qos=shared -N 4 -n 16 ./poisson_v1
mv poisson_v1.prv ${STORE}/MPItraces/poisson_v1.prv
mv poisson_v1.pcf ${STORE}/MPItraces/poisson_v1.pcf
mv poisson_v1.row ${STORE}/MPItraces/poisson_v1.row



