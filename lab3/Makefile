all:
	make -f makefiles/Makefile.bootloader
	make -f makefiles/Makefile.kernel

bootloader:
	make -f makefiles/Makefile.bootloader

kernel:
	make -f makefiles/Makefile.kernel

qemu:
	make -f makefiles/Makefile.qemu run

test:
	make -f makefiles/Makefile.test

clean:
	rm -rf build
	rm *.fit
