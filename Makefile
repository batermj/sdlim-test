# Makefile to build the SDL tests

srcdir  = .

CC      = gcc
EXE	= 
CFLAGS  = -g -O2 -I/usr/local/include/SDL -D_GNU_SOURCE=1 -D_REENTRANT -DHAVE_OPENGL
LIBS	=  -L/usr/local/lib -Wl,-rpath,/usr/local/lib -lSDL -lpthread -lSDL_ttf

TARGETS = sdlim-demo$(EXE)

all: Makefile $(TARGETS)

sdlim-demo$(EXE): sdlim-demo.c
	$(CC) -o $@ $? $(CFLAGS) $(LIBS)

clean:
	rm -f $(TARGETS)

distclean: clean
	
