#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <omp.h>
#include <time.h>
#include <assert.h>

// Define the number of rows and columns
#define NROWS 100
#define NCOLS 100

// Structure to represent a sparse matrix in CRS format
typedef struct {
    size_t nrows;
    size_t ncols;
    size_t nnz;        // Total number of non-zero elements
    size_t *row_ptr;   // Row pointer array (size nrows + 1)
    int *col_ind;      // Column indices array (size nnz)
    int *values;       // Non-zero values array (size nnz)
    int *B;            // Additional array for multiplication (size B_size)
    int *C;            // Additional array for multiplication (size B_size)
    size_t B_size;     // Size of B and C arrays
} SparseMatrix;

/**
 * @brief Generates a pseudo-random sparse matrix in CRS format.
 *
 * @param prob Probability of an element being non-zero.
 * @param num_threads Number of OpenMP threads to use.
 * @return Pointer to the generated SparseMatrix.
 */
SparseMatrix* generateSparseMatrix(double prob, int num_threads) {
    // Initialize OpenMP
    omp_set_num_threads(num_threads);

    // Allocate memory for the SparseMatrix structure
    SparseMatrix *mat = (SparseMatrix*) malloc(sizeof(SparseMatrix));
    if (mat == NULL) {
        perror("Failed to allocate memory for SparseMatrix");
        exit(EXIT_FAILURE);
    }

    mat->nrows = NROWS;
    mat->ncols = NCOLS;
    mat->nnz = 0;
    mat->B_size = 0;

    // Allocate memory for row_ptr array
    mat->row_ptr = (size_t*) malloc((mat->nrows + 1) * sizeof(size_t));
    if (mat->row_ptr == NULL) {
        perror("Failed to allocate memory for row_ptr");
        free(mat);
        exit(EXIT_FAILURE);
    }
    mat->row_ptr[0] = 0; // Initialize the first element

    // Static variable to ensure unique seed per call
    static int seed_increment = 0;
    int current_increment;

    // Assign unique increment for this call
    #pragma omp critical
    {
        current_increment = seed_increment++;
    }

    // First Pass: Count non-zero elements per row
    size_t total_nnz = 0; // Total number of non-zero elements
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

    // Compute cumulative row_ptr
    for (size_t i = 0; i < mat->nrows; i++) {
        mat->row_ptr[i + 1] += mat->row_ptr[i];
    }

    // Allocate memory for col_ind and values arrays
    mat->col_ind = (int*) malloc(mat->nnz * sizeof(int));
    mat->values = (int*) malloc(mat->nnz * sizeof(int));
    if (mat->col_ind == NULL || mat->values == NULL) {
        perror("Failed to allocate memory for col_ind or values");
        free(mat->row_ptr);
        free(mat);
        exit(EXIT_FAILURE);
    }

    // Calculate the number of zero rows
    size_t num_zero_rows = 0;
    for (size_t i = 0; i < mat->nrows; i++) {
        if (mat->row_ptr[i + 1] == mat->row_ptr[i]) {
            num_zero_rows++;
        }
    }

    // Allocate memory for B and C arrays
    // Each zero row requires two entries in B and C
    mat->B_size = mat->nnz + (2 * num_zero_rows);
    mat->B = (int*) malloc(mat->B_size * sizeof(int));
    mat->C = (int*) malloc(mat->B_size * sizeof(int));
    if (mat->B == NULL || mat->C == NULL) {
        perror("Failed to allocate memory for B or C");
        free(mat->col_ind);
        free(mat->values);
        free(mat->row_ptr);
        free(mat);
        exit(EXIT_FAILURE);
    }

    // Precompute starting indices for B and C
    size_t *bc_start_indices = (size_t*) malloc(mat->nrows * sizeof(size_t));
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
            bc_total += 2; // Two zeros for empty rows
        } else {
            bc_total += mat->row_ptr[i + 1] - mat->row_ptr[i];
        }
    }

    // Ensure that bc_total matches B_size
    assert(bc_total == mat->B_size);

    // Second Pass: Fill col_ind, values, B, and C arrays
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
                // Entire row is zero; add two zeros to B and C
                mat->B[bc_idx] = 0;
                mat->C[bc_idx] = 0;
                mat->B[bc_idx + 1] = 0;
                mat->C[bc_idx + 1] = 0;
            } else {
                // Generate unique column indices
                size_t *cols = (size_t*) malloc(row_nnz * sizeof(size_t));
                if (cols == NULL) {
                    perror("Failed to allocate memory for cols");
                    exit(EXIT_FAILURE);
                }

                size_t count = 0;
                while (count < row_nnz) {
                    size_t col = rand_r(&seed) % mat->ncols;
                    // Check for duplicates
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

                // Assign the columns and values
                for (size_t k = 0; k < row_nnz; k++) {
                    size_t j = cols[k];
                    int value = (int)(rand_r(&seed) % 10 + 1); // Values between 1 and 10

                    mat->col_ind[row_start + k] = (int)j;
                    mat->values[row_start + k] = value;

                    mat->B[bc_idx + k] = value;
                    mat->C[bc_idx + k] = (int)j;
                }

                free(cols);

                // Optional: Assert to ensure correct filling
                assert((bc_idx + row_nnz) <= mat->B_size);
            }
        }
    }

    // Free the auxiliary bc_start_indices array
    free(bc_start_indices);

    return mat;
}

/**
 * @brief Frees the memory allocated for a SparseMatrix.
 *
 * @param mat Pointer to the SparseMatrix to be freed.
 */
void freeSparseMatrix(SparseMatrix *mat) {
    if (mat != NULL) {
        free(mat->B);
        free(mat->C);
        free(mat->row_ptr);
        free(mat->col_ind);
        free(mat->values);
        free(mat);
    }
}

/**
 * @brief Main function to demonstrate the generation of sparse matrices.
 *
 * @param argc Argument count.
 * @param argv Argument vector.
 * @return int Exit status.
 */
int main(int argc, char *argv[]) {
    // Check for correct number of command-line arguments
    if (argc != 3) {
        fprintf(stderr, "Usage: %s <probability> <num_threads>\n", argv[0]);
        return EXIT_FAILURE;
    }

    // Parse probability
    double prob = atof(argv[1]);
    if (prob < 0.0 || prob > 1.0) {
        fprintf(stderr, "Error: Probability must be between 0 and 1.\n");
        return EXIT_FAILURE;
    }

    // Parse number of threads
    int num_threads = atoi(argv[2]);
    if (num_threads < 1 || num_threads > 256) {
        fprintf(stderr, "Error: Number of threads must be between 1 and 256.\n");
        return EXIT_FAILURE;
    }

    // Generate Sparse Matrix X
    SparseMatrix *X = generateSparseMatrix(prob, num_threads);
    printf("Sparse Matrix X generated with %zu non-zero elements.\n", X->nnz);

    // Generate Sparse Matrix Y
    SparseMatrix *Y = generateSparseMatrix(prob, num_threads);
    printf("Sparse Matrix Y generated with %zu non-zero elements.\n", Y->nnz);

    // Access B and C for X and Y
    int *B_X = X->B;
    int *C_X = X->C;
    int *B_Y = Y->B;
    int *C_Y = Y->C;

    // To check, print first 10 elements of B and C for X and Y
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

    // Clean up allocated memory
    freeSparseMatrix(X);
    freeSparseMatrix(Y);

    return EXIT_SUCCESS;
}
