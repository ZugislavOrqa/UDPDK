#
# Created by leoll2 on 10/07/20.
# Copyright (c) 2020 Leonardo Lai. All rights reserved.
#

ROOTDIR=../..
DEPSDIR=${ROOTDIR}/deps

PC_FILE :=$(shell pkg-config --path libdpdk)
CFLAGS +=$(shell pkg-config --cflags libdpdk)
LDFLAGS_SHARED =$(shell pkg-config --libs libdpdk)
LDFLAGS_STATIC =$(shell pkg-config --static --libs libdpdk)

ifeq ($(RTE_TARGET),)
$(error "Please define RTE_TARGET environment variable")
endif

ifeq ($(UDPDK_PATH),)
	UDPDK_PATH=${ROOTDIR}
endif

# all source are stored in SRCS-y
SRCS= main.c

LIBS+= -L${UDPDK_PATH}/udpdk -Wl,--whole-archive,-ludpdk,--no-whole-archive
LIBS+= -Wl,--no-whole-archive -lrt -lm -ldl -lcrypto -pthread -lnuma

CFLAGS += $(WERROR_FLAGS) -O3

TARGET="pktgen"
all: $(SRCS) Makefile $(PC_FILE) | build
	cc $(CFLAGS) -I${ROOTDIR}/udpdk -I${DEPSDIR}/dpdk/${RTE_TARGET}/include -o ${TARGET} ${SRCS} $(LDFLAGS) $(LDFLAGS_STATIC) ${LIBS} 

.PHONY: clean
clean:
	rm -f *.o ${TARGET}
