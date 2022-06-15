# Carpetas donde guardamos los .H y .C
fileHeaders=headers
fileSources=src

GCC = gcc

HFILES=$(shell find $(fileHeaders) -name '*.h' | sed 's/^.\///')

# Si queremos ignorar el client.c hagan esto
# FILES= $(filter-out ./src/client.c, $(shell find $(fileSources) -name '*.c' | sed 's/^.\///'))

FILES = $(shell find $(fileSources) -name '*.c' | sed 's/^.\///')

OFILES=$(patsubst %.c,./%.o,$(FILES))

CFLAGS = -fsanitize=address -Wall -pthread -Wno-unused-parameter -D_GNU_SOURCE -Wextra -pedantic -g -std=c11 -D_POSIX_C_SOURCE=200112L

DEBUG_FLAGS = -Wall -Wextra -pedantic -pedantic-errors \
	-fsanitize=address -g -std=c11 -D_POSIX_C_SOURCE=200112L

%.o: %.c $(HFILES)
	$(GCC) -c -o $@ $< $(CFLAGS)

all: socks5d

socks5d: $(OFILES)
	$(GCC) $(OFILES) $(CFLAGS) -o socks5d

# tests: 
# 	cd test; make all;

.PHONY: clean

clean: 
	rm -rf $(OFILES)