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
#include <fstream>
#include <random>
#include <string.h>
using namespace std;

double **alloc_2d_array_double(int rows, int cols) {
	double *data = (double *)malloc(rows*cols*sizeof(double));
	double **array= (double **)malloc(rows*sizeof(double*));
	for (int i=0; i<rows; i++) {
		array[i] = &(data[cols*i]);
	}
	return array;
}

int **alloc_2d_array_int(int rows, int cols) {
	int *data = (int *)malloc(rows*cols*sizeof(int));
	int **array= (int **)malloc(rows*sizeof(int*));
	for (int i=0; i<rows; i++) {
		array[i] = &(data[cols*i]);
	}
	return array;
}

void print_matrix_to_file(double **matrix, char *filename, int x, int y, int flag_invert){
	ofstream file;
	file.open(filename, ios::out | ios::app);
	for(int i = 0; i < x; i++){
		for(int j = 0; j < y; j++){
			if(flag_invert){
				file << matrix[j][i] << " ";
			}else{
				file << matrix[i][j] << " ";
			}

		}
		file << endl;
	}
	file.close();
}

void generate(double **matrix, int x, int y){
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<double> dist(0.0, 10.0);
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
				result[i][j] += A[i][k] * B[j][k];
			}
		}
	}
}

void multiply_part(double **A, double **B, double **result, int x, int y, int offset){
	for(int i = 0; i < x; i++){
		for(int j = 0; j < x; j++){
			for(int k = 0; k < y; k++){
				result[i][j+offset] += A[i][k] * B[j][k];
			}
		}
	}
}

int main(int argc, char *argv[]) {

	MPI::Init(argc, argv);
	int size = MPI::COMM_WORLD.Get_size();
	int rank = MPI::COMM_WORLD.Get_rank();

	double **A, **B, **result, **recvA, **recvB;
	int **sendcnt, **displ;
	int *recv_dimensions;
	int i;

	sendcnt = alloc_2d_array_int(2, size);
	displ = alloc_2d_array_int(2, size);
	recv_dimensions = (int *)malloc(sizeof(int)*4);

	if(rank == 0){
		int xdim, ydim, xportion, yportion, xrest, yrest
		cout << "Please put in the x dimension of the first matrix:" << endl;
		cin >> xdim;
		cout << "Please put in the y dimension of the first matrix:" << endl;
		cin >> ydim;

		A = alloc_2d_array_double(xdim, ydim);
		B = alloc_2d_array_double(xdim, ydim);

		generate(A, xdim, ydim);
		generate(B, ydim, xdim);

		char filename[15] = {0};
		sprintf(filename, "A.txt");
		if( remove( filename ) != 0 ){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(A, filename, xdim, ydim, 0);
		sprintf(filename, "B.txt");
		if( remove( filename ) != 0 ){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(B, filename, ydim, xdim, 1);
		/*
		sprintf(filename, "result.txt");
		if( remove( filename ) != 0 ){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(result, filename, xdim, xdim);
		*/

		xportion = xdim/(size);
		xrest = xdim%(size);
		yportion = ydim/(size);
		yrest = ydim%(size);

		recv_dimensions[0] = xportion+1;
		recv_dimensions[1] = yportion+1;
		recv_dimensions[2] = xdim;
		recv_dimensions[3] = ydim;

		int sumA = 0;
		int sumB = 0;
		for(i = 0; i < size; i++){
			sendcnt[0][i] = xportion;
			sendcnt[1][i] = yportion;
			if(xrest > 0){
				sendcnt[0][i]++;
				xrest--;
			}
			if(yrest > 0){
				sendcnt[1][i]++;
				yrest--;
			}
			displ[0][i] = sumA;
			sumA += sendcnt[0][i];
			displ[1][i] = sumB;
			sumB += sendcnt[1][i];
		}
	}

	MPI::COMM_WORLD.Bcast(&recv_dimensions[0], 4, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&sendcnt[0][0], 2*size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&displ[0][0], 2*size, MPI_INT, 0);

	recvA = alloc_2d_array_double(sendcnt[0][rank], recv_dimensions[3]);
	recvB = alloc_2d_array_double(sendcnt[1][rank], recv_dimensions[3]);

	MPI::COMM_WORLD.Scatterv(&A[0][0], sendcnt[0], displ[0], MPI_DOUBLE, &recvA[0][0], recv_dimensions[0]*recv_dimensions[3], MPI_DOUBLE, 0);
	MPI::COMM_WORLD.Scatterv(&B[0][0], sendcnt[1], displ[1], MPI_DOUBLE, &recvB[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, 0);

	double **temp_result, **tempB_part;
	temp_result = alloc_2d_array_double(sendcnt[0][rank], recv_dimensions[2]);
	tempB_part = alloc_2d_array_double(sendcnt[1][rank], recv_dimensions[3]);
	i = rank;
	do{
		multiply_part(recvA, recvB, temp_result, sendcnt[0][rank], recv_dimensions[3], displ[1][i]);
		if(rank == 0){
			MPI::COMM_WORLD.Send(&recvB[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, rank+1, 0);
			MPI::COMM_WORLD.Recv(&recvB[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, size-1, 0);
		}else if(rank == (size-1)){
			memcpy(tempB_part, recvB, (recv_dimensions[1]*sizeof(double))*(recv_dimensions[3]*sizeof(double)));
			MPI::COMM_WORLD.Recv(&recvB[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, rank-1, 0);
			MPI::COMM_WORLD.Send(&tempB_part[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, 0, 0);
		}else{
			memcpy(tempB_part, recvB, (recv_dimensions[1]*sizeof(double))*(recv_dimensions[3]*sizeof(double)));
			MPI::COMM_WORLD.Recv(&recvB[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, rank-1, 0);
			MPI::COMM_WORLD.Send(&tempB_part[0][0], recv_dimensions[1]*recv_dimensions[3], MPI_DOUBLE, rank+1, 0);
		}
		i++;
		if(i == size){
			i = 0;
		}
	}while (i != rank);

	MPI::Finalize();
	return 0;
}

