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

template<int width, int height>
void generate(double (&matrix)[width][height]){
	random_device rd;
	mt19937 gen(rd());
	uniform_real_distribution<double> dist(0.0, 1000.0);
	for(int i = 0; i < width; i++){
		for(int j = 0; j < height; j++){
			matrix[i][j] = dist(gen);
		}
	}
}

template<int widthA, int heightA, int widthB, int heightB>
void multiply(double (&matrixA)[widthA][heightA], double (&matrixB)[widthB][heightB], double (&result)[widthA][heightB]){
	for(int i = 0; i < heightA; i++){
		for(int j = 0; j < widthB; j++){
			for(int k = 0; k < heightB; k++){
				result[i][j] += matrixA[i][k] * matrixB[k][j];
			}
		}
	}
}

int main(int argc, char *argv[]) {
	double matrixA[5][5];
	double matrixB[5][5];
	double result[5][5];
	generate(matrixA);
	generate(matrixB);

	MPI::Init(argc, argv);

	multiply(matrixA, matrixB, result);

	MPI::Finalize();
	cout << "Finished";
	return 0;
}

