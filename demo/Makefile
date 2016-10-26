CC = gcc
CFLAGS = -Wall -O3 -fPIC -pthread
CPPFLAGS = -I../vencoder/include

LDFLAGS =
LOADLIBES = -L/usr/lib/cedarx
LDLIBS = -lcedar_vencoder -ldl

SRCS = VideoTest.c
OBJS = $(SRCS:.c=.o)
BINS = VideoTest

.PHONY: all clean

all: clean $(OBJS) $(BINS)

$(BINS): $(OBJS)
	$(LINK.c) $^ $(LOADLIBES) $(LDLIBS) -o $@

clean:
	$(RM) $(BINS) $(OBJS)
