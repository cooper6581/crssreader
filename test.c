#include <stdio.h>
#include <assert.h>
#include "rss.h"
#include "common.h"

int
main(int argc, char **argv)
{
  assert(argc == 2);
  init_parser();
  rss_feed_t *r;
  r = load_feed(argv[1],FALSE,NULL,TRUE,"dustink","xxxxx");
  print_feed(r);
  free_feed(r);
  return 0;
}
