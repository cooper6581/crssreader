#include <assert.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>
#include <errno.h>

#include "common.h"
#include "window.h"

#define PADLINES 128

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
  rv.y_view = 0;
  rv.w_articles = newpad(PADLINES,rv.x_par);
  rv.w_titles = newpad(PADLINES,rv.x_par);
  assert(rv.w_articles != NULL);
  assert(rv.w_titles != NULL);
  rv.need_redraw = TRUE;
  rv.is_reloading = FALSE;
  rv.title_viewing = TRUE;
  // setup our mutex
  pthread_mutex_init(&rmutex,NULL);

  return 14;
}

int reinit_view(void) {
  getmaxyx(rv.w_par, rv.y_par, rv.x_par);
  resizeterm(rv.y_par, rv.x_par);
  delwin(rv.w_articles);
  delwin(rv.w_titles);
  rv.w_articles = newpad(PADLINES,rv.x_par);
  rv.w_titles = newpad(PADLINES,rv.x_par);
  assert(rv.w_articles != NULL);
  assert(rv.w_titles != NULL);
  rv.need_redraw = TRUE;
  return 14;
}

// TODO: Take autorefresh argument
int add_feed(char *url, const int autorefresh, const int auth, char *username, char *password) {
  rss_window_t *rw;
  rss_feed_t *rf;
  time_t rawtime;
  struct tm * timeinfo;

  char msg[rv.x_par];
  snprintf(msg,rv.x_par,"Loading: %s",url);
  if(msg != NULL)
    draw_status(msg);
  rf = load_feed(url,0,NULL,auth,username,password);

  if (rf == NULL) {
      // something bad happened, we shouldn't be allocating anything
      return -1;
  }

  // setup our rss window
  rw = malloc(sizeof(rss_window_t));
  rw->cursor = 0;
  rw->r = rf;
  rw->next = NULL;
  rw->auto_refresh = autorefresh;
  rw->timer = rw->auto_refresh;
  rw->auth = auth;
  rw->username = NULL;
  rw->password = NULL;
  rw->is_loading_feed = FALSE;
  if (auth == TRUE) {
    rw->username = malloc(sizeof(char) * AUTH_MAX);
    rw->password = malloc(sizeof(char) * AUTH_MAX);
    strncpy(rw->username,username,AUTH_MAX);
    strncpy(rw->password,password,AUTH_MAX);
  }

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
    fprintf(stderr,"Freeing window %d\n",w++);
#endif
    if(rw->auth == TRUE) {
      if(rw->username != NULL)
        free(rw->username);
      if(rw->password != NULL)
        free(rw->password);
    }
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
  if (rw == NULL || rw->is_loading_feed == TRUE)
      return;

  werase(rv.w_articles);

  if (rv.cursor < 0)
    rv.cursor = 0;

  if (rw->r == NULL)
      return;

  if (rv.cursor >= rw->r->articles)
    rv.cursor = rw->r->articles-1;

  if (rw->r->articles == 0)
    mvwaddstr(rv.w_articles,0,0,"No feeds available.");

  ri = rw->r->first;
  if(ri != NULL) {
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
  }

  draw_status(NULL);
  if(rv.cursor > rv.y_view + rv.y_par-2 && rv.cursor <= rw->r->articles)
    rv.y_view = rv.cursor - rv.y_par+2;
  else if (rv.cursor < rv.y_view)
    rv.y_view = rv.cursor;

  prefresh(rv.w_articles,rv.y_view,0,0,0,rv.y_par-2,rv.x_par);
}

// show all feed urls along with the window number
void draw_titles(void) {
  rss_window_t *rw = NULL;

  werase(rv.w_articles);
  werase(rv.w_titles);
  if (rv.cursor < 0)
      rv.cursor = 0;
  if (rv.cursor >= rv.w_amount)
      rv.cursor = rv.w_amount-1;

  rw = rv.rw_first;
  for(int i=0; rw != NULL; i++, rw = rw->next) {
      if (i == rv.cursor) {
          wattron(rv.w_titles, A_REVERSE);
          mvwaddstr(rv.w_titles, i, 0, rw->r->title);
          wattroff(rv.w_titles, A_REVERSE);
      }
      else
          mvwaddstr(rv.w_titles, i, 0, rw->r->title);
  }

  draw_status("Feeds Loaded");

  if(rv.cursor > rv.y_view + rv.y_par-2 && rv.cursor <= rv.windex)
    rv.y_view = rv.cursor - rv.y_par+2;
  else if (rv.cursor < rv.y_view)
    rv.y_view = rv.cursor;

  prefresh(rv.w_titles,rv.y_view,0,0,0,rv.y_par-2,rv.x_par);
}

