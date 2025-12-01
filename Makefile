CC=gcc
CFLAGS=			\
	-Wall		\
	-Wextra		\
	-pedantic	\
	-std=c99	\
	-ggdb		\
	-Wswitch-enum

LIBS= -lm

build/polnoc: src/polnoc.c src/polnoc_lexer.c src/polnoc_lexer.c
	mkdir -p build
	$(CC) $(CFLAGS) -o $@ $^ $(LIBS)

clean:
	rm -rf build
