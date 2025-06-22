# Makefile for SGI IRIX with MIPSpro 7.4
CC = cc
CFLAGS = -O3 -n32 -mips3
LIBS = -lGLw -lGL -lGLU -lXm -lXt -lX11 -lm -lpthread

all: particle_life

particle_life: particle_life.o simulation.o
	$(CC) $(CFLAGS) -o particle_life particle_life.o simulation.o $(LIBS)

particle_life.o: particle_life.c simulation.h
	$(CC) $(CFLAGS) -c particle_life.c

simulation.o: simulation.c simulation.h
	$(CC) $(CFLAGS) -c simulation.c

clean:
	rm -f *.o particle_life

.PHONY: all clean