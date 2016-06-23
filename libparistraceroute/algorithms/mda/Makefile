#rules for compile individual .o files
all: bound.o ../../vector.o
	gcc -g ../../vector.o bound.o -o bound -lm

bound.o: bound.c
	gcc -c -Wall -g bound.c

#rules for cleaning
clean:
	rm -f bound.o bound
