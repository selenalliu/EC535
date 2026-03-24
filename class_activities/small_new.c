//in-class exercise, EC535
// Selena Liu (selenall)

#include "stdio.h"

int two(int limit)
{
	int a, i;
	a = 0;
	for (i=0; i<limit; i++)
		a += i;
}

// new function
int one(int limit)
{
	int i, a[50];
	
	for (i=0; i<limit; i++) {
        if (i % 50 == 49) // added condition
            a[i % 50]= i + two(i);
    }
	return a[49];
}

int main() 
{
	int j, a;
	a = 0;
	for (j=0; j<1000; j++)
		a = a + one(j);
	printf("The result is %d\n", a);
	return 0;
}