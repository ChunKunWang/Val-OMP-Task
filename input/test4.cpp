#include "omp.h"
#include <stdio.h>

void foo( void );

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

	printf( "This is test4 program:\n" );
	printf( "a = %d, b = %d\n", a, b );

	foo();
	
	return 0;
}


void foo( void )
{
        int a=0,b=0;

        #pragma omp task
        a=1;

        #pragma omp task firstprivate(a,b)
        {
        a=2;
        b=2;
        }

        printf( "This is test4-foo program:\n" );
        printf( "a = %d, b = %d\n", a, b );
}


