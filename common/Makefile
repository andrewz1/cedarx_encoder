CC = gcc
CPPFLAGS = -I.. -Iinclude
DEP_CFLAGS = -MD -MP -MQ $@
CFLAGS = -pthread -Wall -O3 -fPIC
LDFLAGS = -shared -Wl,-soname,$(BINS)
LOADLIBES = -L../base
LDLIBS = -lcedar_base

SRCS = ve.c ve_alloc.c ion_alloc.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
BINS = libcedar_common.so

.PHONY: all clean

all: $(OBJS) $(BINS)

$(BINS): $(OBJS)
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) $(BINS) $(OBJS) $(DEPS)

include $(wildcard $(DEPS))