static char * _pad_message(const char *msg) {
  char *padded;
  int size_of_msg = 0;
  padded = malloc(sizeof(char) * rv.x_par + 1);
  assert (padded != NULL);
  size_of_msg = (int) strlen(msg);
  strncpy(padded,msg,rv.x_par + 1);
  for(int i=size_of_msg;i<rv.x_par;i++)
    padded[i] = ' ';


  return padded;
}

void * reload(void *t) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_window_t *rw = NULL;
  int window_index = 0;
  rss_feed_t *result;

  if (rv.is_reloading) {
    draw_status("rv.is_reloading set to true, exiting");
    pthread_exit(NULL);
  };

  pthread_mutex_lock(&rmutex);
  rv.is_reloading = TRUE;
  memset(tmp_message,0,rv.x_par);
  // if t has a value, lets get that rss_window index, if not
  // we will default to current window
  if(t == NULL) {
      rw = get_current_rss_window();
      if (rw == NULL) {
          pthread_mutex_unlock(&rmutex);
          pthread_exit(NULL);
          return NULL;
      }
  }
  else {
    window_index = *(int *)t;
    rw = get_rss_window_at_index(window_index);
  }
  snprintf(tmp_message,rv.x_par,"Reloading %s",rw->r->title);
  draw_status(tmp_message);
  rw->is_loading_feed = TRUE;
  result = load_feed(rw->r->url,1,rw->r,rw->auth,rw->username,rw->password);
  rw->is_loading_feed = FALSE;
  // Need to see if load_feed failed so that we don't overwrite
  // the feed's URL info in the event load_feed returns NULL
  if (result == NULL) {
    // Unlock the mutex, and pretend nothing ever happened
    rv.is_reloading = FALSE;
    pthread_mutex_unlock(&rmutex);
    pthread_exit(NULL);
  }
  // load_feed should have succeded
  rw->r = result;
  // set the updated time of the window
  time(&rawtime);
  timeinfo = localtime(&rawtime);
  strftime(rw->updated,6,"%H:%M",timeinfo);
  rw->timer = rw->auto_refresh;
  snprintf(tmp_message,rv.x_par,"Completed reloading %s",rw->r->title);
  draw_status(tmp_message);
  rv.need_redraw = TRUE;
  rv.is_reloading = FALSE;
  pthread_mutex_unlock(&rmutex);
  pthread_exit(NULL);
}

// This is what runs when you do shift R
void * reload_all(void *t) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_window_t *rw = NULL;
  rss_feed_t *result;

  if (rv.is_reloading)
    pthread_exit(NULL);

  pthread_mutex_lock(&rmutex);
  rv.is_reloading = TRUE;
  memset(tmp_message,0,rv.x_par);
  for(int i = 0;i<rv.w_amount;i++) {
    rw = get_rss_window_at_index(i);
    // make sure that there isn't another thread messing with rw
    snprintf(tmp_message,rv.x_par,"Reloading %s",rw->r->title);
    draw_status(tmp_message);
    rw->is_loading_feed = TRUE;
    result = load_feed(rw->r->url,1,rw->r,rw->auth,rw->username,rw->password);
    rw->is_loading_feed = FALSE;
    if (result == NULL) {
      // Let's try again at the default auto_refresh
      rw->timer = rw->auto_refresh;
      continue;
    }
    // load_feed should have completed
    rw->r = result;
    // set the updated time of the window
    time(&rawtime);
    timeinfo = localtime(&rawtime);
    strftime(rw->updated,6,"%H:%M",timeinfo);
    rw->timer = rw->auto_refresh;
    snprintf(tmp_message,rv.x_par,"Completed reloading %s",rw->r->title);
    draw_status(tmp_message);
  }
  rv.is_reloading = FALSE;
  rv.need_redraw = TRUE;
  pthread_mutex_unlock(&rmutex);
  pthread_exit(NULL);
}

