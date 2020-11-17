/* 
 * Name: Daeho Kim
 * Login ID: kdh0324
 * 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */
#include <stdio.h>

#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void transpose_32_32(int A[32][32], int B[32][32]);
void transpose_64_64(int A[64][64], int B[64][64]);
void transpose_61_67(int A[67][61], int B[61][67]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N]) {
    if (M == 32)
        transpose_32_32(A, B);
    else if (M == 64)
        transpose_64_64(A, B);
    else
        transpose_61_67(A, B);
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */
void transpose_32_32(int A[32][32], int B[32][32]) {
    for (int i = 0; i < 32; i += 8) {
        for (int j = 0; j < 32; j++) {
            int temp0 = A[j][i];
            int temp1 = A[j][i + 1];
            int temp2 = A[j][i + 2];
            int temp3 = A[j][i + 3];
            int temp4 = A[j][i + 4];
            int temp5 = A[j][i + 5];
            int temp6 = A[j][i + 6];
            int temp7 = A[j][i + 7];

            B[i][j] = temp0;
            B[i + 1][j] = temp1;
            B[i + 2][j] = temp2;
            B[i + 3][j] = temp3;
            B[i + 4][j] = temp4;
            B[i + 5][j] = temp5;
            B[i + 6][j] = temp6;
            B[i + 7][j] = temp7;
        }
    }
}

void transpose_64_64(int A[64][64], int B[64][64]) {
    for (int i = 0; i < 64; i += 4)
        for (int j = 0; j < 64; j++) {
            int temp0 = A[j][i];
            int temp1 = A[j][i + 1];
            int temp2 = A[j][i + 2];
            int temp3 = A[j][i + 3];

            B[i][j] = temp0;
            B[i + 1][j] = temp1;
            B[i + 2][j] = temp2;
            B[i + 3][j] = temp3;
        }
}

void transpose_61_67(int A[67][61], int B[61][67]) {
    for (int i = 0; i < 61; i += 8)
        for (int j = 0; j < 67; j += 8)
            for (int k = j; k < j + 8 && k < 67; k++)
                for (int l = i; l < i + 8 && l < 61; l++) {
                    int temp = A[k][l];
                    B[l][k] = temp;
                }
}

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N]) {
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */
void registerFunctions() {
    /* Register your solution function */
    registerTransFunction(transpose_submit, transpose_submit_desc);

    /* Register any additional transpose functions */
    registerTransFunction(trans, trans_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N]) {
    int i, j;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; ++j) {
            if (A[i][j] != B[j][i]) {
                return 0;
            }
        }
    }
    return 1;
}
