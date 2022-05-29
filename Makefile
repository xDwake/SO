all: server client

server: bin/sdstored

client: bin/sdstore

INCDIR: includes

bin/sdstored: obj/sdstored.o

	gcc -g obj/sdstored.o -o bin/sdstored

obj/sdstored.o: src/sdstored.c INCDIR
	
	gcc -Wall -g -c -I INCDIR src/sdstored.c -o obj/sdstored.o
	
bin/sdstore: obj/sdstore.o
	
	gcc -g obj/sdstore.o -o bin/sdstore

obj/sdstore.o: src/sdstore.c INCDIR

	gcc -Wall -g -c -I INCDIR src/sdstore.c -o obj/sdstore.o

clean:
	
	rm obj/* tmp/* bin/sdstore bin/sdstored 