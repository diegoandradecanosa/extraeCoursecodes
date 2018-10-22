all: bicg_v0 bicg_v1

FLAGS=  -DLARGE_DATASET

bicg_v0: 
	gcc $(FLAGS) -fopenmp -O2 -I../common bicg_v0.c ../common/polybench.c -o bicg_v0

bicg_v1:
	gcc $(FLAGS) -fopenmp -O2 -I../common bicg_v1.c ../common/polybench.c -o bicg_v1

clean:
	rm bicg_v0 bicg_v1
