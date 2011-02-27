#include <assert.h>
#include <locale.h>
#include <pthread.h>
#include <stdlib.h>
#include "curses.h"
#include "rss.h"
#include "window.h"
#include "uloader.h"
#include "common.h"

// config filename
const char *def_cfg_fname = ".rssreaderrc";
// rssreader version
const char *version = "0.0.1";

// our global rss_view
extern rss_view_t rv;

int help(const char *prog_name) {
  printf("Usage %s [FILE]\n"
	 "Try `man %s` for more information.\n", 
	 prog_name, prog_name);
  return 2;
}

int main(int argc, char **argv) {
  struct entries *et;
  struct entry *e;
  pthread_t rthread;
  // Will be cleaned when the input handler is moved outside of main
  int hit_g = 0;

  assert(argc <= 2);
  /*
    The assert is nice when debugging but sucks
    from a user standpoint. The down side is that we have
    to check for the arguments too. We disable asserts through
    the Makefile when debug is disabled (-DNDEBUG). 
  */
  if (argc > 2) 
    return help(argv[0]);

  setlocale(LC_ALL, "");
  if (argc == 1) {
    /*
      TODO: fix this shit. string manipulation like this is so
      dangerous it makes me want to punch people.
    */
    const char *home_path = getenv("HOME");
    assert(home_path != NULL);
    if (home_path != NULL) {
      /* is 64 enough for home folders? Probably not!
	 (I hate string manipulation in C) 
      */
      char tmp_path[64];
      int sz = snprintf(tmp_path, 64, "%s/%s", home_path, def_cfg_fname);
      assert(sz < 64);
      et = load_entries(tmp_path);
    }
    else {
      printf("Could not load %s from the user home folder.\n", def_cfg_fname);
      return -1;
    }
  }
  else {
    /* Load from the file passed as argument. */
    et = load_entries(argv[1]);
  }
  e = et->head;
  assert(init_view());

  while(e != NULL) {
    if(e->auth == TRUE)
      add_feed(e->url,e->refresh,e->auth,e->username,e->password);
    else
      add_feed(e->url,e->refresh,e->auth,NULL,NULL);
    e = e->next;
  }
  free_entries(et);
  free(et);

  draw_titles();
  refresh();
  prefresh(rv.w_titles, 0, 0, 0, 0, rv.y_par-2, rv.x_par);

  // Main loop
  for(;;) {
      if(rv.need_redraw) {
          if (rv.title_viewing) {
              wclear(rv.w_titles);
              refresh();
              wrefresh(rv.w_titles);
              draw_titles();
              rv.need_redraw = FALSE;
          }
          else {
              wclear(rv.w_articles);
              refresh();
              wrefresh(rv.w_articles);
              draw_articles();
              rv.need_redraw = FALSE;
          }
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
    } else if (rv.c == '"') {
        rv.title_viewing = !rv.title_viewing;
        rv.need_redraw = TRUE;
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
    } else if ((rv.c == 'H' || rv.c == 'h') && !rv.title_viewing) {
      if(rv.windex > 0)
        rv.windex -= 1;
      else
        rv.windex = rv.w_amount-1;
      rv.need_redraw = TRUE;
    // window right
    } else if ((rv.c == 'L' || rv.c == 'l') && !rv.title_viewing) {
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
    } else if (rv.c == '\n') {
        if (rv.title_viewing) {
            rv.title_viewing = !rv.title_viewing;
            select_feed();
            rv.need_redraw = TRUE;
        }
        else
            select_article();
    }
  }

  // cleanup
  endwin();
  cleanup_view();
  return 0;
}

