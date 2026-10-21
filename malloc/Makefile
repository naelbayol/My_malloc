CC = gcc
CPPFLAGS = -D_DEFAULT_SOURCE
CFLAGS = -std=c99 -pedantic -Werror -Wall -Wextra -Wvla -fvisibility=hidden -fPIC
LDFLAGS = -shared -Wl,--no-undefined
LDLIBS = -lpthread

LIB = libmalloc.so
OBJS = src/malloc.o src/auxi.o

all: library

library: $(LIB)

$(LIB): $(OBJS)
	$(CC) $(LDFLAGS) -o $@ $^ $(LDLIBS)

$(OBJS): src/malloc.h

debug: CFLAGS += -g
debug: clean library

check: library
	bash tests/testsuite.sh ./$(LIB)

clean:
	$(RM) $(OBJS) $(LIB)

.PHONY: all library debug check clean
