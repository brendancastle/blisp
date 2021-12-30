all: parsing

CC=cc
CFLAGS=--std=c99 -Wall
LIBS=-ledit -lm

parsing: parsing.c mpc.c
	$(CC) $(CFLAGS) $(LIBS) -o $@ $^

clean:
	rm -f parsing
