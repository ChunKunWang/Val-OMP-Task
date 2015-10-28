#include "omp.h"
#include <stdio.h>

int main()
{
	int a=0,b=0;

	#pragma omp task
	a=1;

	#pragma omp task firstprivate(a,b) 
	{
	a=2;
	b=2;
	}

	printf( "This is test3 program:\n" );
	printf( "a = %d\n, b = %d\n", a, b );

	return 0;
}

