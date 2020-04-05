CFLAGS = 
TRACE = trace/astar.trace
LINE_SIZE = 64
WAYS = 8
RP = LRU
WP = write_back_allocate

cache: main.o cache.o
	g++ -o cache main.o cache.o
main.o: main.cpp cache.h
	g++ -o main.o -c main.cpp
cache.o: cache.cpp cache.h
	g++ ${CFLAGS} -o cache.o -c cache.cpp
run:
	make
	./cache ${TRACE} ${LINE_SIZE} ${WAYS} ${RP} ${WP}
	make clean
clean:
	rm *.o cache