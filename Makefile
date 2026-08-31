all: scanner
	./scanner snupl1.gm
# 	./scanner testcase.gm

test: scanner testcase.gm
	./scanner testcase.gm

scanner: main.c ebnf_util.c ebnf.c
	gcc ebnf_util.c ebnf.c main.c -o scanner

clean:
	rm scanner