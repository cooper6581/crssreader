#ifndef ULOADER_H
#define ULOADER_H

#define URL_MAX 1024
#ifndef LINE_MAX
#define LINE_MAX 1024
#endif
#define AUTH_MAX 32

/* XXX: These should probably be an ENUM? */
#define UL_KEY_URL 1
#define UL_KEY_AUTH 2
#define UL_KEY_REFRESH 3
#define UL_KEY_PROXY 4
#define UL_KEY_NTLM 5

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
