#include "omp.h"
#include <stdio.h>

int main()
{
	int a=0,i=1;

	#pragma omp task 
	{
	i=2;
	a=i+1;
	}

	printf( "This is test1 program:\n" );
	printf( "a = %d\n", a );
	printf( "i = %d\n", i );

	return 0;
}

