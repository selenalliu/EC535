# Makefile for the ktimer userspace program

# specify cross compiler
CC = arm-linux-gnueabihf-gcc
# for stripping the executable
STRIP = arm-linux-gnueabihf-strip

SRC = ktimer.o

# static flag because it doesn't work in QEMU without it
ktimer: $(SRC)
	$(CC) -static -o ktimer $(SRC)
	$(STRIP) ktimer

.PHONY: clean
clean:
	-rm ktimer $(SRC)