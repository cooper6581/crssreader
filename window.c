#include "window.h"
#include <assert.h>

#define PADLINES 128

// This file contains public and private functions for the GUI
rss_view_t init_view(void)
{
  rss_view_t rv;

  rv.rss_windows = NULL;
  rv.w_par = initscr();
  assert(rv.w_par != NULL);
  raw();
  noecho();
  use_default_colors();
  // This will probably fail in OSX
  curs_set(0);
  getmaxyx(rv.w_par, rv.y_par, rv.x_par);
  rv.w_articles = newpad(PADLINES,rv.x_par);
  assert(rv.w_articles != NULL);

  return rv;
}

int add_feed(rss_feed *rf, rss_view_t *rv)
{
  rss_window_t rw;

  rw.cursor = 0;
  rw.r = rf;

  return 0;
}
