.PHONY: clean all
CC=gcc
CFLAGS+=-Wall -O3

all: libmacsim.a

batch-means.o: batch-means.c
	$(CC) -c $(CFLAGS) $(LDFLAGS) $< -lm

libmacsim.a: heap.o linked-list.o debug.o hash-table.o random.o batch-means.o macsim.o
	$(AR) rcs $@ $^

tags:
	ctags-exhuberant *

clean:
	rm -f *.o
	rm -f tags
	rm -f libmacsim.a

