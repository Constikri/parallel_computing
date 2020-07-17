/*
 ============================================================================
 Name        : matrix_mult_openMP.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Hello OpenMP World in C
 ============================================================================
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

double drand ( double low, double high )
{
    return ( (double)rand() * ( high - low ) ) / (double)RAND_MAX + low;
}

double **alloc_2d_array_double(int rows, int cols) {
	double *data = (double *)malloc(rows*cols*sizeof(double));
	double **array= (double **)malloc(rows*sizeof(double*));
	for (int i=0; i<rows; i++) {
		array[i] = &(data[cols*i]);
	}
	return array;
}

int main (int argc, char *argv[]) {

	int numThreads, tid, xdim, ydim, i, j, k;
	double **A, **B, **result;
	printf("Please put in the x dimension of the first matrix: ");
	scanf("%d", &xdim);
	printf("Please put in the y dimension of the first matrix: ");
	scanf("%d", &ydim);

	A = alloc_2d_array_double(xdim, ydim);
	B = alloc_2d_array_double(ydim, xdim);
	result = alloc_2d_array_double(xdim, xdim);

	FILE *fp_a, *fp_b;
	fp_a = fopen ("A.txt", "w+");
	fp_b = fopen ("B.txt", "w+");
	for(i = 0; i < xdim; i++){
		for(j = 0; j < ydim; j++){
			A[i][j] = drand(0, 10);
			B[i][j] = drand(0, 10);
			fprintf(fp_a, "%f ", A[i][j]);
			fprintf(fp_b, "%f ", B[i][j]);
		}
		fprintf(fp_a, "\n");
		fprintf(fp_b, "\n");
	}
	fclose(fp_a);
	fclose(fp_b);

#pragma omp parallel private(numThreads, tid, i, j, k) default(shared)
{
	int xportion, xrest, offset;
	tid = omp_get_thread_num();
	numThreads = omp_get_num_threads();

	xportion = xdim/(numThreads);
	// rest verteilen
	xrest = xdim%(numThreads);
	offset = xportion*tid;

	for(i = offset; i < (offset+xportion); i++){
		for(j = 0; j < xdim; j++){
			for(k = 0; k < ydim; k++){
				result[i][j] += A[i][k] * B[k][j];
			}
		}
	}
	printf("Thread: %d i: %d j: %d k: %d\n", tid,i,j,k);

}
	FILE *fp_res;
	fp_res = fopen("result.txt", "w+");
	for(i = 0; i < xdim; i++){
		for(j = 0; j < xdim; j++){
			fprintf(fp_res, "%f ", result[i][j]);
		}
		fprintf(fp_res, "\n");
	}
	fclose(fp_res);

	free(A[0]);
	free(A);
	free(B[0]);
	free(B);
	free(result[0]);
	free(result);
 return 0;
}


