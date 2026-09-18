.PHONY: all clean test scanner

all: test

scanner:
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	@rm -rf scanner output.txt
	@rm -rf t.*

test: scanner
	./scanner grammar.gm test/array01.mod

simple: scanner
	./scanner simple.gm simple.mod

rtest: scanner
	python3 gen_pair.py
	./scanner t.gm t.mod > t.output

rerun: scanner
	./scanner t.gm t.mod > t.output
