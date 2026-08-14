all:
	make -f makefiles/Makefile.bootloader
	bear -- make -f makefiles/Makefile.kernel

bootloader:
	make -f makefiles/Makefile.bootloader

kernel:
	bear -- make -f makefiles/Makefile.kernel

qemu:
	make -f makefiles/Makefile.qemu run

test:
	make -f makefiles/Makefile.test

clean:
	rm -rf build
	rm *.fit
