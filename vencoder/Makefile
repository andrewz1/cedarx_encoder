CC = gcc
CPPFLAGS = -Iinclude -I../common/include -I../util
DEP_CFLAGS = -MMD -MP -MF $(@:%.o=%.d) -MT $(@:%.o=%.d)
CFLAGS = -pthread -Wall -O3 -fpic -fvisibility=hidden -fomit-frame-pointer
LDFLAGS = -shared -Wl,-soname,$(BINS)
LOADLIBES = -L../base -L../common
LDLIBS = -lcedar_base -lcedar_common

BINS = libcedar_vencoder.so
SRCS = BitstreamManager.c EncAdapter.c FrameBufferManager.c venc_device.c vencoder.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)

.PHONY: all clean

all: $(OBJS) $(BINS)

$(BINS): $(OBJS)
	$(CC) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) $(BINS) $(OBJS) $(DEPS)

include $(wildcard $(DEPS))
