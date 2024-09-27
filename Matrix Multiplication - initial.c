#include <stdio.h>
#include <stdlib.h>
#include <time.h>
#include <omp.h>

#define NCOLS 100
#define NROWS 100

void generate_random_matrix(int **matrix, int rows, int cols) {
    // Seed the random number generator
    srand((unsigned int)time(NULL));

    // Generate random integer values for the matrix between 0 and 10
    for (int i = 0; i < rows; i++) {
        for (int j = 0; j < cols; j++) {
            matrix[i][j] = rand() % 11; // Generates integers from 0 to 10
        }
    }
}

void matrix_multiply_standard(int **A, int **B, int **C, int n_rows, int n_cols) {
    #pragma omp parallel for
    for (int i = 0; i < n_rows; i++) {
        for (int j = 0; j < n_cols; j++) {
            int sum = 0;
            for (int k = 0; k < n_cols; k++) {
                sum += A[i][k] * B[k][j];
            }
            C[i][j] = sum;
        }
    }
}


int main() {
    // Allocate memory for matrices A, B, and C
    int **A = malloc(NROWS * sizeof(int *));
    int **B = malloc(NROWS * sizeof(int *));
    int **C_standard = malloc(NROWS * sizeof(int *));
    int **C_transposed = malloc(NROWS * sizeof(int *));
    for (int i = 0; i < NROWS; i++) {
        A[i] = malloc(NCOLS * sizeof(int));
        B[i] = malloc(NCOLS * sizeof(int));
        C_standard[i] = malloc(NCOLS * sizeof(int));
        C_transposed[i] = malloc(NCOLS * sizeof(int));
    }

    // Generate random matrices A and B
    generate_random_matrix(A, NROWS, NCOLS);
    generate_random_matrix(B, NROWS, NCOLS);

    // Measure execution time for standard multiplication
    double start_time = omp_get_wtime();
    matrix_multiply_standard(A, B, C_standard, NROWS, NCOLS);
    double end_time = omp_get_wtime();
    printf("Standard multiplication time: %f seconds\n", end_time - start_time);
    
    // Free allocated memory
    for (int i = 0; i < NROWS; i++) {
        free(A[i]);
        free(B[i]);
        free(C_standard[i]);
        free(C_transposed[i]);
    }
    free(A);
    free(B);
    free(C_standard);
    free(C_transposed);

    return 0;
}