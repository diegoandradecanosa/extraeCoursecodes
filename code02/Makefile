all: trmm_v0 trmm_v1

FLAGS= 

trmm_v0: 
	gcc $(FLAGS) -fopenmp -O2 -I../common trmm_v0.c ../common/polybench.c -o trmm_v0

trmm_v1:
	gcc $(FLAGS) -fopenmp -O2 -I../common trmm_v1.c ../common/polybench.c -o trmm_v1

clean:
	rm trmm_v0 trmm_v1
