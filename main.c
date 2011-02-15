#include <stdlib.h>
#include <stdio.h>
#include <assert.h>
#ifndef RSS_H
#define RSS_H
#include "rss.h"
#endif
#include "curses.h"
#include "window.h"

int main(int argc, char **argv) {
  rss_view_t view;
  rss_feed rf;

  view = init_view();
  rf = load_feed(argv[1]);
  add_feed(&rf,&view);

  // Main loop
  for(;;) {
    view.c = getch();
    switch(view.c) {
      case 'q':
        break;
      case 'j':
        waddstr(view.w_articles,"blkahblalblhab");
        prefresh(view.w_articles,0,0,0,0,16,16);
        continue;
    }
    break;
  }

  endwin();
  free_feed(&rf);
  return 0;
}

/*
void
draw_articles(void) {
  werase(pad);
  */
