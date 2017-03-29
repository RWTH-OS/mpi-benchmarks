CC              := mpicc
LD              := mpicc
CFLAGS          := -Wall -Wextra -O3 -fopenmp -g
CPPFLAGS        := -I ./
LDFLAGS         := -lm
LIBS            := -fopenmp
RM              := rm -f

SRCS        	:= $(wildcard *.c)
OBJS        	:= $(patsubst %.c,%.o,$(SRCS))
BINS        	:= pingpong_lat pingpong_length pingpong_ts

.PHONY: clean

all: $(BINS)

pingpong_lat: pingpong_lat.o stat_eval.o
	$(LD) $(LIBS) $(LDFLAGS) -o $@ $^

pingpong_ts: pingpong_ts.o
	$(LD) $(LIBS) $(LDFLAGS) -o $@ $^

%.o: %.c
	$(CC) $(CPPFLAGS) -c $(CFLAGS) -o $@ $<

clean:
	$(RM) $(OBJS)
	$(RM) $(BINS)
