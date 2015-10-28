#include <stdio.h>

int main()
{
	int i=0;

	for( i=0; i<1; i++ )
	{
	
		printf( "For loop begin.\n" );
		for( i=0; i<1; i++ )
		{
			printf( "For loop in.\n" );
		}
		printf( "For loop end.\n" );
	}

	return 0;
}

