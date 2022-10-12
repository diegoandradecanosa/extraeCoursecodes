all: loadmodules prime_v0 prime_v1 prime_v2
SHELL=/bin/bash

loadmodules:
	module purge
	module load cesga/2020
	module --ignore-cache load "gcccore/system"
	module load gcc/system
	module load openmpi/4.1.4_ft3
prime_v0:
	mpicc -g -lm -o prime_v0 prime_mpi_v0.c
prime_v1:
	mpicc -g -o prime_v1 prime_mpi_v1.c
prime_v2:
	mpicc -g -o prime_v2 prime_mpi_v2.c

clean:
	rm prime_v0 prime_v1 prime_v2

