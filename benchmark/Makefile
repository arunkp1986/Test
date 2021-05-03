all:lru
CC = gcc
SRCS = murmur3.c lru_impl.c
CFLAGS = -g
OBJS = lru_impl.o murmur3.o
DEPS = murmur3.h lru_impl.h redo.h
LIBS = -lm

%o: %c $(DEPS)
	$(CC) $(CFLAGS) -c $< -o $@

lru : $(OBJS)
	$(CC) -o $@ $(OBJS) $(LIBS)

.PHONY: clean

clean:
	rm -f *.o lru
