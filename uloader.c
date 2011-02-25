#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <ctype.h>
#include <unistd.h>
#include "uloader.h"

//globals
static char *buffer;
static int fsize;
static int b_index;
static int prev_line;

// function prototypes
static int get_key(char *line_buffer, char *value, int buffer_size);
static int get_line(char *line_buffer, int buffer_size);
static void push_line(void);

// Each time this function is called, it returns the next line in the buffer.
// Because of how I parse the url file, I needed to be able to push back to
// the stack.  Instead of doing this correctly with a stack of lines, there 
// are just simply 2 global variables (b_index is the top, prev_line is the index
// to the previous line
static int get_line(char *line_buffer, int buffer_size) {
  int j = 0;
  prev_line = b_index;
  while(b_index < fsize) {
    // copy to our line buffer if we don't have a new line
    while(buffer[b_index] != '\n') {
      // check for overflow
      if (j < buffer_size)
        line_buffer[j++] = buffer[b_index++];
    }
    // we hit a new line
    b_index++;
    line_buffer[j] = '\0';
    return 1;
  }
  // file is over
  return -1;
}

// yeah...
static void push_line(void) {
  b_index = prev_line;
}

// This function returns the key type, and sets the value buffer.
// Returns -1 if the key isn't recognized
static int get_key(char *line_buffer, char *value, int buffer_size) {
  char key[LINE_MAX];
  char *l = line_buffer;
  int i = 0;
  int j = 0;
  int status = -1; 

  while(*l != '\0' && *l != '#' && i < buffer_size) {
    i++;
    while(!isspace(*l))
     key[j++] = *l++;
    break;
  }
  key[j] = '\0';
  // skip blank lines
  if(strlen(key)) {
    if (strncasecmp(key,"url",LINE_MAX) == 0)
      status = UL_KEY_URL;
    else if (strncasecmp(key,"auth",LINE_MAX) == 0)
      status = UL_KEY_AUTH;
    else if (strncasecmp(key,"refresh",LINE_MAX) == 0)
      status = UL_KEY_REFRESH;
    else
      fprintf(stderr,"Key not found: %s\n", key);
  }
  // skip over white space, we are going to reuse j to b_index value
  j = 0;
  while(isspace(*l) && i < buffer_size) {
    i++;
    ++l;
  }
  // Copy our value
  while(*l != '\0' && i < buffer_size) {
    value[j++] = *l++;
  }
  value[j] = '\0';

  return status;
}


static struct entries * parse_file(char *b, int fsize) {
  char line_buffer[LINE_MAX];
  char value[LINE_MAX];
  char input_buffer[AUTH_MAX];
  char *password;
  int in_url = FALSE;
  struct entries *e = NULL;
  struct entry *et = NULL;

  e = malloc(sizeof(struct entries));
  assert(e != NULL);

  e->amount = 0;
  e->head = NULL;
  e->tail = NULL;

  while(get_line(line_buffer,LINE_MAX) != -1) {
    // this should only be executed on the first URL encountered
    if (get_key(line_buffer,value,LINE_MAX) == UL_KEY_URL && in_url == FALSE) {
      // We are going to create our entry
      et = malloc(sizeof(struct entry));
      assert(et != NULL);
      // setup our entry
      et->next = NULL;
      et->auth = FALSE;
      et->refresh = -1;
      strncpy(et->url,value,URL_MAX);
      in_url = TRUE;
    } else if (get_key(line_buffer,value,LINE_MAX) == UL_KEY_AUTH && in_url == TRUE) {
      et->auth = TRUE;
      printf("Please enter login credentials for %s\n",et->url);
      printf("Username: ");
      scanf("%s",input_buffer);
      strncpy(et->username,input_buffer,AUTH_MAX);
      password = getpass("Password: ");
      strncpy(et->password,password,AUTH_MAX);
    } else if (get_key(line_buffer,value,LINE_MAX) == UL_KEY_REFRESH && in_url == TRUE) {
      int r = 0;
      r = atoi(value);
      if(r == 0) {
        fprintf(stderr,"problem converting refresh string to integer\n");
        exit(1);
      }
      et->refresh = r;
    // we are now in a new url entry
    } else if (get_key(line_buffer,value,LINE_MAX) == UL_KEY_URL && in_url == TRUE) {
      // setup the linked list
      if(e->head == NULL) {
        e->head = et;
        e->tail = et;
      } else {
        e->tail->next = et;
        e->tail = et;
      }
      //print_entry(e);
      in_url = FALSE;
      // push back to the stack
      push_line();
    }
  }
  // last entry
  // TODO:  this is hacky
  if(e->head == NULL) {
    e->head = et;
    e->tail = et;
  } else {
    e->tail->next = et;
    e->tail = et;
  }

  return e;
}

static int load_file(const char *file_name, char **b) {
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
struct entries * load_entries(const char *filename) {
  // These are global now
  // char *buffer;
  // int fsize;
  b_index = 0;
  prev_line = 0;
  struct entries *et = NULL;

  fsize = load_file(filename,&buffer);
  et = parse_file(buffer,fsize);
  free(buffer);

  return et;
}

void free_entries(struct entries *et) {
  struct entry *e;
  struct entry *temp;
  e = et->head;
  while(e != NULL) {
    temp = e->next;
    free(e);
    e = temp;
  }
}

