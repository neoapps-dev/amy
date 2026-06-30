CC = gcc
CFLAGS = -std=c99 -Wall -Wextra -Wpedantic -O2
LDFLAGS = -lm
HAVE_SDL := $(shell command -v sdl2-config 2>/dev/null)
ifneq ($(HAVE_SDL),)
    CFLAGS += $(shell sdl2-config --cflags) -DAMY_HAVE_SDL
    LDFLAGS += $(shell sdl2-config --libs)
endif
amy: main.c amy.c amy.h
	$(CC) $(CFLAGS) main.c amy.c -o amy $(LDFLAGS)

clean:
	rm -f amy

.PHONY: clean