// Call back used for auto refreshing rss_windows
void * auto_refresh(void *t) {
  time_t rawtime;
  struct tm * timeinfo;
  char tmp_message[rv.x_par];
  rss_window_t *rw = NULL;

  pthread_mutex_lock(&rmutex);

  memset(tmp_message,0,rv.x_par);
  // cycle through each of the rss windows to see which ones
  // have expired timers
  for(int i = 0;i<rv.w_amount;i++) {
    rw = get_rss_window_at_index(i);
    // If this window has an expired timer, update it!
    // TODO:  Best practices for mutex locking
    // the check for -1 is to make sure we don't update a disabled feed
    if (rw->timer <= 0 && rw->timer != -1) {
      snprintf(tmp_message,rv.x_par,"Reloading %s",rw->r->title);
#ifdef DEBUG
      char tmpstamp[8];
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      strftime(tmpstamp,6,"%H:%M",timeinfo);
      fprintf(stderr,"[%s] - firing thread for %s\n",tmpstamp,rw->r->title);
#endif
      draw_status(tmp_message);
      rw->r = load_feed(rw->r->url,1,rw->r,rw->auth,rw->username,rw->password);
      rw->is_loading_feed = FALSE;
      rw->timer = rw->auto_refresh;
      // set the updated time of the window
      time(&rawtime);
      timeinfo = localtime(&rawtime);
      strftime(rw->updated,6,"%H:%M",timeinfo);
      snprintf(tmp_message,rv.x_par,"Completed reloading %s",rw->r->title);
      draw_status(tmp_message);
    }
  }
  rv.need_redraw = TRUE;
  pthread_mutex_unlock(&rmutex);
  pthread_exit(NULL);
}

