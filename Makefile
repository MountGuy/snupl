all: scanner
# 	./scanner snupl1.gm > output.txt
	./scanner snupl1.gm

scanner: main.c lexer.c ebnf.c
	gcc lexer.c ebnf.c main.c -o scanner

clean:
	rm scanner