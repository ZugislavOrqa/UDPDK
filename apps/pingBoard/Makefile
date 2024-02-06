#
# Created by leoll2 on 9/25/20.
# Copyright (c) 2020 Leonardo Lai. All rights reserved.
#

ROOTDIR=../..
DEPSDIR=${ROOTDIR}/deps

ifeq ($(RTE_TARGET),)
$(error "Please define RTE_TARGET environment variable")
endif

ifeq ($(UDPDK_PATH),)
	UDPDK_PATH=${ROOTDIR}
endif

ORQA_PKGCONF := pkg-config
ORQA_UDPDK := ../../udpdk
ORQA_CFLAGS := -O3 $(shell $(ORQA_PKGCONF) --cflags libdpdk)
ORQA_LFLAGS := -lpthread -lm -L$(ORQA_UDPDK) -Wl,--whole-archive,-ludpdk,--no-whole-archive $(shell $(ORQA_PKGCONF) --libs libdpdk) 

# all source are stored in SRCS-y
SRCS= main.c

LIBS+= -L${UDPDK_PATH}/udpdk -Wl,--whole-archive,-ludpdk,--no-whole-archive
LIBS+= $(shell $(ORQA_PKGCONF) --libs libdpdk)
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

CFLAGS += $(WERROR_FLAGS) -O3 $(shell $(ORQA_PKGCONF) --cflags libdpdk)

TARGET="pingBoard"
all:
	cc -I${ROOTDIR}/udpdk -I${DEPSDIR}/dpdk/${RTE_TARGET}/include -o ${TARGET} ${SRCS} ${LIBS}

.PHONY: clean
clean:
	rm -f *.o ${TARGET}
