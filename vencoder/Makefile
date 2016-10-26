CC = gcc
CPPFLAGS = -Iinclude -I../common/include -I../util
DEP_CFLAGS = -MD -MP -MQ $@
CFLAGS = -pthread -Wall -O3 -fpic -fvisibility=hidden -fomit-frame-pointer
LDFLAGS = -shared -Wl,-soname,$(BINS)
LOADLIBES = -L../base -L../common
LDLIBS = -lcedar_base -lcedar_common

SRCS = BitstreamManager.c EncAdapter.c FrameBufferManager.c venc_device.c vencoder.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
BINS = libcedar_vencoder.so

.PHONY: all clean

all: $(OBJS) $(BINS)

$(BINS): $(OBJS)
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) $(BINS) $(OBJS) $(DEPS)

include $(wildcard $(DEPS))
