#ifndef ULOADER_H
#define ULOADER_H

#define URL_MAX 1024

struct entry {
  char url[URL_MAX];
  struct entry *next;
};

struct entries {
  int amount;
  struct entry *head;
  struct entry *tail;
};

struct entries *load_entries(const char *filename);
void print_entries(struct entries *et);
void free_entries(struct entries *et);

//static int load_file(const char *file_name, char **b);
//static struct entries *parse_file(char *b, int fsize);

#endif
