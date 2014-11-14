output: thread1_prev.o my402list.o
	gcc -o output -g -lm -pthread thread1_prev.o my402list.o

thread1_prev.o: thread1_prev.c my402list.h shared.h
	gcc -g -c Wall thread1_prev.c 

my402list.o: my402list.c my402list.h
	gcc -g -c -Wall my402list.c 

clean:
	rm -f *.o output