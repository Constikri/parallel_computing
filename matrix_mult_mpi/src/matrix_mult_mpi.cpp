/*
 ============================================================================
 Name        : matrix_mult_mpi.c
 Author      : 
 Version     :
 Copyright   : Your copyright notice
 Description : Compute Pi in MPI C++
 ============================================================================
 */
#include <stdlib.h>
#include <unistd.h>
#include <math.h> 
#include "mpi.h" 
#include <iostream>
#include <fstream>
#include <random>
#include <string.h>
using namespace std;

bool file_exist(char *name) {
	return ( access( name, F_OK ) != -1 );
}

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
	file << endl;
	file.close();
}

void print_matrix_double(double **matrix, int x, int y, int flag_invert){
	for(int i = 0; i < x; i++){
		for(int j = 0; j < y; j++){
			if(flag_invert){
				cout << matrix[j][i] << " ";
			}else{
				cout << matrix[i][j] << " ";
			}

		}
		cout << endl;
	}
	cout << endl;
}

void print_matrix_int(int **matrix, int x, int y, int flag_invert){
	for(int i = 0; i < x; i++){
		for(int j = 0; j < y; j++){
			if(flag_invert){
				cout << matrix[j][i] << " ";
			}else{
				cout << matrix[i][j] << " ";
			}

		}
		cout << endl;
	}
	cout << endl;
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

void multiply_part(double **A, double **B, double **result, int x_a,int x_b, int y, int offset, int rank){
	for(int i = 0; i < x_a; i++){
		int of_run = offset;
		for(int j = 0; j < x_b; j++){
//			if(rank == 3 && of_run == 1){
//				cout << "prev res: " << result[i][of_run];
//			}
			for(int k = 0; k < y; k++){
				result[i][of_run] += A[i][k] * B[j][k];
			}
			if(rank == 3){
				cout << "res: " << result[i][of_run] << endl;
			}
			of_run++;
		}
	}
}

int main(int argc, char *argv[]) {

	MPI::Init(argc, argv);
	int size = MPI::COMM_WORLD.Get_size();
	int rank = MPI::COMM_WORLD.Get_rank();

	double **A, **B, **result, **recvA, **recvB;
	int **sendcnt, **displ, **recv_dimensions;
	int i;

	sendcnt = alloc_2d_array_int(2, size);
	displ = alloc_2d_array_int(2, size);
	recv_dimensions = alloc_2d_array_int(3, size);

	if(rank == 0){
		int xdim, ydim, xportion, yportion, xrest, yrest;
		cout << "Please put in the x dimension of the first matrix:" << endl;
		cin >> xdim;
		cout << "Please put in the y dimension of the first matrix:" << endl;
		cin >> ydim;

		A = alloc_2d_array_double(xdim, ydim);
		B = alloc_2d_array_double(xdim, ydim);
		result = alloc_2d_array_double(xdim, xdim);

		generate(A, xdim, ydim);
		generate(B, ydim, xdim);
		multiply(A, B, result, xdim, ydim);

		char filename[15] = {0};
		sprintf(filename, "A.txt");
		if( file_exist(filename) && (remove( filename ) != 0 )){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(A, filename, xdim, ydim, 0);
		sprintf(filename, "B.txt");
		if( file_exist(filename) && (remove( filename ) != 0 )){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(B, filename, ydim, xdim, 1);
		sprintf(filename, "result.txt");
		if( file_exist(filename) && (remove( filename ) != 0 )){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(result, filename, xdim, xdim, 0);

		xportion = xdim/(size);
		xrest = xdim%(size);
		yportion = ydim/(size);
		yrest = ydim%(size);

		recv_dimensions[2][0] = xdim;
		recv_dimensions[2][1] = ydim;

		int sumA = 0;
		int sumB = 0;
		for(i = 0; i < size; i++){
			sendcnt[0][i] = xportion*ydim;
			sendcnt[1][i] = yportion*xdim;
			recv_dimensions[0][i] = xportion;
			recv_dimensions[1][i] = yportion;
			if(xrest > 0){
				sendcnt[0][i] += ydim;
				recv_dimensions[0][i] += 1;
				xrest--;
			}
			if(yrest > 0){
				sendcnt[1][i] += xdim;
				recv_dimensions[1][i] += 1;
				yrest--;
			}
			displ[0][i] = sumA;
			sumA += sendcnt[0][i];
			displ[1][i] = sumB;
			sumB += sendcnt[1][i];
		}
	}

	MPI::COMM_WORLD.Bcast(&recv_dimensions[0][0], 3*size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&sendcnt[0][0], 2*size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&displ[0][0], 2*size, MPI_INT, 0);

	recvA = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[2][1]);
	recvB = alloc_2d_array_double(recv_dimensions[1][rank], recv_dimensions[2][1]);

	MPI::COMM_WORLD.Scatterv(&(A[0][0]), sendcnt[0], displ[0], MPI_DOUBLE, &(recvA[0][0]), sendcnt[0][rank], MPI_DOUBLE, 0);
	MPI::COMM_WORLD.Scatterv(&(B[0][0]), sendcnt[1], displ[1], MPI_DOUBLE, &(recvB[0][0]), sendcnt[1][rank], MPI_DOUBLE, 0);

	double **temp_result;
	temp_result = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[2][0]);
	i = rank;
	do{
		multiply_part(recvA, recvB, temp_result, recv_dimensions[0][rank], recv_dimensions[1][i], recv_dimensions[2][1], ((displ[1][i])/(recv_dimensions[2][1])), rank);
		if(rank == 0){
			MPI::COMM_WORLD.Send(&(recvB[0][0]), sendcnt[1][i], MPI_DOUBLE, rank+1, 0);
			free(recvB[0]);
			free(recvB);
			if(i == 0){
				i = size;
			}
			i--;
			recvB = alloc_2d_array_double(recv_dimensions[1][i], recv_dimensions[2][1]);
			MPI::COMM_WORLD.Recv(&(recvB[0][0]), sendcnt[1][i], MPI_DOUBLE, size-1, 0);
		}else {
			double **tempB_part;
			int temp_i;
			tempB_part = alloc_2d_array_double(recv_dimensions[1][i], recv_dimensions[2][1]);
			copy(&recvB[0][0], &recvB[0][0]+(recv_dimensions[1][i]*recv_dimensions[2][1]), &tempB_part[0][0]);
			temp_i = i;
			free(recvB[0]);
			free(recvB);
			if(i == 0){
				i = size;
			}
			i--;
			recvB = alloc_2d_array_double(recv_dimensions[1][i], recv_dimensions[2][1]);
			MPI::COMM_WORLD.Recv(&(recvB[0][0]), sendcnt[1][i], MPI_DOUBLE, rank-1, 0);
			if(rank == (size-1)){
				MPI::COMM_WORLD.Send(&(tempB_part[0][0]), sendcnt[1][temp_i], MPI_DOUBLE, 0, 0);
			}else{
				MPI::COMM_WORLD.Send(&(tempB_part[0][0]), sendcnt[1][temp_i], MPI_DOUBLE, rank+1, 0);
			}
			free(tempB_part[0]);
			free(tempB_part);
		}
	}while (i != rank);

	MPI::COMM_WORLD.Gatherv(&(temp_result[0][0]), sendcnt[0][rank], MPI_DOUBLE, &result[0][0], sendcnt[0], displ[0], MPI_DOUBLE, 0);

	if(rank == 0){
		char filename[15] = {0};
		sprintf(filename, "result.txt");
		print_matrix_to_file(result, filename, recv_dimensions[2][0], recv_dimensions[2][0], 0);
	}

	if(rank == 0){
		free(A[0]);
		free(A);
		free(B[0]);
		free(B);
		free(result[0]);
		free(result);
	}
	free(recvA[0]);
	free(recvA);
	free(recvB[0]);
	free(recvB);
	free(recv_dimensions[0]);
	free(recv_dimensions);
	free(sendcnt[0]);
	free(sendcnt);
	free(displ[0]);
	free(displ);
	free(temp_result[0]);
	free(temp_result);

	cout << "Process " << rank << " finished" << endl;

	MPI::Finalize();
	return 0;
}

