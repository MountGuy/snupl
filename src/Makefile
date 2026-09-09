CC = gcc
CFLAGS = -Wall
TARGET = scanner
SRCS = arena.c main.c character.c dump.c meta.c log.c
OBJS = $(SRCS:.c=.o)
COMMON = common.h struct.h

all: $(TARGET)

$(TARGET): $(OBJS) $(COMMON)
	$(CC) $(CFLAGS) -o ../$@ $^

%.o: %.c %.h $(COMMON)
	$(CC) $(CFLAGS) -c -g -O0 -o $@ $<

main.h:
	@echo

clean:
	rm -f $(OBJS) $(TARGET)

