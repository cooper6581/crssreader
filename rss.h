#include <stdlib.h>
#define CHARMAX 1024

typedef struct rss_item_t {
  char title[CHARMAX];
  char link[CHARMAX];
  char pubdate[CHARMAX];
  struct rss_item_t *next;
} rss_item;

typedef struct rss_feed_t {
  char title[CHARMAX];
  char link[CHARMAX];
  char desc[CHARMAX];
  rss_item *first;
  rss_item *last;
  int articles;
} rss_feed;

struct MemoryStruct {
  char *memory;
  size_t size;
};

rss_feed load_feed(char *url);
void print_feed(rss_feed *r);
void free_feed(rss_feed *r);
