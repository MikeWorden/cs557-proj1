IDIR =./include
SRCDIR =./src
CC=gcc
CXX=g++
CFLAGS=-I$(IDIR) -g -Wall -fno-inline
CXXFLAGS=-I$(SRCDIR) -I$(IDIR) -g -Wall -fno-inline
FLAGS=${CXXFLAGS} ${AC_DEFS}


VPATH=./src
ODIR=obj
LDIR =./lib

LIBS=-lm

_DEPS = manager.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = proj1.o client.o init.o manager.o tracker.o utility.o timers.o timers-c.o tools.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:  tools.o timers.o timers-c.o proj1

.PHONY:  all 

	gcc -o $@ $^ $(CFLAGS) $(LIBS)

tools.o:  src/tools.cc src/tools.hh
	$(CXX) $(FLAGS) -c src/tools.cc -o obj/tools.o

timers.o: timers.cc timers.hh
	$(CXX) $(FLAGS) -c src/timers.cc -o obj/timers.o

timers-c.o: timers-c.cc timers-c.h
	$(CXX) $(FLAGS) -c src/timers-c.cc -o obj/timers-c.o

proj1: $(OBJ)
	g++ -o $@ $^ $(CFLAGS) $(LIBS)
.PHONY: clean

clean:
	rm -f *.o $(ODIR)/*.o *~ core $(INCDIR)/*~ 
