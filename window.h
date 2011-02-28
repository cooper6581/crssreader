#ifndef WINDOW_H
#define WINDOW_H

#include "rss.h"
#include "curses.h"

#define AUTH_MAX 32

// struct used to represent data which is displayed in an rss window
typedef struct rss_window {
  // 1 rss feed per rss_window
  rss_feed_t *r;
  struct rss_window *next;
  int cursor;
  char updated[6];
  // this is the auto refresh value for the feed
  // -1 will mean disabled
  int auto_refresh;
  // This is our actual timer
  int timer;
  // These fields are to support rss feeds that require authentication
  int auth;
  char *username;
  char *password;

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
  // Our parent window, article view window and titles view window
  WINDOW *w_par;
  WINDOW *w_articles;
  WINDOW *w_titles;
  // Linked list of rss_windows
  rss_window_t *rw_first;
  rss_window_t *rw_last;
  // and the amount we have
  int w_amount;
  // a hopefully clean and cpu friendly way to handle screen redraw
  int need_redraw;
  // if I end up needing more status states, this will need to be an ENUM
  int is_reloading;
  // are we drawing titles
  int title_viewing;
  // view stuff to make scrolling more like vi
  int y_view;
} rss_view_t;

int init_view(void);
int add_feed(char *url, const int autorefresh, const int auth, char *username, char *password);
void draw_articles(void);
void draw_titles(void);
void cleanup_view(void);
void draw_status(const char *msg);
void select_article(void);
void select_feed(void);
void debug_msg(const char *msg);
void *reload(void *t);
void *reload_all(void *t);
void *auto_refresh(void *t);
void yank(void);
void check_time(void);
rss_window_t * get_current_rss_window(void);
rss_window_t * get_rss_window_at_index(int index);

#endif
