#include "window.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define PADLINES 128

//prototypes
static char * _pad_message(const char *msg);

// globals for window
rss_view_t rv;

// This file contains public and private functions for the GUI
int init_view(void) {
  rv.rw_first = NULL;
  rv.cursor = 0;
  rv.windex = 0;
  rv.w_amount = 0;
  rv.w_par = initscr();
  assert(rv.w_par != NULL);
  raw();
  noecho();
  halfdelay(10);
  use_default_colors();
  // This will probably fail in OSX
  curs_set(0);
  getmaxyx(rv.w_par, rv.y_par, rv.x_par);
  rv.w_articles = newpad(PADLINES,rv.x_par);
  assert(rv.w_articles != NULL);

  return 14;
}

int add_feed(char *url) {
  rss_window_t *rw;
  rss_feed_t *rf;
  time_t rawtime;
  struct tm * timeinfo;

  char msg[rv.x_par];
  snprintf(msg,rv.x_par,"Loading: %s",url);
  draw_status(msg);
  rf = load_feed(url,0,NULL);

  // setup our rss window
  rw = malloc(sizeof(rss_window_t));
  rw->cursor = 0;
  rw->r = rf;
  rw->next = NULL;

  // Add time time
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(rw->updated,6,"%H:%M",timeinfo);

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

void cleanup_view(void) {
  rss_window_t *rw;
  rss_window_t *temp;
  rw = rv.rw_first;
#ifdef DEBUG
  int w = 0;
#endif
  for(rw = rv.rw_first; rw != NULL; rw=temp) {
    temp = rw->next;
    free_feed(rw->r);
#ifdef DEBUG
    printf("Freeing window %d\n",w++);
#endif
    free(rw);
    rw=temp;
  }
}

void draw_articles(void) {
  rss_item_t *ri = NULL;
  rss_window_t *rw = NULL;

  rw = get_current_rss_window();
  werase(rv.w_articles);
  if (rv.cursor < 0)
    rv.cursor = 0;
    //rv.cursor = rw->r->articles;
  if (rv.cursor >= rw->r->articles)
    rv.cursor = rw->r->articles-1;

  ri = rw->r->first;
  for(int i = 0;ri != NULL;i++,ri=ri->next) {
    // TODO: Make this a function
    if(strlen(ri->title) < rv.x_par) {
      if (i == rv.cursor) {
        wattron(rv.w_articles,A_REVERSE);
        mvwaddstr(rv.w_articles,i,0,ri->title);
        wattroff(rv.w_articles,A_REVERSE);
      }
      else
        mvwaddstr(rv.w_articles,i,0,ri->title);
    } else {
      char title[rv.x_par];
      strncpy(title,ri->title,rv.x_par-1);
      title[rv.x_par-1] = '>';
      title[rv.x_par] = '\0';
      if (i == rv.cursor) {
        wattron(rv.w_articles,A_REVERSE);
        mvwaddstr(rv.w_articles,i,0,title);
        wattroff(rv.w_articles,A_REVERSE);
      }
      else
        mvwaddstr(rv.w_articles,i,0,title);
    }
  }

  draw_status(NULL);
  if (rw->r->articles > rv.y_par-2 && 
      rv.cursor > rv.y_par-2)
    prefresh(rv.w_articles,rv.cursor - rv.y_par + 2,
        0,0,0,rv.y_par-2,rv.x_par);
  else
    prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
}

static char * _pad_message(const char *msg) {
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

void * reload(void *thread_id) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;

  rw = get_current_rss_window();
  // make sure that there isn't another thread messing with rw
  if (rw->is_updating == TRUE)
    pthread_exit(NULL);
  rw->is_updating = TRUE;
  rf = rw->r;
  snprintf(tmp_message,rv.x_par,"Reloading %s",rf->title);
  draw_status(tmp_message);
  load_feed(NULL,1,rf);
  // set the updated time of the window
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(rw->updated,6,"%H:%M",timeinfo);
  draw_articles();
  snprintf(tmp_message,rv.x_par,"Completed reloading %s",rf->title);
  draw_status(tmp_message);
  rw->is_updating = FALSE;
  pthread_exit(NULL);
}

void * reload_all(void *t) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;
  rss_window_t *crw = NULL;
  int blah = *(int *)t;

  rw = get_rss_window_at_index(blah);
  crw = get_current_rss_window();
  // make sure that there isn't another thread messing with rw
  if (rw->is_updating == TRUE)
    pthread_exit(NULL);
  rw->is_updating = TRUE;
  rf = rw->r;
  snprintf(tmp_message,rv.x_par,"Reloading %s",rf->title);
  draw_status(tmp_message);
  load_feed(NULL,1,rf);
  // set the updated time of the window
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(rw->updated,6,"%H:%M",timeinfo);
  if(crw == rw)
    draw_articles();
  snprintf(tmp_message,rv.x_par,"Completed reloading %s",rf->title);
  draw_status(tmp_message);
  rw->is_updating = FALSE;
  pthread_exit(NULL);
}

void draw_status(const char *msg) {
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;
  char status[rv.x_par];
  char *test_message;

  //wclear(rv.w_par);
  wattron(rv.w_par,A_REVERSE);
  if(msg != NULL)
    sprintf(status,"(%s)",msg);
  else {
    rw = get_current_rss_window();
    rf = rw->r;
    sprintf(status,"[%d/%d] %s - %s",rv.windex+1,rv.w_amount,rf->title,rw->updated);
  }
  test_message = _pad_message(status);
  mvwaddstr(rv.w_par,rv.y_par-1,0,test_message);
  wattroff(rv.w_par,A_REVERSE);
  refresh();
  if(test_message)
    free(test_message);
}

void select_article(void) {
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

void debug_msg(const char *msg) {
    werase(rv.w_articles);
    mvwaddstr(rv.w_articles,0,0,msg);
    prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
}

rss_window_t * get_current_rss_window(void) {
  rss_window_t *rw = NULL;
  rw = rv.rw_first;
  for(int i = 0; i < rv.windex && rw != NULL;i++)
    rw = rw->next;
  return rw;
}

rss_window_t * get_rss_window_at_index(int index) {
  rss_window_t *rw = NULL;
  rw = rv.rw_first;
  for(int i = 0; i != index; i++)
    rw = rw->next;
  return rw;
}

void yank(void) {
  rss_window_t *rw;

  rw = get_current_rss_window();
  char command[1024];
  rss_item_t *ri = NULL;
  ri = get_item(rw->r,rv.cursor);
#ifdef OSX
  snprintf(command,1024,"echo \"%s\" | pbcopy",ri->link);
  system(command);
#endif
#ifdef LINUX
  draw_status("Not implemented");
#endif
}
