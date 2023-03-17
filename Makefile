CFLAGS = -ansi -pedantic -Wall

ifeq ($(DEBUG), yes)
	CFLAGS += -O0 -g
else
	CFLAGS += -O0 -g
endif

CC = gcc
LD = gcc

all: test

hadrjson.o: hadrjson.c hadrjson.h

test: hadrjson.o test.o
	$(LD) -o test $^

clean:
	rm -f test *.o
