all:
	make -f Makefile.bootloader
	make -f Makefile.kernel

bootloader:
	make -f Makefile.bootloader

kernel:
	make -f Makefile.kernel

qemu:
	make -f Makefile.qemu run

test:
	make -f Makefile.test

clean:
	rm -rf build
	rm *.fit
