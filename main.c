#include <assert.h>
#include "curses.h"
#include "rss.h"
#include "window.h"

// our global rss_view
extern rss_view_t rv;

int main(int argc, char **argv) {
  char reddit[] = "http://www.reddit.com/r/programming/.rss";
  char git[] = "http://github-trends.oscardelben.com/explore/week.xml";

  assert(init_view());
  add_feed(reddit);
  add_feed(git);

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
    else if (rv.c == 'H') {
      if(rv.windex > 0)
        rv.windex -= 1;
      else
        rv.windex = rv.w_amount-1;
      draw_articles();
    }
    else if (rv.c == 'L') {
      if(rv.windex < rv.w_amount-1)
        rv.windex += 1;
      else
        rv.windex = 0;
      draw_articles();
    }
    else if (rv.c == '\n')
     select_article();
  }

  // cleanup
  endwin();
  cleanup_view();
  return 0;
}

