#include <stdlib.h>
#include <stdio.h>
#include "rss.h"

int main(int argc, char **argv) {
  rss_feed rf;
  rf = load_feed(argv[1]);
  print_feed(&rf);
  free_feed(&rf);
  return 0;
}
