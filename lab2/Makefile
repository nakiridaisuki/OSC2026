all:
	make -f Makefile.bootloader
	make -f Makefile.kernel

bootloader:
	make -f Makefile.bootloader

kernel:
	make -f Makefile.kernel

test:
	make -f Makefile.test

run:
	make run -f Makefile.kernel

clean:
	rm -rf build
	rm *.fit
