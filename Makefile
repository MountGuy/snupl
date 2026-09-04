.PHONY: all clean test scanner

all: scanner

scanner:
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	@rm -rf scanner output.txt

simple: scanner
	./scanner simple.gm simple.tc
pl: scanner
	./scanner snupl2.gm generated.tc
inv: scanner
	./scanner snupl2.gm invalid.tc
