all: loadmodules prime_v0 prime_v1 prime_v2
SHELL=/bin/bash

loadmodules:
	module load gcc/6.4.0
	module load openmpi
prime_v0:
	mpicc -lm -o prime_v0 prime_mpi_v0.c
prime_v1:
	mpicc -o prime_v1 prime_mpi_v1.c
prime_v2:
	mpicc -o prime_v2 prime_mpi_v2.c

clean:
	rm prime_v0 prime_v1 prime_v2

