CC=gcc
CFLAGS=-I.

all : parallel_min_max process_memory zombie psum

parallel_min_max : utils.o find_min_max.o utils.h find_min_max.h parallel_min_max.c
	$(CC) -o parallel_min_max utils.o find_min_max.o parallel_min_max.c $(CFLAGS)

zombie : zombie.c
	$(CC) -o zombie zombie.c $(CFLAGS)

process_memory : process_memory.c
	$(CC) -w -o process_memory process_memory.c $(CFLAGS)

libsum.a : sum.o
	ar rcs libsum.a sum.o

psum : libsum.a utils.o parallel_sum.c utils.h sum.h
	$(CC) -o psum parallel_sum.c utils.o -L. -lsum -pthread $(CFLAGS)

utils.o : utils.c utils.h
	$(CC) -o utils.o -c utils.c $(CFLAGS)

find_min_max.o : find_min_max.c find_min_max.h utils.h
	$(CC) -o find_min_max.o -c find_min_max.c $(CFLAGS)

sum.o : sum.c sum.h
	$(CC) -o sum.o -c sum.c $(CFLAGS)

clean :
	rm -f utils.o find_min_max.o sum.o libsum.a parallel_min_max \
	      parallel_min_max_slow process_memory zombie psum shared_data.txt
