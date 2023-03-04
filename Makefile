CFLAGS = -g -ansi -pedantic -Wall

ifeq ($(DEBUG), yes)
	CFLAGS += -O0
else
	CFLAGS += -O3
endif

CC = gcc
LD = gcc

all: hadrjson

hadrjson.o: hadrjson.c hadrjson.h

hadrjson: hadrjson.o
	$(LD) -o hadrjson $^

clean:
	rm -f hadrjson *.o
