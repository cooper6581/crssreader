#define CHARMAX 1024

typedef struct rss_item_t {
  char title[CHARMAX];
  char link[CHARMAX];
  char pubdate[CHARMAX];
} rss_item;

typedef struct rss_feed_t {
  char title[CHARMAX];
  char link[CHARMAX];
  char desc[CHARMAX];
  rss_item *rss_items;
} rss_feed;

struct MemoryStruct {
  char *memory;
  size_t size;
};

rss_feed load_feed(char *url);
