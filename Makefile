all: scanner
	./scanner snupl1.gm

test: scanner testcase.gm
	./scanner testcase.gm

scanner: main.c lexer.c ebnf.c
	gcc lexer.c ebnf.c main.c -o scanner

clean:
	rm scanner