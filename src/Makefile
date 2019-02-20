
all: linux

linux:
	rm -rf *.o
	$(CC) -g -c -DUSE_FASTBOOT fastboot/protocol.c fastboot/engine.c fastboot/fastboot.c fastboot/usb_linux.c fastboot/util_linux.c
	$(CC) -g -c download.cpp file.cpp os_linux.cpp qcn.cpp serialif.cpp
	$(CC) *.o -lrt -o QFlash

clean:
	rm -rf $(NDK_OUT) libs *.o QFlash *~
	make -C fastboot clean
