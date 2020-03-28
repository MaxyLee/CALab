cache: main.o cache.o
	g++ -o cache main.o cache.o
main.o: main.cpp cache.h
	g++ -o main.o -c main.cpp
cache.o: cache.cpp cache.h
	g++ -o cache.o -c cache.cpp
run:
	make
	./cache
clean:
	rm *.o cache