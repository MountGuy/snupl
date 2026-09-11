.PHONY: all clean test scanner

all: test

scanner:
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	@rm -rf scanner output.txt

test: scanner
	./scanner grammar.gm test/array01.mod

