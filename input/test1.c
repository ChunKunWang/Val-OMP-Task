#include "omp.h"
#include <stdio.h>

int main()
{
	int a=0;

	#	pragma   omp	 task 
	a++;

	printf( "This is test1 program:\n" );
	printf( "a = %d\n", a );

	return 0;
}

