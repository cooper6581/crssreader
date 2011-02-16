all:
	gcc -D DEBUG -g -Wall -std=c99 -I /usr/include/libxml2 -o rssreader rss.c window.c main.c -l xml2 -l curl -l curses
test:
	gcc -D DEBUG -g -Wall -std=c99 -I /usr/include/libxml2 -o test_rss rss.c test.c -l xml2 -l curl
