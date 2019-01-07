/* 
 * trans.c - Matrix transpose B = A^T
 *
 * Each transpose function must have a prototype of the form:
 * void trans(int M, int N, int A[N][M], int B[M][N]);
 *
 * A transpose function is evaluated by counting the number of misses
 * on a 1KB direct mapped cache with a block size of 32 bytes.
 */ 
#include <stdio.h>
#include <assert.h>
#include "cachelab.h"

int is_transpose(int M, int N, int A[N][M], int B[M][N]);
void trans_32x32_8x8(int M, int N, int A[N][M], int B[M][N]);
void trans_64x64_4x4(int M, int N, int A[N][M], int B[M][N]);
void trans_64x64_8x4(int M, int N, int A[N][M], int B[M][N]);
void trans_64x64_8x8_4_4x4(int M, int N, int A[N][M], int B[M][N]);
void trans_67x61(int M, int N, int A[N][M], int B[M][N]);

/* 
 * transpose_submit - This is the solution transpose function that you
 *     will be graded on for Part B of the assignment. Do not change
 *     the description string "Transpose submission", as the driver
 *     searches for that string to identify the transpose function to
 *     be graded. 
 */
char transpose_submit_desc[] = "Transpose submission";
void transpose_submit(int M, int N, int A[N][M], int B[M][N])
{
	if (M == 32 && N == 32)
		trans_32x32_8x8(M, N, A, B);
	else if (M == 64 && N == 64)
		trans_64x64_8x8_4_4x4(M, N, A, B);
	else if (M == 61 && N == 67)
		trans_67x61(M, N, A, B);
}

/* 
 * You can define additional transpose functions below. We've defined
 * a simple one below to help you get started. 
 */ 

/* 
 * trans - A simple baseline transpose function, not optimized for the cache.
 */
char trans_desc[] = "Simple row-wise scan transpose";
void trans(int M, int N, int A[N][M], int B[M][N])
{
    int i, j, tmp;

    for (i = 0; i < N; i++) {
        for (j = 0; j < M; j++) {
            tmp = A[i][j];
            B[j][i] = tmp;
        }
    }    

}

char trans_32x32_8x8_desc[] = "32x32 transpose by 8x8 block";
void trans_32x32_8x8(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k, l;
	int tmp[8];
	for (i = 0; i < 32; i += 8)
		for (j = 0; j < 32; j += 8)
			for (k = 0; k < 8; k++)
			{
				for (l = 0; l < 8; l++)
				{
					tmp[l] = A[i + k][j + l];
				}
				for (l = 0; l < 8; l++)
				{
					B[j + l][i + k] = tmp[l];
				}
			}
}

char trans_64x64_4x4_desc[] = "64x64 transpose by 4x4 block";
void trans_64x64_4x4(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k, l;
	int tmp[4];
	for (i = 0; i < 64; i += 4)
		for (j = 0; j < 64; j += 4)
			for (k = 0; k < 4; k++)
			{
				for (l = 0; l < 4; l++)
				{
					tmp[l] = A[i + k][j + l];
				}
				for (l = 0; l < 4; l++)
				{
					B[j + l][i + k] = tmp[l];
				}
			}
}

char trans_64x64_8x4_desc[] = "64x64 transpose by 8x4 block";
void trans_64x64_8x4(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k, l;
	int tmp[4];
	for (i = 0; i < 64; i += 8)
		for (j = 0; j < 64; j += 4)
			for (k = 0; k < 8; k++)
			{
				for (l = 0; l < 4; l++)
				{
					tmp[l] = A[i + k][j + l];
				}
				for (l = 0; l < 4; l++)
				{
					B[j + l][i + k] = tmp[l];
				}
			}
}

char trans_64x64_8x8_4_4x4_desc[] = "64x64 transpose by 8x8 block and each block is"
				    "divided into four 4x4 sub-blocks";
void trans_64x64_8x8_4_4x4(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k, l;
	int tmp[8];
	for (i = 0; i < 64; i += 8)
		for (j = 0; j < 64; j += 8)
		{
			for (k = 0; k < 4; k++)
			{
				for (l = 0; l < 8; l++)
				{
					tmp[l] = A[i + k][j + l];
				}
				for (l = 0; l < 4; l++)
				{
					B[j + l][i + k] = tmp[l];
					B[j + l][i + 4 + k] = tmp[4 + l];
				}
			}

			for (k = 0; k < 4; k++)
			{	
				// 1
				for (l = 0; l < 4; l++)
				{
					tmp[l] = B[j + k][i + 4 + l];
				}

				// 2
				for (l = 4; l < 8; l++)
				{
					tmp[l] = A[i + l][j + k];
				}
				for (l = 4; l < 8; l++)
				{
					B[j + k][i + l] = tmp[l];
				}
				
				// 3
				for (l = 4; l < 8; l++)
				{
					tmp[l] = A[i + l][j + 4 + k];
				}

				// 4
				for (l = 0; l < 8; l++)
				{
					B[j + 4 + k][i + l] = tmp[l];
				}
			}
		}
}

char trans_67x61_desc[] = "67x61 transpose by 4x4 block";
void trans_67x61(int M, int N, int A[N][M], int B[M][N])
{
	int i, j, k, l;
	int block_size = 16;
	for (i = 0; i < 67; i += block_size)
		for (j = 0; j < 61; j += block_size)
			for (k = 0; k < block_size && i + k < 67; k++)
				for (l = 0; l < block_size && j + l < 61; l++)
				{
					B[j + l][i + k] = A[i + k][j + l];
				}
}

/*
 * registerFunctions - This function registers your transpose
 *     functions with the driver.  At runtime, the driver will
 *     evaluate each of the registered functions and summarize their
 *     performance. This is a handy way to experiment with different
 *     transpose strategies.
 */

void registerFunctions()
{
    // Register your solution function 
    registerTransFunction(transpose_submit, transpose_submit_desc); 

    // Register any additional transpose functions 
    //registerTransFunction(trans, trans_desc); 

    //registerTransFunction(trans_32x32_8x8, trans_32x32_8x8_desc);

    //registerTransFunction(trans_64x64_4x4, trans_64x64_4x4_desc);
    
    //registerTransFunction(trans_64x64_8x4, trans_64x64_8x4_desc);

    //registerTransFunction(trans_64x64_8x8_4_4x4, trans_64x64_8x8_4_4x4_desc);

    //registerTransFunction(trans_67x61, trans_67x61_desc);
}

/* 
 * is_transpose - This helper function checks if B is the transpose of
 *     A. You can check the correctness of your transpose by calling
 *     it before returning from the transpose function.
 */
int is_transpose(int M, int N, int A[N][M], int B[M][N])
{
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

