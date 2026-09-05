.PHONY: all clean test scanner

all: scanner

scanner:
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	@rm -rf scanner output.txt

pl: scanner
	./scanner snupl2.gm generated.tc
inv: scanner
	./scanner snupl2.gm invalid.tc
