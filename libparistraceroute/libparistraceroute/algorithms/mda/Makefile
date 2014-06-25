#rules for compile individual .o files
all: bound link1

binsearch: binsearchBound link2

bound:
	gcc -c -Wall -g bound.c

link1:
	gcc -g ../../vector.o bound.o -o bound -lm

binsearchBound:
	gcc -c -Wall -g bin_search_bound.c

link2:
	gcc -g bin_search_bound.o -o bin_search_bound -lm

#rules for cleaning
cleanWorkshop:
	rm -f bound.o bound

cleanBinsearch:
	rm -rf bin_search_bound.o bin_search_bound

cleanAll:
	rm -f *.o bound bin_search_bound
