all:
	gcc -g -Wall -I /usr/include/libxml2 -o rssreader rss.c window.c main.c -l xml2 -l curl -l curses
