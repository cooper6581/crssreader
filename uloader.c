#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include "uloader.h"

static struct entries *
parse_file(char *b, int fsize)
{
  struct entries *e = NULL;

  e = malloc(sizeof(struct entries));
  assert(e != NULL);

  e->amount = 0;
  e->head = NULL;
  e->tail = NULL;

  for(int i=0;i<fsize;i++) {
    struct entry *et = malloc(sizeof(struct entry));
    assert(et != NULL);
    et->next = NULL;
    int j = 0;
    while(b[i] != '\n')
      et->url[j++] = b[i++];
    et->url[j] = '\0';
    // setup the linked list
    if(e->head == NULL) {
      e->head = et;
      e->tail = et;
    }
    else {
      e->tail->next = et;
      e->tail = et;
    }
  }

  return e;
}

static int
load_file(const char *file_name, char **b)
{
  FILE *fh;
  size_t fsize;
  char *buffer;

  fh = fopen(file_name, "r");
  assert(fh != NULL);
  fseek(fh,0,SEEK_END);
  fsize = ftell(fh);
  rewind(fh);
  buffer = malloc(sizeof(char) *fsize);
  assert(buffer != NULL);
  fread(buffer,sizeof(char),fsize,fh);
  fclose(fh);

  *b = buffer;
  return (int)fsize;
}

// Pop in a file, returns a nice list of urls
struct entries *
load_entries(const char *filename)
{
  char *buffer;
  int fsize;
  struct entries *et = NULL;

  fsize = load_file(filename,&buffer);
  et = parse_file(buffer,fsize);
  free(buffer);

  return et;
}

void
free_entries(struct entries *et)
{
  struct entry *e;
  struct entry *temp;
  e = et->head;
  while(e != NULL) {
    temp = e->next;
    free(e);
    e = temp;
  }
}

