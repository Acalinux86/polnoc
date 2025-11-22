CC=gcc
CFLAGS=			\
	-Wall		\
	-Werror		\
	-Wextra		\
	-pedantic	\
	-std=c99	\
	-ggdb		\
	-Wswitch-enum

LIBS= -lm

build/polnoc: polnoc.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $< $(LIBS)

clean:
	rm -rf build
