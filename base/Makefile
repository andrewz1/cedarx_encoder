CC = gcc
CPPFLAGS =
DEP_CFLAGS = -MD -MP -MQ $@
CFLAGS = -pthread -Wall -O3 -fPIC
LDFLAGS = -shared -Wl,-soname,$(BINS)
LOADLIBES =
LDLIBS =

SRCS = CdxUtils.c
OBJS = $(SRCS:.c=.o)
DEPS = $(SRCS:.c=.d)
BINS = libcedar_base.so

.PHONY: all clean

all: $(OBJS) $(BINS)

$(BINS): $(OBJS)
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) $(LDFLAGS) $^ $(LOADLIBES) $(LDLIBS) -o $@

%.o: %.c
	$(CC) $(DEP_CFLAGS) $(CFLAGS) $(CPPFLAGS) -c -o $@ $<

clean:
	$(RM) $(BINS) $(OBJS) $(DEPS)

include $(wildcard $(DEPS))
