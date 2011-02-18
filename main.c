#include <assert.h>
#include <locale.h>
#include "curses.h"
#include "rss.h"
#include "window.h"
#include "uloader.h"

// our global rss_view
extern rss_view_t rv;

int main(int argc, char **argv) {
  struct entries *et;
  struct entry *e;

  assert(argc == 2);
  setlocale(LC_ALL, "");
  et = load_entries(argv[1]);
  e = et->head;

  assert(init_view());

  while(e != NULL) {
    add_feed(e->url);
    e = e->next;
  }

  draw_articles();
  refresh();
  prefresh(rv.w_articles,0,0,0,0,rv.y_par-2,rv.x_par);

  // Main loop
  for(;;) {
    rv.c = getch();
    if(rv.c == 'q')
      break;
    else if (rv.c == 'r') {
      reload();
      draw_articles();
    }
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

