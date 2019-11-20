all: correlation_v0 correlation_v1

FLAGS= -lm -g 

correlation_v0: 
	gcc -fopenmp -O2 -I../../common correlation_v0.c ../../common/polybench.c -o correlation_v0 $(FLAGS)

correlation_v1:
	gcc -fopenmp -O2 -I../../common correlation_v1.c ../../common/polybench.c -o correlation_v1 $(FLAGS)

clean:
	rm correlation_v0 correlation_v1
