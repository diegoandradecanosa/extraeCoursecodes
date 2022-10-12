all: poisson_v0 poisson_v1 loadmodules
SHELL=/bin/bash

loadmodules:
	module purge
	module load cesga/2020
	module --ignore-cache load "gcccore/system"
	module load gcc/system
	module load openmpi/4.1.4_ft3
poisson_v0:
	mpicc -g -o poisson_v0 poisson_mpi_v0.c
poisson_v1:
	mpicc -g -o poisson_v1 poisson_mpi_v1.c

clean:
	rm poisson_v0 poisson_v1

