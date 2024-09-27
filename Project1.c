#include <omp.h>
#include <stdio.h>

// generate three pairs of sparse matrices, X and Y with size 100000 x 100000
// when an entry of the matrix can be non-zero with probabilities 0.01, 0.02, and 0.05
    // e.g. in first case can generate random number between 1 and 100 for each entry of the matrix,
    // and the entry is non-zero if the random number is between 0 and 1
    // generate another small integer between 1 and 10 and store it as the entry in that matrix location

// create the two arrays B and C for row compression for each matrix
    // e.g. for multiplying two matrices X and Y of size 100000 x 100000 each
    // generate two matrices XB and XC (for X) and YB and YC (for Y)
    // and use these four matrices for multiplying X and Y

    // row compression format example, where column indices of the non-zero elements in each row are stored:
    // int A[4][4] = {{2, 0, 0, 1},
    //                {0, 0, 0, 3},
    //                {0, 1, 0, 0},
    //                {0, 0, 1, 0}};
    // would be represented by two matrices,
    // the B matrix stores the actual elements in each row:
    // int B[4][2] = {{2, 1},
    //                {3},
    //                {1},
    //                {1}};
    // and the C matrix stores the column indices of the non-zero elements in each row:
    // int C[4][2] = {{0, 3},
    //                {3},
    //                {1},
    //                {2}};

    // If there is no non-zero element in a row, the corresponding rows of B and C will store two consecutive 0s.
    // For example:
    // int A = {{2, 0, 0, 1},
    //          {0, 0, 0, 3},
    //          {0, 0, 0, 0},
    //          {0, 0, 1, 0}};
    // would be represented by:
    // int B[4][2] = {{2, 1},
    //                {3},
    //                {0, 0},
    //                {1}};
    // int C[4][2] = {{0, 3},
    //                {3},
    //                {0, 0},
    //                {2}};



// [1] 2 marks for ordinary matrix multiplication algorithm
// Write an ordinary matrix multiplication algorithm so that we can check your results.
// The number of rows, number of columns should be declared at the top of your code using #define directives.

// [2] 4 marks for correctly generating the B and C matrices
// Write separate code for generating the B and C matrices given a matrix X.
// You can generate the matrix X inside this code

// [3] 6 marks for performance evaluation
// Multiply the three sets of matrices above and evaluate their running times
    // You have to write OpenMP code and vary the number of threads
    // Though Setonix has 28 cores you can use more threads than 28.
    // You have to find the number of threads that gives the best performance for each of these cases

    // Performance evaluation section should include:
        // - experiments for determining the optimal number of threads for the best performance for the three matrix sizes
        // - an analysis of performance using the four scheduling strategies for scheduling for loops for the matrix multiplication problem for one of the three matrix sizes
    
    // Code should write the final results for the B and C matrices in two separate files named FileB and FileC
        // FILE *fp, *fopen();
        // fp=fopen("FileB","w"); /*opens the file named FileB for writing*/
        // fprintf(fp,"%d", i); /* write variable i to the file*/
    // You should allocate matrices using dynamic allocation.

#define ROWS 100        // number of rows (#TODO 100,000 in final)
#define COLS 100        // number of columns (#TODO 100,000 in final)

struct sparse_matrix {
    int nrows;
    int ncols;
    int *offset;
    int *index;
    double *value;
};

typedef struct sparse_matrix sparse_matrix_t;

// init_sparse_matrix() initialises a sparse matrix, with given number of rows/columns/non-zero values
void init_sparse_matrix(sparse_matrix_t *input_matrix, int nrows, int ncols, int nnonzero) {
    // Check input is valid
    assert(nrows >= 0);
    assert(ncols >= 0);
    assert(nnonzero >= 0);

    // Set inital matrix values to 0
    memset(input_matrix, 0x0, sizeof(*input_matrix));

    // Allocate input values and sufficient memory for matrix
    input_matrix->nrows = nrows;
    input_matrix->ncols = ncols;
    input_matrix->offset = (int *)malloc((nrows + 1) * sizeof(*input_matrix->offset));
    input_matrix->index = (int *)malloc(nnonzero * sizeof(*input_matrix->index));
    input_matrix->value = (double *)malloc(nnonzero * sizeof(*input_matrix->value));

    // Check memory allocation was successful
    if ((input_matrix->offset == NULL) || (input_matrix->index == NULL) || (input_matrix->value == NULL)) {
        fprintf(stderr, "ERROR: Allocation to the sparse matrix failed.\n");
        exit(1);
    }

    memset(input_matrix->offset, 0x0, (nrows + 1) * sizeof(*input_matrix->offset));
}

// empty_sparse_matrix() frees memory allocated to a sparse matrix
void empty_sparse_matrix(sparse_matrix_t *input_matrix) {
    free(input_matrix->offset);
    free(input_matrix->index);
    free(input_matrix->value);
    memset(input_matrix, 0x0, sizeof(*input_matrix));
}

void generate_random() {
    // generate random number between 1 and 100 for each entry of the matrix,
    // and the entry is non-zero if the random number is between 0 and 1
    // generate another small integer between 1 and 10 and store it as the entry in that matrix location
}

int main() {
    omp_set_num_threads(4);
    #pragma omp parallel 
    {
    #pragma omp for
        for (int i = 0; i < 10; i++) {
            printf("Thread %d, i = %d\n", omp_get_thread_num(), i);
        }
    }
    return 0;
}