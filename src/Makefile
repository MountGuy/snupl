CC = gcc
CFLAGS = "-Wall"
TARGET = scanner
SRCS = main.c ebnf.c ebnf_util.c
OBJS = $(SRCS:.c=.o)

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) $(CFLAGS) -o ../$@ $^

%.o: %.c
	$(CC) $(CFLAGS) -c -o $@ $<

clean:
	rm -f $(OBJS) $(TARGET)

test: $(TARGET) testcase.gm
	./$(TARGET) testcase.gm