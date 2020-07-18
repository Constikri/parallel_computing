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

int main(int argc, char *argv[]) {

	MPI::Init(argc, argv);
	int size = MPI::COMM_WORLD.Get_size();
	int rank = MPI::COMM_WORLD.Get_rank();

	double **A, **B, **result, **recvA, **recvB;
	int **recv_dimensions;
	int *sendcnt, *displ;
	int counter;

	sendcnt = (int *)malloc(size*sizeof(int));
	displ = (int *)malloc(size*sizeof(int));
	recv_dimensions = alloc_2d_array_int(2, size);

	if(rank == 0){
		int xdim, ydim, xportion, xrest;
		cout << "Please put in the x dimension of the first matrix:" << endl;
		cin >> xdim;
		cout << "Please put in the y dimension of the first matrix:" << endl;
		cin >> ydim;

		A = alloc_2d_array_double(xdim, ydim);
		B = alloc_2d_array_double(xdim, ydim);
		result = alloc_2d_array_double(xdim, xdim);

		generate(A, xdim, ydim);
		generate(B, xdim, ydim);
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

		free(result[0]);
		free(result);

		xportion = xdim/(size);
		xrest = xdim%(size);

		recv_dimensions[1][0] = xdim;
		recv_dimensions[1][1] = ydim;

		int sum = 0;
		for(counter = 0; counter < size; counter++){
			sendcnt[counter] = xportion*ydim;
			recv_dimensions[0][counter] = xportion;
			if(xrest > 0){
				sendcnt[counter] += ydim;
				recv_dimensions[counter] += 1;
				xrest--;
			}
			displ[counter] = sum;
			sum += sendcnt[counter];
		}
	}

	MPI::COMM_WORLD.Bcast(&recv_dimensions[0][0], 2*size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&sendcnt[0], size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&displ[0], size, MPI_INT, 0);

//	if(rank == 3){
//		cout << "dimensions" << endl;
//		print_matrix_int(recv_dimensions, 2, size, 0);
//		cout << "sendcnt" << endl;
//		cout << sendcnt[0] << " " << sendcnt[1] << " " << sendcnt[2] << " " << sendcnt[3] << " " << endl;
//		cout << "displ" << endl;
//		cout << displ[0] << " " << displ[1] << " " << displ[2] << " " << displ[3] << " " << endl;
//	}

	recvA = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[1][1]);
	recvB = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[1][1]);

	MPI::COMM_WORLD.Scatterv(&(A[0][0]), sendcnt, displ, MPI_DOUBLE, &(recvA[0][0]), sendcnt[rank], MPI_DOUBLE, 0);
	MPI::COMM_WORLD.Scatterv(&(B[0][0]), sendcnt, displ, MPI_DOUBLE, &(recvB[0][0]), sendcnt[rank], MPI_DOUBLE, 0);

//	if(rank == 3){
//		cout << "recvA" << endl;
//		print_matrix_double(recvA,recv_dimensions[0][rank], recv_dimensions[1][1], 0);
//		cout << "recvB" << endl;
//		print_matrix_double(recvB,recv_dimensions[0][rank], recv_dimensions[1][1], 0);
//	}
	double **temp_result;
	temp_result = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[1][0]);
	counter = rank;
	do{
		for(int i = 0; i < recv_dimensions[0][rank]; i++){
			int offset = ((displ[counter])/(recv_dimensions[1][1]));
			for(int j = 0; j < recv_dimensions[0][counter]; j++){
				temp_result[i][offset] = 0;
				for(int k = 0; k < recv_dimensions[1][1]; k++){
					temp_result[i][j+offset] += recvA[i][k] * recvB[j][k];
				}
				//offset++;
			}
		}
		if(rank == 0){
			MPI::COMM_WORLD.Send(&(recvB[0][0]), sendcnt[counter], MPI_DOUBLE, rank+1, 0);
			free(recvB[0]);
			free(recvB);
			if(counter == 0){
				counter = size;
			}
			counter--;
			recvB = alloc_2d_array_double(recv_dimensions[0][counter], recv_dimensions[1][1]);
			MPI::COMM_WORLD.Recv(&(recvB[0][0]), sendcnt[counter], MPI_DOUBLE, size-1, 0);
		}else {
			double **tempB_part;
			int temp_counter;
			tempB_part = alloc_2d_array_double(recv_dimensions[0][counter], recv_dimensions[1][1]);
			copy(&recvB[0][0], &recvB[0][0]+(recv_dimensions[0][counter]*recv_dimensions[1][1]), &tempB_part[0][0]);
			temp_counter = counter;
			free(recvB[0]);
			free(recvB);
			if(counter == 0){
				counter = size;
			}
			counter--;
			recvB = alloc_2d_array_double(recv_dimensions[0][counter], recv_dimensions[1][1]);
			MPI::COMM_WORLD.Recv(&(recvB[0][0]), sendcnt[counter], MPI_DOUBLE, rank-1, 0);
			if(rank == (size-1)){
				MPI::COMM_WORLD.Send(&(tempB_part[0][0]), sendcnt[temp_counter], MPI_DOUBLE, 0, 0);
			}else{
				MPI::COMM_WORLD.Send(&(tempB_part[0][0]), sendcnt[temp_counter], MPI_DOUBLE, rank+1, 0);
			}
			free(tempB_part[0]);
			free(tempB_part);
		}
		if(rank == 1){
			cout << "counter: " << counter << endl;
			print_matrix_double(recvB,recv_dimensions[0][rank], recv_dimensions[1][1], 0);
		}
	}while (counter != rank);

	if(rank == 0){
		result = alloc_2d_array_double(recv_dimensions[1][0], recv_dimensions[1][0]);
	}
	MPI::COMM_WORLD.Gatherv(&(temp_result[0][0]), sendcnt[rank], MPI_DOUBLE, &result[0][0], sendcnt, displ, MPI_DOUBLE, 0);

	if(rank == 0){
		char filename[15] = {0};
		sprintf(filename, "result.txt");
		print_matrix_to_file(result, filename, recv_dimensions[1][0], recv_dimensions[1][0], 0);
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
	free(sendcnt);
	free(displ);
	free(temp_result[0]);
	free(temp_result);

	//cout << "Process " << rank << " finished" << endl;

	MPI::Finalize();
	return 0;
}

