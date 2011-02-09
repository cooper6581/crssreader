#include <stdlib.h>
#include <stdio.h>
#include "rss.h"

int main(int argc, char **argv) {
  load_feed(argv[1]);
  return 0;
}
