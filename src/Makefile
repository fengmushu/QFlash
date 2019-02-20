CC?=gcc
CPP?=g++

all: linux

C_FLAGS:= -DUSE_FASTBOOT -DFIREHOSE_ENABLE
C_SOURCE:= fastboot/protocol.c \
			fastboot/engine.c \
			fastboot/fastboot.c \
			fastboot/usb_linux_fastboot.c \
			fastboot/util_linux.c \
			firehose/qfirehose.c \
			firehose/sahara_protocol.c \
			firehose/firehose_protocol.c \
			firehose/usb_linux_firehose.c
	
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
	$(CPP) -g -c $(C_FLAGS) $(CPP_SOURCE)
	$(CPP) *.o -lrt -lpthread -o QFlash

clean:
	rm -rf $(NDK_OUT) libs *.o QFlash *~
	make -C fastboot clean
	make -C firehose clean
