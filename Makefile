.PHONY: all clean test scanner

all: test

scanner:
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	@rm -rf scanner

test: scanner
	@echo
	./scanner snupl2.gm generated.tc
# 	@echo
# 	./scanner snupl2.gm invalid.tc