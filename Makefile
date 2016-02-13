all: mutex

mutex: mutex.c
	gcc mutex.c -pthread -o mutex

clean:
	-rm mutex
	-rm Makefile~
	-rm *.c~
