dist ?= LINUX
debug ?= 1

PREFIX=/usr
TARGET=rssreader

CC=gcc
# endpwent and getpwent need _BSD_SOURCE or POSIX feature macros to be defined
CFLAGS= -O0 -Wall -std=c99 -pthread -D_BSD_SOURCE
LIBS=-l xml2 -l curl
DBGFLAGS=
LDFLAGS=-L$(PREFIX)/lib
INCDIR=-I$(PREFIX)/include -I$(PREFIX)/include/libxml2

ifeq ($(debug), 1)
	DBGFLAGS+= -g
else
	CFLAGS += -D NDEBUG
endif

ifeq ($(dist), OSX)
	LIBS+=-l curses
endif
ifeq ($(dist), LINUX)
	LIBS+=-l ncursesw
endif

all:
	$(CC) -D $(dist) $(DBGFLAGS) $(CFLAGS) $(INCDIR) -o $(TARGET) rss.c uloader.c window.c main.c $(LDFLAGS) $(LIBS)
test:
	$(CC) -D $(dist) $(DBGFLAGS) $(CFLAGS) $(INCDIR) -o test rss.c test.c $(LDFLAGS) $(LIBS)
clean:
	rm -rf $(TARGET) rssreader.* test_rss.* test test.d*
