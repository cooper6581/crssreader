#include "window.h"
#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>

#define PADLINES 128
// Default to 2 minutes
#define AUTOREFRESH 120

//globals
time_t start_time;
time_t temp_time;
time_t timer;
pthread_mutex_t rmutex;

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
  rv.need_redraw = TRUE;
  // setup our mutex
  pthread_mutex_init(&rmutex,NULL);

  return 14;
}

// TODO: Take autorefresh argument
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
  rw->auto_refresh = AUTOREFRESH;
  rw->timer = rw->auto_refresh;

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
  // clean up the mutex
  pthread_mutex_destroy(&rmutex);
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

void * reload(void *t) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;
  int window_index = 0;

  // if t has a value, lets get that rss_window index, if not
  // we will default to current window
  if(t == NULL)
    rw = get_current_rss_window();
  else {
    window_index = *(int *)t;
    rw = get_rss_window_at_index(window_index);
  }
  rf = rw->r;
  snprintf(tmp_message,rv.x_par,"Reloading %s",rf->title);
  draw_status(tmp_message);
  pthread_mutex_lock(&rmutex);
  load_feed(NULL,1,rf);
  pthread_mutex_unlock(&rmutex);
  // set the updated time of the window
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  pthread_mutex_lock(&rmutex);
  strftime(rw->updated,6,"%H:%M",timeinfo);
  pthread_mutex_unlock(&rmutex);
  //draw_articles();
  snprintf(tmp_message,rv.x_par,"Completed reloading %s",rf->title);
  draw_status(tmp_message);
  // drawing needs to be done while the mutex is locked
  pthread_exit(NULL);
}

void * reload_all(void *t) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;

  for(int i = 0;i<rv.w_amount;i++) {
    rw = get_rss_window_at_index(i);
    // make sure that there isn't another thread messing with rw
    rf = rw->r;
    snprintf(tmp_message,rv.x_par,"Reloading %s",rf->title);
    draw_status(tmp_message);
    pthread_mutex_lock(&rmutex);
    load_feed(NULL,1,rf);
    pthread_mutex_unlock(&rmutex);
    // set the updated time of the window
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    pthread_mutex_lock(&rmutex);
    strftime(rw->updated,6,"%H:%M",timeinfo);
    pthread_mutex_unlock(&rmutex);
    snprintf(tmp_message,rv.x_par,"Completed reloading %s",rf->title);
    draw_status(tmp_message);
  }
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

void check_time(void) {
  time_t current_time;
  rss_window_t *rw;
  current_time = time(NULL);
  if (current_time > temp_time) {
    // cycle through all of our rss windows, and dec the timers
    for(int i = 0; i < rv.w_amount; i++) {
      rw = get_rss_window_at_index(i);
      rw->timer--;
      temp_time = current_time;
      // if the timer hits 0, fire off a thread to refresh that rss feed
      if (rw->timer == 0) {
        rw->timer = rw->auto_refresh;
        pthread_t thread;
        pthread_create(&thread, NULL, reload, &i);
        // need to block for now to see if it's causing my problem
        // it is...
        //pthread_join(thread, NULL);
        pthread_detach(thread);
      }
    }
  }
}
