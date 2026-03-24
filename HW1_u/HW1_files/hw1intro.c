#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <pthread.h>
 
 void *myFirstThread(void *p)
 {
     	sleep(1);
     	printf("I am the first thread \n");
     	return NULL;
}

void *mySecondThread(void *p)
{ 
     	printf("I am the second thread \n");
     	pthread_yield();
}

void *myNewThread(void *p)
{
	printf("Hello!\n");
	//Ask user to enter an integer
	printf("Please enter an integer: ");
	int num;
	scanf("%d", &num);
	//print the integer in decimal, binary, and in hexadecimal form
	printf("Your number in decimal form: %d\n", num);
	
	printf("Your number in binary form: ");
	
	float bits_f = log2(num);
	float bits = ceil(bits_f);
	
	if (num == 0) {
		printf("0");
	} else {
		for (int i = bits; i >= 0; i--) {
			int num_shift = num >> i;

			if (num_shift & 1 == 1)
				printf("1");
			else
				printf("0"); 
		}
	}
	printf("\n");

	printf("Your number in hexadecimal form: %x\n", num);
}

int main() 
{
	pthread_t tid,tid2, tid3;
	
	printf("Program started...\n");
	
	pthread_create(&tid, NULL, myFirstThread, NULL);
	pthread_create(&tid2, NULL, mySecondThread, NULL);
	pthread_create(&tid3, NULL, myNewThread, NULL);
	
	pthread_join(tid, NULL);
	pthread_join(tid2, NULL);
	pthread_join(tid3, NULL);
	
	printf("All threads done.\n");
       	exit(0);
}
