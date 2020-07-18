/*
 ============================================================================
 Name        : matrix_mult_openMP.c
 Author      : ck
 Version     : 1.0
 Copyright   :
 Description : parallel multiplication of two random matrices
 ============================================================================
 */
#include <omp.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

// generate random double between low and high
double drand ( double low, double high )
{
    return ( (double)rand() * ( high - low ) ) / (double)RAND_MAX + low;
}

//allocate memory to a two dimensional pointer
double **alloc_2d_array_double(int rows, int cols) {
	double *data = (double *)malloc(rows*cols*sizeof(double));
	double **array= (double **)malloc(rows*sizeof(double*));
	for (int i=0; i<rows; i++) {
		array[i] = &(data[cols*i]);
	}
	return array;
}

//print a matrix to a file
void print_matrix(double **matrix, char *filename, int rows, int cols){
	FILE *fp;
	fp = fopen (filename, "w+");
	for(int i = 0; i < rows; i++){
		for(int j = 0; j < cols; j++){
			fprintf(fp, "%f ", matrix[i][j]);
		}
		fprintf(fp, "\n");
	}
	fclose(fp);
}

int main (int argc, char *argv[]) {

	//initialize all important variables
	int numThreads, tid, xdim, ydim, i, j, k;
	double **A, **B, **result;

	//request the dimensions of the matrices from the user
	printf("Please put in the x dimension matrices: ");
	scanf("%d", &xdim);
	printf("Please put in the y dimension matrices: ");
	scanf("%d", &ydim);

	//allocate memory to the matrix pointers
	A = alloc_2d_array_double(xdim, ydim);
	B = alloc_2d_array_double(ydim, xdim);
	result = alloc_2d_array_double(xdim, xdim);

	//initialize matrices with random values
	for(i = 0; i < xdim; i++){
		for(j = 0; j < ydim; j++){
			A[i][j] = drand(0.0, 10.0);
			B[j][i] = drand(0.0, 10.0);
		}
	}

//start multiple threads for computation
#pragma omp parallel private(numThreads, tid, i, j, k) default(shared)
{
	int xportion, xrest, offset;

	//get thread number and total number of threads
	tid = omp_get_thread_num();
	numThreads = omp_get_num_threads();

	//calculate the portion of the computation, the thread will make
	//and the offset at which the thread will start
	xportion = xdim/(numThreads);
	xrest = xdim%(numThreads);
	offset = 0;
	//if the workload is not evenly dividable for all threads, spread the rest and adjust the offset
	if(xrest > tid){
		xportion++;
	}else{
		offset += xrest;
	}
	offset += xportion*tid;

	//compute part_of_matrix_A * matrix_B
	for(i = offset; i < (offset+xportion); i++){
		for(j = 0; j < xdim; j++){
			for(k = 0; k < ydim; k++){
				result[i][j] += A[i][k] * B[k][j];
			}
		}
	}
}
	//print all matrices to their respective files
	char filename[20] = {0};
	sprintf(filename, "A.txt");
	print_matrix(A, filename, xdim, ydim);
	sprintf(filename, "B.txt");
	print_matrix(B, filename, ydim, xdim);
	sprintf(filename, "result.txt");
	print_matrix(result, filename, xdim, xdim);

	//free the memory blocked by the matrices
	free(A[0]);
	free(A);
	free(B[0]);
	free(B);
	free(result[0]);
	free(result);

	printf("Program Finished\n");
	printf("Look up files A.txt and B.txt to see the matrixes used, and look up result.txt to see the result");

 return 0;
}


