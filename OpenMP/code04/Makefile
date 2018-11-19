all: jacobi-2d-imper_v0 jacobi-2d-imper_v1 jacobi-2d-imper_v2

FLAGS= -DEXTRALARGE_DATASET -g 

jacobi-2d-imper_v0: 
	gcc $(FLAGS) -fopenmp -O2 -I../../common jacobi-2d-imper_v0.c ../../common/polybench.c -o jacobi-2d-imper_v0

jacobi-2d-imper_v1:
	gcc $(FLAGS) -fopenmp -O2 -I../../common jacobi-2d-imper_v1.c ../../common/polybench.c -o jacobi-2d-imper_v1

jacobi-2d-imper_v2:
	gcc $(FLAGS) -fopenmp -O2 -I../../common jacobi-2d-imper_v2.c ../../common/polybench.c -o jacobi-2d-imper_v2

clean:
	rm jacobi-2d-imper_v0 jacobi-2d-imper_v1 jacobi-2d-imper_v2
