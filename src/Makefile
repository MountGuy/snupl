CC = gcc
CFLAGS = -Wall -g -O0
TARGET = scanner
SRCS = arena.c main.c character.c dump.c meta.c log.c lexer.c
OBJS = $(SRCS:.c=.o)

COMMON = common.h struct.h

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o ../$@ $^

%.o: %.c $(COMMON)
	$(CC) $(CFLAGS) -c -o $@ $<

main.o: main.c $(COMMON)

clean:
	rm -f $(OBJS) ../$(TARGET)
