#include <sys/param.h>

#include <assert.h>
#include <locale.h>
#include <pthread.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pwd.h>
#include "curses.h"
#include "rss.h"
#include "window.h"
#include "uloader.h"
#include "common.h"

// config filename
const char def_cfg_fname[] = RC_NAME;
// rssreader version
//XXX: move this into a config.h file
const char *version = "0.0.1";

// our global rss_view
extern rss_view_t rv;

// global window resized
int need_resize = FALSE;

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
  uid_t uid;
  struct passwd *pwd;
  // Will be cleaned when the input handler is moved outside of main
  int hit_g = 0;
  int tmp;
  char tmp_path[MAXPATHLEN];

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
         HOME may be set to something dangerous. Best to get the path
         for the user home from the passwd record.
      */
      uid = getuid();
      if (!(pwd = getpwuid(uid))) {
          printf("Unable to get user's record.\n");
          endpwent();
          return 1;
      }
      tmp = snprintf(tmp_path, sizeof(tmp_path), "%s/%s", pwd->pw_dir, def_cfg_fname);
      if (tmp == MAXPATHLEN) {
          printf("Path was truncated. Not sure what to do. Exiting.");
          return -1;
      }
      endpwent();
  } else {
      if (strncpy(tmp_path, argv[1], MAXPATHLEN)) {
          printf("Path was truncated. Not sure what to do. Exiting.");
          return -1;
      }
  }

  init_parser();
  et = load_entries(tmp_path);
  e = et->head;
  assert(init_view());

  while(e != NULL) {
    if(e->auth != FALSE)
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
    // Check for resize
    if(need_resize == TRUE) {
        need_resize = FALSE;
        assert(reinit_view());
    }
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
    rv.c = wgetch(rv.w_par);
    check_time();
    // check resize
    if(rv.c == KEY_RESIZE || rv.c == ERR) {
        fprintf(stderr,"HI!!\n");
        need_resize = TRUE;
    }
    // quit
    else if(rv.c == 'q')
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
    // Just a test to see if the pop-up window is working
    } else if (rv.c == '?') {
      alert("This is a help window!\n I'm going to purposefully type a lot of text here to see what happens");
    // enter
    } else if (rv.c == '\n') {
        if (rv.title_viewing) {
            rv.title_viewing = !rv.title_viewing;
            select_feed();
            rv.need_redraw = TRUE;
        }
        else
            select_article();
    } else if (rv.c == '>') {
            show_article();
    }
  }

  // cleanup
  endwin();
  cleanup_view();
  cleanup_parser();
  pthread_exit(NULL);
}