// Draws the bar at the bottom of the screen
void draw_status(const char *msg) {
  rss_feed_t *rf = NULL;
  rss_window_t *rw = NULL;
  char status[rv.x_par];
  char *test_message;

  memset(status,0,rv.x_par);
  //wclear(rv.w_par);
  wattron(rv.w_par,A_REVERSE);
  if(msg != NULL)
    sprintf(status,"(%s)",msg);
  else {
    rw = get_current_rss_window();
    rf = rw->r;
    sprintf(status,"[%d/%d] %s - %s",rv.cursor+1,rw->r->articles,rf->title,rw->updated);
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
  if (rw == NULL)
      return;

  char command[1024];
  rss_item_t *ri = NULL;
  ri = get_item(rw->r, rv.cursor);
#ifdef WITH_X
# ifdef LINUX
  snprintf(command, 1024, "xdg-open \"%s\"  2>&1 > /dev/null", ri->link);
# else
  snprintf(command, 1024, "open \"%s\"", ri->link);
# endif
  system(command);
#else
  alert("If you want this functionality, you will need to compile with WITH_X=1.");
#endif
}

void show_article(void) {
  rss_window_t *rw = NULL;
  rss_item_t *ri = NULL;
  if(!rv.title_viewing) {
    rw = get_current_rss_window();
    if (rw == NULL)
        return;
    ri = get_item(rw->r,rv.cursor);
    content(ri->desc);
  } else {
    rw = get_rss_window_at_index(rv.cursor);
    content(rw->r->desc);
  }
}

void select_feed(void) {
  // the index we want, is where our cursor is
  // when viewing the titles
  rv.windex = rv.cursor;
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
  if (rw == NULL)
      return NULL;

  for(int i = 0; i != index; i++)
    rw = rw->next;
  return rw;
}

// copies the select item to the clipboard
// NOTE:  Only OSX is currently supported
void yank(void) {
  rss_window_t *rw;

  rw = get_current_rss_window();
  if (rw != NULL) {
      rss_item_t *ri = NULL;
      ri = get_item(rw->r,rv.cursor);
#ifdef OSX
      char command[1024];
      snprintf(command,1024,"echo \"%s\" | pbcopy",ri->link);
      system(command);
#endif
#if defined(LINUX) && defined(WITH_X)
      char command[1024];
      snprintf(command, 1024, "echo \"%s\" | xclip", ri->link);
      system(command);
#else
      alert("If you want this functionality, you will need to compile with WITH_X=1.");
#endif
  }
}

// Function used to decrease the timers for each rss_window
void check_time(void) {
    time_t current_time;
    rss_window_t *rw;
    current_time = time(NULL);
    if (current_time > temp_time) {
        // cycle through all of our rss windows, and dec the timers
        for(int i = 0; i < rv.w_amount; i++) {
            rw = get_rss_window_at_index(i);
            // make sure the timer is enabled before dec
            // also make sure we don't accidentaly disable a real timer
            if(rw->timer != -1 && rw->timer != 0)
                rw->timer--;
            temp_time = current_time;
            // Launch a thread to refresh if one of the timers expired
            // this also is what keeps our threads sane.  check_time
            // will set is_loading_feed to TRUE, the thread will set it
            // to FALSE after it has reloaded the feed.
            if(rw->timer == 0) {
                if(rw->is_loading_feed == FALSE) {
                    pthread_t thread;
                    int error_no;
                    error_no = pthread_create(&thread, NULL, auto_refresh, NULL);
                    if (error_no && error_no != EAGAIN) {
                        // detach if we were unable to create.
                        //pthread_detach(thread);
                        fprintf(stderr,"pthread_create returned %s\n",strerror(error_no));
                        exit(EXIT_FAILURE);
                    }
                    // We were able to launch a thread, remember to detach...
                    if (error_no != EAGAIN) {
                      pthread_detach(thread);
                      rw->is_loading_feed = TRUE;
                    }
                    else
                      fprintf(stderr,"Unable to launch thread, trying again\n");
                }
            }
        }
    }
}

void alert(const char *msg) {
  WINDOW *w_alert = NULL;
  WINDOW *w_alert_text = NULL;
  w_alert = newwin(rv.y_par / 3, rv.x_par / 2, rv.y_par / 3, rv.x_par / 4);
  w_alert_text = subwin(w_alert, (rv.y_par / 3) - 2, (rv.x_par / 2) - 2, (rv.y_par / 3) + 1, (rv.x_par / 4) + 1);
  assert(w_alert || w_alert_text != NULL);
  box(w_alert, 0 , 0);
  mvwaddstr(w_alert_text,0,0,msg);
  // Input is going to go into blocking mode on purpose to leave window up until a key is pressed
  raw();
  wgetch(w_alert);
  // cleanup the window, and go back to non blocking input mode
  // ugly way to clear the border completely
  wborder(w_alert, ' ', ' ', ' ',' ',' ',' ',' ',' ');
  werase(w_alert);
  werase(w_alert_text);
  wrefresh(w_alert);
  wrefresh(w_alert_text);
  delwin(w_alert);
  delwin(w_alert_text);
  halfdelay(10);
  rv.need_redraw = TRUE;
}

// This is a content window (larger than an alert window)
void content(const char *msg) {
  WINDOW *w_alert = NULL;
  WINDOW *w_alert_text = NULL;
  int content_y = rv.y_par -8;
  int content_x = rv.x_par -8;
  w_alert = newwin(content_y, content_x, 4, 4);
  w_alert_text = subwin(w_alert, content_y - 3,  content_x - 4, 5, 6);
  assert(w_alert || w_alert_text != NULL);
  box(w_alert, 0 , 0);
  mvwaddstr(w_alert_text,0,0,msg);
  // Input is going to go into blocking mode on purpose to leave window up until a key is pressed
  raw();
  wgetch(w_alert);
  // cleanup the window, and go back to non blocking input mode
  // ugly way to clear the border completely
  wborder(w_alert, ' ', ' ', ' ',' ',' ',' ',' ',' ');
  werase(w_alert);
  werase(w_alert_text);
  wrefresh(w_alert);
  wrefresh(w_alert_text);
  delwin(w_alert);
  delwin(w_alert_text);
  halfdelay(10);
  rv.need_redraw = TRUE;
}
