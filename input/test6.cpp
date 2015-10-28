#include "omp.h"
#include <stdio.h>

void foo( int );

int main()
{
	int a=0,b=0,i=1;

	#pragma omp task
	a=1;

	#pragma omp task firstprivate(a,b) 
	{
	a=2;
	b=2;
	foo( i );
	}

	printf( "This is test6 program:\n" );
	printf( "a = %d, b = %d\n", a, b );
	printf( "i = %d", i );	
	return 0;
}


void foo( int d )
{
        int a=0,b=0;

        #pragma omp task
        a=1;

        #pragma omp task firstprivate(a,b)
        {
        a=2;
        b=2;
        }

        printf( "This is test6-foo program:\n" );
        printf( "a = %d, b = %d\n", a, b );
}


