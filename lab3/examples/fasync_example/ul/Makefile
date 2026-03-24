# EC535 
# Makefile for fasync_tester (userland program)

# Makefile adapted from http://www.cs.colby.edu/maxwell/courses/tutorials/maketutor/

CC = arm-linux-gnueabihf-gcc
CFLAGS = -std=c99 -Wall -I. -static -O0
DEPS = 
OBJ = fasync_tester.o

%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

fasync_tester: $(OBJ)
	$(CC) -o $@ $^ $(CFLAGS)
	
clean:
	rm *.o fasync_tester
