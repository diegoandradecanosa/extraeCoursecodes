all: poisson loadmodules
SHELL=/bin/bash

loadmodules:
	module load gcc/6.4.0
	module load openmpi
poisson:
	mpicc -o poisson poisson_mpi.c

clean:
	rm poisson

