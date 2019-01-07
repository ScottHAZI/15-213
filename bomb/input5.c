#include <stdio.h>

int main(void)
{
	// desired string for phase_5 is flyers
	// table entries: 9 15 14 5 6 7
	char a = 0x60;
	//putchar(a);
	char s[] = {a+9, a+15, a+14, a+5, a+6, a+7, '\0'};
	//puts(s);
	printf("%s\n", s);
}
