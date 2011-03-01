#ifndef RSS_H
#define RSS_H

#include <stdlib.h>

#define CHARMAX 1024

typedef struct rss_item {
  char title[CHARMAX];
  char link[CHARMAX];
  char pubdate[CHARMAX];
  char desc[CHARMAX];
  struct rss_item *next;
} rss_item_t;

typedef struct rss_feed {
  char title[CHARMAX];
  char link[CHARMAX];
  char desc[CHARMAX];
  char url[CHARMAX];
  char pubdate[CHARMAX];
  rss_item_t *first;
  rss_item_t *last;
  int articles;
} rss_feed_t;

struct MemoryStruct {
  char *memory;
  size_t size;
  int errored;
};

// public accessable prototypes
rss_feed_t * load_feed(char *url, int reload, rss_feed_t *feed, const int auth, const char *username, const char *password);
rss_item_t * get_item(rss_feed_t *rf, int index);
void print_feed(rss_feed_t *rf);
void free_feed(rss_feed_t *rf);

#endif
