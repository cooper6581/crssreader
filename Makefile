all:
	gcc -D OSX -g -Wall -std=c99 -I /usr/include/libxml2 -o rssreader rss.c uloader.c window.c main.c -l xml2 -l curl -l curses

test:
	gcc -D DEBUG -g -Wall -std=c99 -I /usr/include/libxml2 -o test_rss rss.c test.c -l xml2 -l curl

clean:
	rm -rf rssreader test_rss rssreader.* test_rss.*
