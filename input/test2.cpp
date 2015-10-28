#include "omp.h"
#include <stdio.h>

int main()
{
	int a=0, b=0;

	#pragma omp task
	a=b+1;

	#pragma omp task
	b=1;

	printf( "This is test2 program:\n" );
	printf( "a = %d\n, b = %d\n", a, b );

	return 0;
}

