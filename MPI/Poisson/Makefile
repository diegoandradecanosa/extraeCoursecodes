all: poisson_v0 poisson_v1 loadmodules
SHELL=/bin/bash

loadmodules:
	module load gcc/6.4.0
	module load openmpi
poisson_v0:
	mpicc -o poisson_v0 poisson_mpi_v0.c
poisson_v1:
	mpicc -o poisson_v1 poisson_mpi_v1.c

clean:
	rm poisson_v0 poisson_v1

