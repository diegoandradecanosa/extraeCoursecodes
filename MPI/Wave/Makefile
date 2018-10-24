all: satisfy loadmodules
SHELL=/bin/bash

loadmodules:
	module load gcc/6.4.0
	module load openmpi
satisfy:
	mpicc -o satisfy satisfy_mpi.c

clean:
	rm satisfy

