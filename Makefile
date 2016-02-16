all: mutexFinal SemaphoreCode

mutexFinal: mutexFinal.c
	gcc mutexFinal.c -pthread -o mutexFinal

SemaphoreCode: SemaphoreCode.c
	gcc SemaphoreCode.c -pthread -o SemaphoreCode

clean:
	-rm mutexFinal
	-rm SemaphoreCode
	-rm Makefile~
	-rm *.c~
