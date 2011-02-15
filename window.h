#ifndef RSS_H
#define RSS_H
#include "rss.h"
#endif
#include "curses.h"

// struct used to represent data which is displayed in an rss window
typedef struct rss_window {
  // 1 rss feed per rss_window
  rss_feed *r;
  int cursor;
  char *updated;
} rss_window_t;

// encapsulates the main window and article display
typedef struct rss_view {
  // our awesome 1 byte input buffer!
  char c;
  // the size of the terminal window when ncurses is launched, or when the
  // window is resized
  int x_par;
  int y_par;
  // Our parent window, and article view window
  WINDOW *w_par;
  WINDOW *w_articles;
  // Linked list of rss_windows
  rss_window_t *rss_windows;
} rss_view_t;

rss_view_t init_view(void);
int add_feed(rss_feed *rf, rss_view_t *rv);

