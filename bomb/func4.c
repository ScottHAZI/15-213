#include <stdio.h>

int func4(int x, int y, int z)
{
	// x : rdi, y : rsi, z : rdx
	// y = 0 and z = 5 at the beginning
	int result;
	result = z;
	result -= y;
	int temp = result;
	temp >>= 31; // logical shift
	result += temp;
	result >>= 1; // arithmetic shift
	temp = result + y;
	if (x >= temp) {
		result = 0;
		if (x <= temp) {
			return result;
		}
		y = temp + 1;
		result = func4(x,y,z);
		result = 1 + result*2;
		return result;
	}

	z = temp - 1;
	result = func4(x,y,z);
	result *= 2;
	return result;
}

int main(void)
{
	int i = 1;
	for (i=0; i<=14; i++) {
		printf("%d func4 %d\n", i, func4(i, 0, 14));
		if (!func4(i, 0, 14)) printf("%d\n", i);
	}
	return 0;
}
