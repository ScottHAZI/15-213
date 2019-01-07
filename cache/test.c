#include <stdio.h>

#define NN 67
#define MM 61

extern void trans_64x64_8x8_4_4x4(int M, int N, int A[N][M], int B[N][M]);
extern void trans_67x61_4x4(int M, int N, int A[N][M], int B[N][M]);

int main(void)
{
	int a[NN][MM], b[MM][NN];
	
	for (int i = 0; i < NN; i++)
		for (int j = 0; j < MM; j++)
			a[i][j] = MM * i + j;
	
	//trans_64x64_8x8_4_4x4(M, N, a, b);
	trans_67x61_4x4(MM, NN, a, b);
	for (int i = 0; i < MM; i++)
		printf("%d ", b[1][i]);

	printf("\n");
}
