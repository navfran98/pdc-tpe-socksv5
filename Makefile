# Carpetas donde guardamos los .H y .C
fileHeaders=headers
fileSources=src
clientFileSources=client

GCC = gcc

HFILES=$(shell find $(fileHeaders) -name '*.h' | sed 's/^.\///')

FILES = $(shell find $(fileSources) -name '*.c' | sed 's/^.\///')
CFILES = $(shell find $(clientFileSources) -name '*.c' | sed 's/^.\///')

OFILES=$(patsubst %.c,./%.o,$(FILES))
COFILES=$(patsubst %.c,./%.o,$(CFILES))

CFLAGS = -fsanitize=address -Wall -pthread -Wno-unused-parameter -D_GNU_SOURCE -Wextra -pedantic -g -std=c11 -D_POSIX_C_SOURCE=200112L

DEBUG_FLAGS = -Wall -Wextra -pedantic -pedantic-errors \
	-fsanitize=address -g -std=c11 -D_POSIX_C_SOURCE=200112L

%.o: %.c $(HFILES)
	$(GCC) -c -o $@ $< $(CFLAGS)

all: socks5d client

socks5d: $(OFILES)
	$(GCC) $(OFILES) $(CFLAGS) -o socks5d

client: $(COFILES)
	$(GCC) $(COFILES) $(CFLAGS) -o clients

# tests: 
# 	cd test; make all;

.PHONY: clean

clean: 
	rm -rf $(OFILES); rm -rf $(COFILES)

run:
	make clean; make all; ./socks5d -a santi:santi -u coco:coco;