CC = gcc
CFLAGS = -Wall
TARGET = scanner
SRCS = main.c ebnf.c ebnf_util.c analysis.c lexer.c
OBJS = $(SRCS:.c=.o)
COMMON = common.h

all: $(TARGET)

$(TARGET): $(OBJS) $(COMMON)
	$(CC) $(CFLAGS) -o ../$@ $^

%.o: %.c %.h $(COMMON)
	$(CC) $(CFLAGS) -c -g -O0 -o $@ $<

main.h:
	@echo

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET) testcase.gm
	./$(TARGET) testcase.gm