/*
 ============================================================================
 Name        : matrix_mult_mpi.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Compute Pi in MPI C++
 ============================================================================
 */
#include <math.h> 
#include "mpi.h" 
#include <iostream>
#include <random>
using namespace std;

double **alloc_2d_init(int rows, int cols) {
	double *data = (double *)malloc(rows*cols*sizeof(double));
	double **array= (double **)malloc(rows*sizeof(double*));
	for (int i=0; i<rows; i++)
		array[i] = &(data[cols*i]);

	return array;
}

void generate(double **matrix, int x, int y){
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<double> dist(0.0, 1000.0);
	for(int i = 0; i < x; i++){
		for(int j = 0; j < y; j++){
			matrix[i][j] = dist(gen);
		}
	}
}

void multiply(double **A, double **B, double **result, int x, int y){
	for(int i = 0; i < x; i++){
		for(int j = 0; j < x; j++){
			for(int k = 0; k < y; k++){
				result[i][j] += A[i][k] * B[k][j];
			}
		}
	}
}

int main(int argc, char *argv[]) {

	MPI::Init(argc, argv);
	int size = MPI::COMM_WORLD.Get_size();
	int rank = MPI::COMM_WORLD.Get_rank();

	if(rank == 0){
		int xdim, ydim, xportion, yportion, xrest, yrest;
		cout << "Please put in the x dimension of the first matrix:" << endl;
		cin >> xdim;
		cout << "Please put in the y dimension of the first matrix:" << endl;
		cin >> xdim;

		double **A, **B, **result;
		A = alloc_2d_init(xdim, ydim);
		B = alloc_2d_init(ydim, xdim);
		result = alloc_2d_init(xdim, xdim);

		xportion = xdim/(size-1);
		xrest = xdim%(size-1);
		yportion = ydim/(size-1);
		yrest = ydim%(size-1);
	}

	MPI::Finalize();
	return 0;
}

