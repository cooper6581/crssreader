#include <assert.h>
#include <locale.h>
#include <pthread.h>
#include <time.h>
#include "curses.h"
#include "rss.h"
#include "window.h"
#include "uloader.h"

#define AUTOREFRESH 120

// this is just a proof of concept
void check_time(void);

// our global rss_view
extern rss_view_t rv;

// globals for the poc
time_t start_time;
time_t temp_time;
time_t timer;

void check_time(void) {
  time_t current_time;
  current_time = time(NULL);
  if (current_time > temp_time) {
    timer--;
    temp_time = current_time;
    if (timer == 0) {
      timer = AUTOREFRESH;
      // Ghetto, just a POC
      pthread_t threads[rv.w_amount];
      for(int i=0;i<rv.w_amount;i++) {
        pthread_create(&threads[i], NULL, reload_all,&i);
        pthread_detach(threads[i]);
      }
      wclear(rv.w_articles);
      draw_articles();
    }
  }
}

int main(int argc, char **argv) {
  struct entries *et;
  struct entry *e;
  pthread_t rthread;

  assert(argc == 2);
  setlocale(LC_ALL, "");
  et = load_entries(argv[1]);
  e = et->head;

  assert(init_view());

  while(e != NULL) {
    add_feed(e->url);
    e = e->next;
  }
  free_entries(et);
  free(et);

  //timer stuff
  timer = AUTOREFRESH;
  start_time = time(NULL);
  temp_time = start_time;

  draw_articles();
  refresh();
  prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);

  // Main loop
  for(;;) {
    rv.c = getch();
    check_time();
    // quit
    if(rv.c == 'q')
      break;
    // reload
    else if (rv.c == 'r') {
      pthread_create(&rthread, NULL, reload,NULL);
      pthread_detach(rthread);
    // reload all
    } else if (rv.c == 'R') {
      // create an array of threads
      pthread_create(&rthread, NULL, reload_all,NULL);
      pthread_detach(rthread);
      wclear(rv.w_articles);
      draw_articles();
    // go down article
    } else if (rv.c == 'j') {
      rv.cursor += 1;
      draw_articles();
    // page down ctrl+d
    } else if (rv.c == 0x04) {
      rv.cursor += rv.y_par/2;
      draw_articles();
    // go up
    } else if (rv.c == 'k') {
      rv.cursor -= 1;
      draw_articles();
    // page up ctrl+u
    } else if (rv.c == 0x15) {
      rv.cursor -= rv.y_par/2;
      draw_articles();
    // switch window left
    } else if (rv.c == 'H' || rv.c == 'h') {
      if(rv.windex > 0)
        rv.windex -= 1;
      else
        rv.windex = rv.w_amount-1;
      draw_articles();
    // window right
    } else if (rv.c == 'L' || rv.c == 'l') {
      if(rv.windex < rv.w_amount-1)
        rv.windex += 1;
      else
        rv.windex = 0;
      draw_articles();
    // G bottom of the page
    // HACK for now
    } else if (rv.c == 'G') {
      rv.cursor += 9999;
      draw_articles();
    // Copies URL to clipboard
    } else if (rv.c == 'y') {
      yank();
    // enter
    } else if (rv.c == '\n')
     select_article();
  }

  // cleanup
  endwin();
  cleanup_view();
  return 0;
}

