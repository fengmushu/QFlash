# Copyright 2006 The Android Open Source Project

LOCAL_PATH:= $(call my-dir)
include $(CLEAR_VARS)

include $(CLEAR_VARS)
LOCAL_SRC_FILES:= download.cpp file.cpp os_linux.cpp qcn.cpp serialif.cpp
LOCAL_SRC_FILES+= tinystr.cpp tinyxml.cpp tinyxmlerror.cpp tinyxmlparser.cpp
LOCAL_SRC_FILES+= fastboot/protocol.c fastboot/engine.c fastboot/fastboot.c fastboot/usb_linux.c fastboot/util_linux.c
LOCAL_CFLAGS += -DUSE_FASTBOOT
LOCAL_CFLAGS += -pie -fPIE
LOCAL_LDFLAGS += -pie -fPIE
LOCAL_MODULE_TAGS:= optional
LOCAL_MODULE:= QEmbedUPG_EC20
include $(BUILD_EXECUTABLE)
