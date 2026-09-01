scanner: src
	$(MAKE) -C src scanner

clean:
	$(MAKE) -C src clean
	rm -rf scanner