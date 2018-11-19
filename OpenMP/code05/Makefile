all: 2mm_v0 2mm_v1

FLAGS=  -DLARGE_DATASET -g 

2mm_v0: 
	gcc $(FLAGS) -fopenmp -O2 -I../../common 2mm_v0.c ../../common/polybench.c -o 2mm_v0

2mm_v1:
	gcc $(FLAGS) -fopenmp -O2 -I../../common 2mm_v1.c ../../common/polybench.c -o 2mm_v1

clean:
	rm 2mm_v0 2mm_v1
