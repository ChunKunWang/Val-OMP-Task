#include "omp.h"
#include <stdio.h>

void foo( void );

int main()
{
	int a=0,i=0;
	float b=0.0;
	int j;
/*[][]*/
#pragma omp task //{{{ 
	{/*{}{}*/
	//{{
		j=a;
		//a=2;
		b=2.0;
		//j=a;
		for( i=0; i<10; i++ ) {
			j++;
		}
	
	}

	j=3;
	printf( "This is test5 program:\n" );
	printf( "a = %d, b = %f\n", a, b );
	printf( "i = %d, j = %d\n", i, j );

	return 0;
}
