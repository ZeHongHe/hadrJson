CFLAGS = -g -ansi -pedantic -Wall

ifeq ($(DEBUG), yes)
	CFLAGS += -O0
else
	CFLAGS += -O0
endif

CC = gcc
LD = gcc

all: test

hadrjson.o: hadrjson.c hadrjson.h

test: hadrjson.o test.o
	$(LD) -o test $^

clean:
	rm -f test *.o
