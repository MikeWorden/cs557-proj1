IDIR =./include
CC=gcc
CFLAGS=-I$(IDIR) -pthread
VPATH=./src
SRCDIR=./src
ODIR=obj
LDIR =./lib

LIBS=-lm

_DEPS = manager.h
DEPS = $(patsubst %,$(IDIR)/%,$(_DEPS))

_OBJ = proj1.o client.o init.o manager.o tracker.o utility.o
OBJ = $(patsubst %,$(ODIR)/%,$(_OBJ))


$(ODIR)/%.o: %.c $(DEPS)
	$(CC) -c -o $@ $< $(CFLAGS)

all:  proj1

.PHONY:  all

proj1: $(OBJ)
	gcc -o $@ $^ $(CFLAGS) $(LIBS)

.PHONY: clean

clean:
	rm -f $(ODIR)/*.o *~ core $(INCDIR)/*~ 
