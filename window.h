#ifndef WINDOW_H
#define WINDOW_H

#include "rss.h"
#include "curses.h"

// struct used to represent data which is displayed in an rss window
typedef struct rss_window {
  // 1 rss feed per rss_window
  rss_feed_t *r;
  struct rss_window *next;
  int cursor;
  char *updated;
} rss_window_t;

// encapsulates the main window and article display
typedef struct rss_view {
  // our awesome 1 byte input buffer!
  char c;
  // Our global cursor
  int cursor;
  // the size of the terminal window when ncurses is launched, or when the
  // window is resized
  int x_par;
  int y_par;
  // Our window index
  int windex;
  // Our parent window, and article view window
  WINDOW *w_par;
  WINDOW *w_articles;
  // Linked list of rss_windows
  rss_window_t *rw_first;
  rss_window_t *rw_last;
  // and the amount we have
  int w_amount;
} rss_view_t;

int init_view(void);
int add_feed(char *url);
void draw_articles(void);
void cleanup_view(void);
void draw_status(const char *msg);
void select_article(void);
void debug_msg(const char *msg);
void reload(void);
void yank(void);
rss_window_t * get_current_rss_window(void);

#endif
