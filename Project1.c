#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <time.h>
#include <assert.h>

#define PROB 0.01
#define NROWS 100000
#define NCOLS 100000

typedef struct {
    size_t nrows;
    size_t ncols;
    size_t nnz;
    size_t* row_ptr;
    int* col_ind;
    int* values;
    int* B;
    int* C;
    size_t B_size;
} SparseMatrix;

// Function to write an array to a file
void writeArrayToFile(const char* filename, int* array, size_t size) {
    FILE* fp = fopen(filename, "w");
    if (fp == NULL) {
        perror("Error opening file for writing");
        return;
    }

    for (size_t i = 0; i < size; i++) {
        fprintf(fp, "%d\n", array[i]);
    }

    fclose(fp);
}

// Function to multiply two dense matrices
void multiply_matrix(int** A, int** B, int** C, int size) {
#pragma omp parallel for collapse(2)
    for (int i = 0; i < size; i++) {
        for (int j = 0; j < size; j++) {
            C[i][j] = 0;
            for (int k = 0; k < size; k++) {
                C[i][j] += A[i][k] * B[k][j];
            }
        }
    }
}

// Function to multiply two sparse matrices
void sparse_matrix_multiply(SparseMatrix* X, SparseMatrix* Y, SparseMatrix* Z) {
    if (X->ncols != Y->nrows) {
        printf("Error: Incompatible matrix dimensions for multiplication.\n");
        return;
    }

    Z->nrows = X->nrows;
    Z->ncols = Y->ncols;
    Z->nnz = 0;

    Z->row_ptr = (size_t*)malloc((Z->nrows + 1) * sizeof(size_t));
    Z->row_ptr[0] = 0;

    size_t estimated_nnz = 0;
    size_t* col_accumulator = (size_t*)calloc(Z->ncols, sizeof(size_t));
    size_t num_zero_rows = 0;

    for (size_t i = 0; i < X->nrows; i++) {
        int row_non_zero = 0;
        for (size_t x_idx = X->row_ptr[i]; x_idx < X->row_ptr[i + 1]; x_idx++) {
            int k = X->col_ind[x_idx];
            int x_value = X->values[x_idx];

            for (size_t y_idx = Y->row_ptr[k]; y_idx < Y->row_ptr[k + 1]; y_idx++) {
                int j = Y->col_ind[y_idx];
                int y_value = Y->values[y_idx];

                if (x_value * y_value != 0) {
                    if (col_accumulator[j] == 0) {
                        estimated_nnz++;
                        col_accumulator[j] = 1;
                    }
                    row_non_zero = 1;
                }
            }
        }

        if (row_non_zero == 0) {
            num_zero_rows++;
        }

        for (size_t j = 0; j < Z->ncols; j++) {
            col_accumulator[j] = 0;
        }

        Z->row_ptr[i + 1] = estimated_nnz;
    }

    Z->nnz = estimated_nnz;
    Z->col_ind = (int*)malloc(Z->nnz * sizeof(int));
    Z->values = (int*)malloc(Z->nnz * sizeof(int));

    Z->B_size = Z->nnz + (2 * num_zero_rows);
    Z->B = (int*)malloc(Z->B_size * sizeof(int));
    Z->C = (int*)malloc(Z->B_size * sizeof(int));

    size_t current_nnz = 0;
    size_t bc_idx = 0;
    int* row_sum = (int*)calloc(Z->ncols, sizeof(int));

    for (size_t i = 0; i < X->nrows; i++) {
        int row_non_zero = 0;

        for (size_t x_idx = X->row_ptr[i]; x_idx < X->row_ptr[i + 1]; x_idx++) {
            int k = X->col_ind[x_idx];
            int x_value = X->values[x_idx];

            for (size_t y_idx = Y->row_ptr[k]; y_idx < Y->row_ptr[k + 1]; y_idx++) {
                int j = Y->col_ind[y_idx];
                int y_value = Y->values[y_idx];

                row_sum[j] += x_value * y_value;
                row_non_zero = 1;
            }
        }

        if (row_non_zero == 0) {
            Z->B[bc_idx] = 0;
            Z->C[bc_idx] = 0;
            Z->B[bc_idx + 1] = 0;
            Z->C[bc_idx + 1] = 0;
            bc_idx += 2;
        } else {
            for (size_t j = 0; j < Z->ncols; j++) {
                if (row_sum[j] != 0) {
                    Z->col_ind[current_nnz] = j;
                    Z->values[current_nnz] = row_sum[j];
                    Z->B[bc_idx] = row_sum[j];
                    Z->C[bc_idx] = j;
                    current_nnz++;
                    bc_idx++;
                    row_sum[j] = 0;
                }
            }
        }

        Z->row_ptr[i + 1] = current_nnz;
    }

    free(col_accumulator);
    free(row_sum);
}

