#CFLAGS=-g
CFLAGS=-O2

xbacklight2: xbacklight2.c

.phony: clean
clean:
	rm -f xbacklight2
