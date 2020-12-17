srun --time=00:15:00 -p cola-corta -N 4 -n 8 -c 12 ./subMPI.sh
mv prime_v0.prv ${STORE}/MPItraces/prime_v0.prv
mv prime_v0.pcf ${STORE}/MPItraces/prime_v0.pcf
mv prime_v0.row ${STORE}/MPItraces/prime_v0.row
mv prime_v1.prv ${STORE}/MPItraces/prime_v1.prv
mv prime_v1.pcf ${STORE}/MPItraces/prime_v1.pcf
mv prime_v1.row ${STORE}/MPItraces/prime_v1.row
mv prime_v2.prv ${STORE}/MPItraces/prime_v2.prv
mv prime_v2.pcf ${STORE}/MPItraces/prime_v2.pcf
mv prime_v2.row ${STORE}/MPItraces/prime_v2.row



