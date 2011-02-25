#ifndef ULOADER_H
#define ULOADER_H

#define URL_MAX 1024
#define LINE_MAX 1024
#define AUTH_MAX 32

#define TRUE 1
#define FALSE 0

#define UL_KEY_URL 1
#define UL_KEY_AUTH 2
#define UL_KEY_REFRESH 3

struct entry {
  char url[URL_MAX];
  int auth;
  int refresh;
  char username[AUTH_MAX];
  char password[AUTH_MAX];
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

#endif
