# CC = arm-linux-gnueabihf-gcc # selenall: compiler specifying flag
CC = gcc # selenall: compiler specifying flag
FILE1 = qsort_small.c
FILE2 = qsort_large.c

all: qsort_small qsort_large

# -static flag for BeagleBone execution
qsort_small: qsort_small.c Makefile
	$(CC) qsort_small.c -o qsort_small -O3 -lm 

qsort_large: qsort_large.c Makefile
	$(CC) qsort_large.c -o qsort_large -O3 -lm 

clean:
	rm -rf qsort_small qsort_large output*
