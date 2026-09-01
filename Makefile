all: test

scanner: src
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	rm -rf scanner

test: scanner
	./scanner testcase.gm
	./scanner snupl1.gm
	./scanner snupl2.gm