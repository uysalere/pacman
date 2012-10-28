OBJS = Pacman.o Timer.o
CC = g++
DEBUG = -g
CFLAGS = -Wall -c $(DEBUG)
LFLAGS = -Wall -L/usr/include/X11 -lGL -lGLU -lglut -lm $(DEBUG)

pacman : $(OBJS)
	$(CC) $(OBJS) -o pacman $(LFLAGS)

Pacman.o : Timer.h
	$(CC) $(CFLAGS) Pacman.c $(LFLAGS)

Timer.o : Timer.c Timer.h
	$(CC) $(CFLAGS) Timer.c $(LFLAGS)

clean:
	\rm *.o *~ pacman
