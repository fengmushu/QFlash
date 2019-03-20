CC?=gcc
CPP?=g++

all: linux

C_FLAGS:= -DUSE_FASTBOOT
C_SOURCE:= \
		fastboot/protocol.c \
		fastboot/engine.c \
		fastboot/fastboot.c \
		fastboot/usb_linux.c \
		fastboot/util_linux.c
	
CPP_SOURCE:= \
		tinystr.cpp \
		tinyxml.cpp \
		tinyxmlerror.cpp \
		tinyxmlparser.cpp \
		md5.cpp \
		at_tok.cpp \
		atchannel.cpp \
		ril-daemon.cpp \
		download.cpp \
		file.cpp \
		os_linux.cpp \
		serialif.cpp \
		quectel_log.cpp \
		quectel_common.cpp \
		quectel_crc.cpp

linux:
	rm -rf *.o
	$(CC) -g -c $(C_FLAGS) $(C_SOURCE)
	$(CXX) -g -c  -DMD5_CHECK $(C_FLAGS) $(CPP_SOURCE)
	$(CXX) *.o -lrt -lpthread -o QFlash

clean:
	rm -rf $(NDK_OUT) libs *.o QFlash *~
	make -C fastboot clean
