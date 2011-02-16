#include <assert.h>
#include "curses.h"
#include "rss.h"
#include "window.h"

// our global rss_view
extern rss_view_t rv;

int main(int argc, char **argv) {
  rss_feed rf;

  assert(init_view());
  rf = load_feed(argv[1]);
  add_feed(&rf);
  draw_articles();
  refresh();
  prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);

  // Main loop
  for(;;) {
    rv.c = getch();
    if(rv.c == 'q')
      break;
    else if (rv.c == 'j') {
      rv.cursor += 1;
      draw_articles();
    }
    else if (rv.c == 'k') {
      rv.cursor -= 1;
      draw_articles();
    }
    else if (rv.c == '\n')
     select_article();
  }

  // cleanup
  endwin();
  free_feed(&rf);
  cleanup_view();
  return 0;
}

