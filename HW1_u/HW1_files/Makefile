objects = main.o bits.o

BitApp: $(objects)
	gcc -o BitApp $(objects)

main.o: main.c bits.h
	gcc -c main.c

bits.o: bits.c bits.h
	gcc -c bits.c

.PHONY: clean
clean:
	-rm BitApp $(objects)