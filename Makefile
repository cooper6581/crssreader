dist ?= LINUX
debug ?= 1
WITH_X ?= 1

PREFIX=/usr
TARGET=rssreader

CC=gcc
# endpwent and getpwent need _BSD_SOURCE or POSIX feature macros to be defined
CFLAGS= -O0 -Wall -std=c99 -pthread -D_BSD_SOURCE
LIBS=-l xml2 -l curl
DBGFLAGS=
LDFLAGS=-L$(PREFIX)/lib
INCDIR=-I$(PREFIX)/include -I$(PREFIX)/include/libxml2
OPTIONS=

ifeq ($(debug), 1)
	DBGFLAGS+= -g
else
	CFLAGS += -D NDEBUG
endif

ifeq ($(WITH_X), 1)
	OPTIONS += -D WITH_X
endif

ifeq ($(dist), OSX)
	LIBS+=-l curses
endif
ifeq ($(dist), LINUX)
	LIBS+=-l ncursesw
endif

.PHONY: check_deps clean

all: check_deps
	$(CC) -D $(dist) $(OPTIONS) $(DBGFLAGS) $(CFLAGS) $(INCDIR) -o $(TARGET) rss.c uloader.c window.c main.c $(LDFLAGS) $(LIBS)
test: check_deps
	$(CC) -D $(dist) $(OPTIONS) $(DBGFLAGS) $(CFLAGS) $(INCDIR) -o test rss.c test.c $(LDFLAGS) $(LIBS)
clean:
	rm -rf $(TARGET) rssreader.* test_rss.* test test.d*
check_deps:
	@command -v xclip &> /dev/null || echo "We require xclip to run in Linux/BSD. If you use a terminal, compile with WITH_X=0"
	@command -v xdg-open &> /dev/null || echo "We require xdg-open to run in Linux/BSD. If you use a terminal, compile with WITH_X=0" 

