#include "window.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>

#define PADLINES 128

// globals for window
rss_view_t rv;

// This file contains public and private functions for the GUI
int
init_view(void)
{
  rv.rw_first = NULL;
  rv.cursor = 0;
  rv.windex = 0;
  rv.w_amount = 0;
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
add_feed(char *url)
{
  rss_window_t *rw;
  rss_feed_t *rf;

  char msg[rv.x_par];
  snprintf(msg,rv.x_par,"Loading: %s",url);
  draw_status(msg);
  rf = load_feed(url);

  // setup our rss window
  rw = malloc(sizeof(rss_window_t));
  rw->cursor = 0;
  rw->r = rf;
  rw->next = NULL;

  // add the rss window
  rv.w_amount++;
  if(rv.rw_first == NULL) {
    rv.rw_first = rw;
    rv.rw_last = rw;
    return 0;
  }
  rv.rw_last->next = rw;
  rv.rw_last = rw;
  return 0;
}

void
cleanup_view(void)
{
  rss_window_t *rw;
  rss_window_t *temp;
  rw = rv.rw_first;
  int w = 0;
  for(rw = rv.rw_first; rw != NULL; rw=temp) {
    temp = rw->next;
    free_feed(rw->r);
#ifdef DEBUG
    printf("Freeing window %d\n",w++);
#endif
    free(rw);
  }
}

void
draw_articles(void)
{
  rss_item_t *ri = NULL;
  rss_window_t *rw = NULL;

  rw = get_current_rss_window();
  werase(rv.w_articles);
  if (rv.cursor < 0)
    rv.cursor = rw->r->articles;
  if (rv.cursor >= rw->r->articles)
    rv.cursor = 0;

  ri = rw->r->first;
  for(int i = 0;ri != NULL;i++,ri=ri->next) {
    if (i == rv.cursor) {
      wattron(rv.w_articles,A_REVERSE);
      mvwaddstr(rv.w_articles,i,0,ri->title);
      wattroff(rv.w_articles,A_REVERSE);
    }
    else
    mvwaddstr(rv.w_articles,i,0,ri->title);
  }
  draw_status(NULL);
  if (rw->r->articles > rv.y_par-2 && 
      rv.cursor > rv.y_par-2)
    prefresh(rv.w_articles,rv.cursor - rv.y_par + 2,
        0,0,0,rv.y_par-2,rv.x_par);
  else
    prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
}

static char *
_pad_message(const char *msg)
{
  char *padded = NULL;
  int size_of_msg = 0;
  padded = malloc(sizeof(char) * rv.x_par);
  assert (padded != NULL);
  size_of_msg = (int) strlen(msg);
  strncpy(padded,msg,rv.x_par);
  for(int i=size_of_msg;i<rv.x_par;i++)
    padded[i] = ' ';

  return padded;
}

void
reload(void)
{
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;
  rw = get_current_rss_window();
  rf = rw->r;
  draw_status("reloading");
  reload_feed(rf);
  draw_articles();
}


void
draw_status(const char *msg)
{
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;
  char status[rv.x_par];
  char *test_message;

  wclear(rv.w_par);
  wattron(rv.w_par,A_REVERSE);
  if(msg != NULL)
    sprintf(status,"(%s)",msg);
  else {
    rw = get_current_rss_window();
    rf = rw->r;
    sprintf(status,"[%d/%d] %s",rv.windex+1,rv.w_amount,rf->title);
  }
  test_message = _pad_message(status);
  mvwaddstr(rv.w_par,rv.y_par-1,0,test_message);
  wattroff(rv.w_par,A_REVERSE);
  refresh();
  if(test_message)
    free(test_message);
}

void
select_article()
{
  rss_window_t *rw;

  rw = get_current_rss_window();
  char command[1024];
  rss_item_t *ri = NULL;
  ri = get_item(rw->r,rv.cursor);
  snprintf(command,1024,"open \"%s\"",ri->link);
#ifdef LINUX
  snprintf(command,1024,"gnome-open \"%s\"",ri->link);
#endif
  system(command);
}

void
debug_msg(const char *msg)
{
    werase(rv.w_articles);
    mvwaddstr(rv.w_articles,0,0,msg);
    prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
}

rss_window_t *
get_current_rss_window(void)
{
  rss_window_t *rw = NULL;
  rw = rv.rw_first;
  for(int i = 0; i < rv.windex && rw != NULL;i++)
    rw = rw->next;
  return rw;
}
