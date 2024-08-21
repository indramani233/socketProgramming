library:mysocket.o
	ar -rcs libmsocket.a mysocket.o
mysocket.o: mysocket.h mysocket.c
	gcc -c mysocket.c -lpthread
clean: 
	rm -f mysocket.o libmsocket.a

