#include "window.h"
#include <assert.h>
#include <stdlib.h>

#define PADLINES 128

// globals for window
rss_view_t rv;

// This file contains public and private functions for the GUI
int
init_view(void)
{
  rv.rw = NULL;
  rv.cursor = 0;
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

  return 14;
}

int
add_feed(rss_feed *rf)
{
  rss_window_t *rw;
  rw = malloc(sizeof(rss_window_t));
  rw->cursor = 0;
  rw->r = rf;
  rv.rw = rw;

  return 0;
}

void
cleanup_view(void)
{
  if (rv.rw != NULL)
    free(rv.rw);
}

// This should be redesigned to not take rss_view
// but to take rss_window.  rss_view should be
// static and global somewhere, as there will only
// ever be 1 rss_view.
//
// Note to self:  Since the rss windows will be a linked
// list, I should have a current pointer to make things
// cleaner for this function
void
draw_articles(void)
{
  rss_item *ri = NULL;
  werase(rv.w_articles);
  if (rv.cursor < 0)
    rv.cursor = rv.rw->r->articles;
  if (rv.cursor >= rv.rw->r->articles)
    rv.cursor = 0;

  ri = rv.rw->r->first;
  for(int i = 0;ri->next != NULL;i++,ri=ri->next) {
    if (i == rv.cursor) {
      wattron(rv.w_articles,A_REVERSE);
      mvwaddstr(rv.w_articles,i,0,ri->title);
      wattroff(rv.w_articles,A_REVERSE);
    }
    else
    mvwaddstr(rv.w_articles,i,0,ri->title);
  }
  draw_status(NULL);
  if (rv.rw->r->articles > rv.y_par-2 && 
      rv.cursor > rv.y_par-2)
    prefresh(rv.w_articles,rv.cursor - rv.y_par + 2,
        0,0,0,rv.y_par-2,rv.x_par);
  else
    prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
}

void
draw_status(const char *msg)
{
  wclear(rv.w_par);
  wattron(rv.w_par,A_REVERSE);
  mvwaddstr(rv.w_par,rv.y_par-1,0,msg);
  wattroff(rv.w_par,A_REVERSE);
  refresh();
}

void
select_article()
{
  char command[1024];
  rss_item *ri = NULL;
  ri = get_item(rv.rw->r,rv.cursor);
  snprintf(command,1024,"open %s",ri->link);
  system(command);
}

void
debug_msg(const char *msg)
{
    werase(rv.w_articles);
    mvwaddstr(rv.w_articles,0,0,msg);
    prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
}

