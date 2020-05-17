CFLAGS = 
TESTCASE = 0.basic.nel

tomasulo: main.o rs.o rrs.o inst.o lb.o
	g++ $(CFLAGS) -o tomasulo main.o rs.o rrs.o inst.o lb.o
main.o: main.cpp 
	g++ $(CFLAGS) -o main.o -c main.cpp
rs.o: rs.cpp rs.h
	g++ $(CFLAGS) -o rs.o -c rs.cpp
rrs.o: rrs.cpp rrs.h
	g++ $(CFLAGS) -o rrs.o -c rrs.cpp
inst.o: inst.cpp inst.h
	g++ $(CFLAGS) -o inst.o -c inst.cpp
lb.o: lb.cpp lb.h
	g++ $(CFLAGS) -o lb.o -c lb.cpp
run:
	make
	./tomasulo TestCase/$(TESTCASE)
	make clean
clean:
	rm *.o tomasulo