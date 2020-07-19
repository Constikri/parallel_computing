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

//check if a file exists
bool file_exist(char *name) {
	return ( access( name, F_OK ) != -1 );
}

// allocate memory to a two dimensional double pointer
double **alloc_2d_array_double(int rows, int cols) {
	//reserve space for all the data
	double *data = (double *)malloc(rows*cols*sizeof(double));
	//reserve space for the actual array
	double **array= (double **)malloc(rows*sizeof(double*));
	//assign the data memory to the array
	for (int i=0; i<rows; i++) {
		array[i] = &(data[cols*i]);
	}
	return array;
}

// allocate memory to a two dimensional int pointer
int **alloc_2d_array_int(int rows, int cols) {
	//reserve space for all the data
	int *data = (int *)malloc(rows*cols*sizeof(int));
	//reserve space for the actual array
	int **array= (int **)malloc(rows*sizeof(int*));
	//assign the data memory to the array
	for (int i=0; i<rows; i++) {
		array[i] = &(data[cols*i]);
	}
	return array;
}

//print a double matrix to a file
void print_matrix_to_file(double **matrix, char *filename, int x, int y, int flag_invert){
	//open file stream
	ofstream file;
	file.open(filename, ios::out | ios::app);
	//iterate through matrix and print the content to file
	for(int i = 0; i < x; i++){
		for(int j = 0; j < y; j++){
			if(flag_invert){ // prints the rows as columns and the columns as rows
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

//utilities used for debugging
//work as print_matrix_to_file() but prints to cout instead
//void print_matrix_double(double **matrix, int x, int y, int flag_invert){
//	for(int i = 0; i < x; i++){
//		for(int j = 0; j < y; j++){
//			if(flag_invert){
//				cout << matrix[j][i] << " ";
//			}else{
//				cout << matrix[i][j] << " ";
//			}
//
//		}
//		cout << endl;
//	}
//	cout << endl;
//}
//
//void print_matrix_int(int **matrix, int x, int y, int flag_invert){
//	for(int i = 0; i < x; i++){
//		for(int j = 0; j < y; j++){
//			if(flag_invert){
//				cout << matrix[j][i] << " ";
//			}else{
//				cout << matrix[i][j] << " ";
//			}
//
//		}
//		cout << endl;
//	}
//	cout << endl;
//}

//generate random double values and fill a  matrix with them
void generate(double **matrix, int x, int y){
	//initialise random generator
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<double> dist(0.0, 10.0);
	//iterate through matrix and assign random values
	for(int i = 0; i < x; i++){
		for(int j = 0; j < y; j++){
			matrix[i][j] = dist(gen);
		}
	}
}

// multiply two matrices and save the result to another matrix
// the second matrix needs to have rows and columns swapped
void multiply(double **A, double **B, double **result, int x, int y){
	//iterate through the rows of the first matrix
	for(int i = 0; i < x; i++){
		//iterate through the columns of the second matrix
		for(int j = 0; j < x; j++){
			//iterate through the columns of the first matrix and the rows of the second matrix
			for(int k = 0; k < y; k++){
				//perform the multiplication
				result[i][j] += A[i][k] * B[j][k];
			}
		}
	}
}

int main(int argc, char *argv[]) {

	//initialize mpi
	MPI::Init(argc, argv);
	int size = MPI::COMM_WORLD.Get_size();
	int rank = MPI::COMM_WORLD.Get_rank();

	//initialize important variables and arrays
	double **A, **B, **result, **recvA, **recvB;
	int **recv_dimensions, **sendcnt, **displ;
	int counter;

	//allocate memory to utility int arrays
	sendcnt = alloc_2d_array_int(2, size);
	displ = alloc_2d_array_int(2, size);
	recv_dimensions = alloc_2d_array_int(2, size);

	//do all the preparation for the calculation on the root process
	if(rank == 0){
		//initialize local variables
		int xdim, ydim, portion, rest;
		//request matrix dimensions from the user
		cout << "Please put in the x dimension of the first matrix:" << endl;
		cin >> xdim;
		cout << "Please put in the y dimension of the first matrix:" << endl;
		cin >> ydim;

		//allocate memory the two operand matrices
		A = alloc_2d_array_double(xdim, ydim);
		//the x dimension from matrix A is the y dimension from matrix B and
		//the x dimension from matrix A is the x dimension from matrix B but
		//but they are swapped to have no complications with MPI_Scatterv later on
		//this is taken into account when calculating the matrices
		B = alloc_2d_array_double(xdim, ydim);

		//fill the matrices with random numbers
		generate(A, xdim, ydim);
		generate(B, xdim, ydim);

		//print matrices A and B to a File
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

		//if only one process is running the program, skip the parallel code
		if(size == 1){
			//allocate memory for the result array
			result = alloc_2d_array_double(xdim, xdim);
			//multiply the matrices
			multiply(A, B, result, xdim, ydim);
			//print the result matrix to a File
			sprintf(filename, "result.txt");
			if( file_exist(filename) && (remove( filename ) != 0 )){
				cout << "Error deleting file" << filename;
				return 1;
			}
			print_matrix_to_file(result, filename, xdim, xdim, 0);

			//free the blocked memory
			free(A[0]);
			free(A);
			free(B[0]);
			free(B);
			free(result[0]);
			free(result);
			free(recv_dimensions[0]);
			free(recv_dimensions);
			free(sendcnt[0]);
			free(sendcnt);
			free(displ[0]);
			free(displ);

			cout << "Look up files A.txt and B.txt to see the matrixes used, and look up result.txt to see the result" << endl;
			cout << "Program Finished" << endl;

			MPI::Finalize();
			return 0;
		}

		//calculate the portion of rows every process gets and the rest portion of the rows are not evenly spreadable
		portion = xdim/(size);
		rest = xdim%(size);

		//save the original dimensions to broadcast them alter on to each process
		recv_dimensions[1][0] = xdim;
		recv_dimensions[1][1] = ydim;

		//save the number of elements that will be send and the displacement in the array for each process in arrays
		int sum0 = 0;
		int sum1 = 0;
		for(counter = 0; counter < size; counter++){
			//send counts to scatter A and B
			sendcnt[0][counter] = portion*ydim;
			//send counts to gather result
			sendcnt[1][counter] = portion*xdim;
			//dimensions of the portion array for each process
			recv_dimensions[0][counter] = portion;
			//if a rest exist spread it to the processes
			if(rest > 0){
				sendcnt[0][counter] += ydim;
				sendcnt[1][counter] += xdim;
				recv_dimensions[0][counter] += 1;
				rest--;
			}
			//displacement to scatter A and B
			displ[0][counter] = sum0;
			//displacement to gather result
			displ[1][counter] = sum1;
			//calculate the displacement
			sum0 += sendcnt[0][counter];
			sum1 += sendcnt[1][counter];
		}
	}

	//broadcast the utility arrays to every process
	MPI::COMM_WORLD.Bcast(&recv_dimensions[0][0], 2*size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&sendcnt[0][0], 2*size, MPI_INT, 0);
	MPI::COMM_WORLD.Bcast(&displ[0][0], 2*size, MPI_INT, 0);

	// allocate memory for the portion array of each process
	recvA = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[1][1]);
	recvB = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[1][1]);

	//scatter A and B to the processes
	MPI::COMM_WORLD.Scatterv(&(A[0][0]), sendcnt[0], displ[0], MPI_DOUBLE, &(recvA[0][0]), sendcnt[0][rank], MPI_DOUBLE, 0);
	MPI::COMM_WORLD.Scatterv(&(B[0][0]), sendcnt[0], displ[0], MPI_DOUBLE, &(recvB[0][0]), sendcnt[0][rank], MPI_DOUBLE, 0);

	//initialize and array for the partial result and allocate memory to it
	double **temp_result;
	temp_result = alloc_2d_array_double(recv_dimensions[0][rank], recv_dimensions[1][0]);
	counter = rank;
	do{
		//calculate a part of the result with the received part arrays
		for(int i = 0; i < recv_dimensions[0][rank]; i++){ // iterate through the rows of part matrix A
			//calculate an offset depending on the part of the second matrix that is currently being held
			int offset = ((displ[0][counter])/(recv_dimensions[1][1]));
			for(int j = 0; j < recv_dimensions[0][counter]; j++){ // iterate through the columns of part matrix B
				// make sure the place in the result array is empty before filling the actual result in it
				temp_result[i][offset] = 0;
				for(int k = 0; k < recv_dimensions[1][1]; k++){ // iterate through the columns of part matrix A and the rows of part matrix B
					//perform the multiplication
					temp_result[i][offset] += recvA[i][k] * recvB[j][k];
				}
				offset++;
			}
		}
		//exchange the part of the second array in a round robbin between the processes
		//the root process starts the chain
		if(rank == 0){
			//send the current part matrix to the next process
			MPI::COMM_WORLD.Send(&(recvB[0][0]), sendcnt[0][counter], MPI_DOUBLE, rank+1, 0);
			//adjust the counter to point to the next part of the matrix
			if(counter == 0){
				counter = size;
			}
			counter--;
			//reallocate memory for the part matrix so it fits for the part that is being received
			free(recvB[0]);
			free(recvB);
			recvB = alloc_2d_array_double(recv_dimensions[0][counter], recv_dimensions[1][1]);
			//receive the next part of the matrix from the last process
			MPI::COMM_WORLD.Recv(&(recvB[0][0]), sendcnt[0][counter], MPI_DOUBLE, size-1, 0);
		}else {
			//save the current part of the matrix and the current state of the counter in temporary variables
			double **tempB_part;
			int temp_counter;
			tempB_part = alloc_2d_array_double(recv_dimensions[0][counter], recv_dimensions[1][1]);
			copy(&recvB[0][0], &recvB[0][0]+(recv_dimensions[0][counter]*recv_dimensions[1][1]), &tempB_part[0][0]);
			temp_counter = counter;
			//adjust the counter to point to the next part of the matrix
			if(counter == 0){
				counter = size;
			}
			counter--;
			//reallocate memory for the part matrix so it fits for the part that is being received
			free(recvB[0]);
			free(recvB);
			recvB = alloc_2d_array_double(recv_dimensions[0][counter], recv_dimensions[1][1]);
			//receive the next part of the matrix from the previous process
			MPI::COMM_WORLD.Recv(&(recvB[0][0]), sendcnt[0][counter], MPI_DOUBLE, rank-1, 0);
			//send the temporary saved part matrix to the next process
			if(rank == (size-1)){ // if you are the last process, send to the root process
				MPI::COMM_WORLD.Send(&(tempB_part[0][0]), sendcnt[0][temp_counter], MPI_DOUBLE, 0, 0);
			}else{
				MPI::COMM_WORLD.Send(&(tempB_part[0][0]), sendcnt[0][temp_counter], MPI_DOUBLE, rank+1, 0);
			}
			//free the temporary storage for the matrix part
			free(tempB_part[0]);
			free(tempB_part);
		}
	}while (counter != rank);//run one round for each process

	////allocate memory for the result array
	if(rank == 0){
		result = alloc_2d_array_double(recv_dimensions[1][0], recv_dimensions[1][0]);
	}

	// Gather the partial results from the processes and assemble them in the result array
	MPI::COMM_WORLD.Gatherv(&(temp_result[0][0]), sendcnt[1][rank], MPI_DOUBLE, &result[0][0], sendcnt[1], displ[1], MPI_DOUBLE, 0);

	//print the result matrix to a File
	if(rank == 0){
		char filename[15] = {0};
		sprintf(filename, "result.txt");
		if( file_exist(filename) && (remove( filename ) != 0 )){
			cout << "Error deleting file" << filename;
			return 1;
		}
		print_matrix_to_file(result, filename, recv_dimensions[1][0], recv_dimensions[1][0], 0);
		cout << "Look up files A.txt and B.txt to see the matrixes used, and look up result.txt to see the result" << endl;
	}

	//free the blocked memory
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