// Function to generate a sparse matrix
SparseMatrix* generateSparseMatrix(double prob) {
    SparseMatrix* mat = (SparseMatrix*)malloc(sizeof(SparseMatrix));
    if (mat == NULL) {
        perror("Failed to allocate memory for SparseMatrix");
        exit(EXIT_FAILURE);
    }

    mat->nrows = NROWS;
    mat->ncols = NCOLS;
    mat->nnz = 0;
    mat->B_size = 0;

    mat->row_ptr = (size_t*)malloc((mat->nrows + 1) * sizeof(size_t));
    if (mat->row_ptr == NULL) {
        perror("Failed to allocate memory for row_ptr");
        free(mat);
        exit(EXIT_FAILURE);
    }
    mat->row_ptr[0] = 0;

    static int seed_increment = 0;
    int current_increment;

#pragma omp critical
    {
        current_increment = seed_increment++;
    }

    size_t total_nnz = 0;
#pragma omp parallel
    {
        unsigned int seed = (unsigned int)(time(NULL) + current_increment) ^ omp_get_thread_num();

#pragma omp for schedule(static) reduction(+:total_nnz)
        for (size_t i = 0; i < mat->nrows; i++) {
            size_t row_nnz = 0;
            for (size_t j = 0; j < mat->ncols; j++) {
                double rand_prob = (double)rand_r(&seed) / RAND_MAX;
                if (rand_prob < prob) {
                    row_nnz++;
                }
            }
            mat->row_ptr[i + 1] = row_nnz;
            total_nnz += row_nnz;
        }
    }
    mat->nnz = total_nnz;

    for (size_t i = 0; i < mat->nrows; i++) {
        mat->row_ptr[i + 1] += mat->row_ptr[i];
    }

    mat->col_ind = (int*)malloc(mat->nnz * sizeof(int));
    mat->values = (int*)malloc(mat->nnz * sizeof(int));
    if (mat->col_ind == NULL || mat->values == NULL) {
        perror("Failed to allocate memory for col_ind or values");
        free(mat->row_ptr);
        free(mat);
        exit(EXIT_FAILURE);
    }

    size_t num_zero_rows = 0;
    for (size_t i = 0; i < mat->nrows; i++) {
        if (mat->row_ptr[i + 1] == mat->row_ptr[i]) {
            num_zero_rows++;
        }
    }

    mat->B_size = mat->nnz + (2 * num_zero_rows);
    mat->B = (int*)malloc(mat->B_size * sizeof(int));
    mat->C = (int*)malloc(mat->B_size * sizeof(int));
    if (mat->B == NULL || mat->C == NULL) {
        perror("Failed to allocate memory for B or C");
        free(mat->col_ind);
        free(mat->values);
        free(mat->row_ptr);
        free(mat);
        exit(EXIT_FAILURE);
    }

    size_t* bc_start_indices = (size_t*)malloc(mat->nrows * sizeof(size_t));
    if (bc_start_indices == NULL) {
        perror("Failed to allocate memory for bc_start_indices");
        free(mat->B);
        free(mat->C);
        free(mat->col_ind);
        free(mat->values);
        free(mat->row_ptr);
        free(mat);
        exit(EXIT_FAILURE);
    }

    size_t bc_total = 0;
    for (size_t i = 0; i < mat->nrows; i++) {
        bc_start_indices[i] = bc_total;
        if (mat->row_ptr[i + 1] == mat->row_ptr[i]) {
            bc_total += 2;
        } else {
            bc_total += mat->row_ptr[i + 1] - mat->row_ptr[i];
        }
    }

    assert(bc_total == mat->B_size);

#pragma omp parallel
    {
        unsigned int seed = (unsigned int)(time(NULL) + current_increment) ^ omp_get_thread_num();

#pragma omp for schedule(static)
        for (size_t i = 0; i < mat->nrows; i++) {
            size_t row_start = mat->row_ptr[i];
            size_t row_end = mat->row_ptr[i + 1];
            size_t row_nnz = row_end - row_start;
            size_t bc_idx = bc_start_indices[i];

            if (row_nnz == 0) {
                mat->B[bc_idx] = 0;
                mat->C[bc_idx] = 0;
                mat->B[bc_idx + 1] = 0;
                mat->C[bc_idx + 1] = 0;
            } else {
                size_t* cols = (size_t*)malloc(row_nnz * sizeof(size_t));
                if (cols == NULL) {
                    perror("Failed to allocate memory for cols");
                    exit(EXIT_FAILURE);
                }

                size_t count = 0;
                while (count < row_nnz) {
                    size_t col = rand_r(&seed) % mat->ncols;
                    int duplicate = 0;
                    for (size_t k = 0; k < count; k++) {
                        if (cols[k] == col) {
                            duplicate = 1;
                            break;
                        }
                    }
                    if (!duplicate) {
                        cols[count++] = col;
                    }
                }

                for (size_t k = 0; k < row_nnz; k++) {
                    size_t j = cols[k];
                    int value = (int)(rand_r(&seed) % 10 + 1);

                    mat->col_ind[row_start + k] = (int)j;
                    mat->values[row_start + k] = value;

                    mat->B[bc_idx + k] = value;
                    mat->C[bc_idx + k] = (int)j;
                }

                free(cols);

                assert((bc_idx + row_nnz) <= mat->B_size);
            }
        }
    }

    free(bc_start_indices);

    return mat;
}

