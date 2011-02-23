#include <assert.h>
#include <locale.h>
#include <pthread.h>
#include "curses.h"
#include "rss.h"
#include "window.h"
#include "uloader.h"

#define TRUE 1
#define FALSE 0

// our global rss_view
extern rss_view_t rv;

int main(int argc, char **argv) {
  struct entries *et;
  struct entry *e;
  pthread_t rthread;
  // Will be cleaned when the input handler is moved outside of main
  int hit_g = 0;

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

  draw_articles();
  refresh();
  prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);

  // Main loop
  for(;;) {
    if(rv.need_redraw == TRUE) {
      wclear(rv.w_articles);
      refresh();
      prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);
      draw_articles();
      rv.need_redraw = FALSE;
    }
    rv.c = getch();
    check_time();
    // quit
    if(rv.c == 'q')
      break;
    // escape clears our g counter
    else if (rv.c == 0x1B)
      hit_g = 0;
    // go to top gg
    else if (rv.c == 'g') {
      hit_g++;
      if (hit_g == 2) {
        hit_g = 0;
        rv.cursor = 0;
        rv.need_redraw = TRUE;
      }
    // reload
    } else if (rv.c == 'r') {
      pthread_create(&rthread, NULL, reload,NULL);
      pthread_detach(rthread);
    // reload all
    } else if (rv.c == 'R') {
      pthread_create(&rthread, NULL, reload_all,NULL);
      pthread_detach(rthread);
    // go down article
    } else if (rv.c == 'j') {
      rv.cursor += 1;
      rv.need_redraw = TRUE;
    // page down ctrl+d
    } else if (rv.c == 0x04) {
      rv.cursor += rv.y_par/2;
      rv.need_redraw = TRUE;
    // go up
    } else if (rv.c == 'k') {
      rv.cursor -= 1;
      rv.need_redraw = TRUE;
    // page up ctrl+u
    } else if (rv.c == 0x15) {
      rv.cursor -= rv.y_par/2;
      rv.need_redraw = TRUE;
    // switch window left
    } else if (rv.c == 'H' || rv.c == 'h') {
      if(rv.windex > 0)
        rv.windex -= 1;
      else
        rv.windex = rv.w_amount-1;
      rv.need_redraw = TRUE;
    // window right
    } else if (rv.c == 'L' || rv.c == 'l') {
      if(rv.windex < rv.w_amount-1)
        rv.windex += 1;
      else
        rv.windex = 0;
      rv.need_redraw = TRUE;
    // G bottom of the page
    // HACK for now
    } else if (rv.c == 'G') {
      rv.cursor += 9999;
      rv.need_redraw = TRUE;
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

