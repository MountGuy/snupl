all: scanner
	./scanner snupl2.gm

test: scanner testcase.gm
	./scanner testcase.gm

scanner: main.c ebnf_util.c ebnf.c febnf.c
	gcc ebnf_util.c ebnf.c febnf.c main.c -o scanner -Wall

clean:
	rm scanner

count:
	@./count.sh
