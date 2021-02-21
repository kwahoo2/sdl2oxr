CC=g++

CFLAGS=-c -std=c++14 -Wall -I./common 

LDFLAGS = `pkg-config --libs glfw3` -lopenxr_loader -lSDL2 -lfmt -lpthread -lGL -lX11 -lassimp -lm -ldl

all: sdl2oxr 

sdl2oxr: sdl2oxr.o glad.o
	$(CC) sdl2oxr.o glad.o -o sdl2oxr $(LDFLAGS)

glad.o: common/glad.c
	$(CC) $(CFLAGS) common/glad.c
	
sdl2oxr.o: sdl2oxr.cpp
	$(CC) $(CFLAGS) sdl2oxr.cpp
	

clean:
	rm *o sdl2oxr