// Function to free the memory allocated for a sparse matrix
void freeSparseMatrix(SparseMatrix* mat) {
    if (mat != NULL) {
        if (mat->row_ptr != NULL) {
            free(mat->row_ptr);
            mat->row_ptr = NULL;
        }
        if (mat->col_ind != NULL) {
            free(mat->col_ind);
            mat->col_ind = NULL;
        }
        if (mat->values != NULL) {
            free(mat->values);
            mat->values = NULL;
        }
        if (mat->B != NULL) {
            free(mat->B);
            mat->B = NULL;
        }
        if (mat->C != NULL) {
            free(mat->C);
            mat->C = NULL;
        }
        free(mat);
    }
}

int main(int argc, char* argv[]) {
    if (argc != 2) {
        fprintf(stderr, "Usage: %s <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }
    double start_time = omp_get_wtime();

    omp_set_num_threads(atoi(argv[1]));
    printf("Generating sparse matrix with %d threads...\n", omp_get_max_threads());

    SparseMatrix* X = generateSparseMatrix(PROB);
    printf("Sparse Matrix X generated with %zu non-zero elements.\n", X->nnz);

    SparseMatrix* Y = generateSparseMatrix(PROB);
    printf("Sparse Matrix Y generated with %zu non-zero elements.\n", Y->nnz);

    int* B_X = X->B;
    int* C_X = X->C;
    int* B_Y = Y->B;
    int* C_Y = Y->C;

    printf("\nFirst 10 elements of B and C for Matrix X:\n");
    size_t print_limit_X = (X->B_size < 10) ? X->B_size : 10;
    for (size_t i = 0; i < print_limit_X; i++) {
        printf("X: B[%zu] = %d, C[%zu] = %d\n", i, B_X[i], i, C_X[i]);
    }
    printf("\nFirst 10 elements of B and C for Matrix Y:\n");
    size_t print_limit_Y = (Y->B_size < 10) ? Y->B_size : 10;
    for (size_t i = 0; i < print_limit_Y; i++) {
        printf("Y: B[%zu] = %d, C[%zu] = %d\n", i, B_Y[i], i, C_Y[i]);
    }

    SparseMatrix* Z = (SparseMatrix*)malloc(sizeof(SparseMatrix));
    if (Z == NULL) {
        fprintf(stderr, "Error: Unable to allocate memory for the result matrix Z.\n");
        freeSparseMatrix(X);
        freeSparseMatrix(Y);
        return EXIT_FAILURE;
    }

    if (X->ncols != Y->nrows) {
        fprintf(stderr, "Error: Matrices cannot be multiplied due to incompatible dimensions.\n");
        freeSparseMatrix(X);
        freeSparseMatrix(Y);
        free(Z);
        return EXIT_FAILURE;
    }

    sparse_matrix_multiply(X, Y, Z);

    printf("\nSparse Matrix Z generated with %zu non-zero elements.\n", Z->nnz);

    writeArrayToFile("FileB.csv", Z->B, Z->B_size);
    writeArrayToFile("FileC.csv", Z->C, Z->B_size);

    printf("First 10 elements of B and C for Matrix Z:\n");
    size_t print_limit_Z = (Z->B_size < 10) ? Z->B_size : 10;
    for (size_t i = 0; i < print_limit_Z; i++) {
        printf("Z: B[%zu] = %d, C[%zu] = %d\n", i, Z->B[i], i, Z->C[i]);
    }

    freeSparseMatrix(X);
    freeSparseMatrix(Y);
    freeSparseMatrix(Z);

    double end_time = omp_get_wtime();
    double elapsed_time = end_time - start_time;

    printf("Time taken to generate and process matrices:\n%f seconds.\n", elapsed_time);

    return EXIT_SUCCESS;
}
