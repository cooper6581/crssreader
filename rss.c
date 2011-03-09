#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <libxml/xmlmemory.h>
#include <libxml/HTMLparser.h>
#include <libxml/parser.h>
#include <assert.h>

#include "common.h"
#include "rss.h"

// prototypes
static size_t WriteMemoryCallback(void *ptr, size_t size,size_t nmemb, void *data);
static struct MemoryStruct _load_url(char *url, const int auth, const char *username, const char *password);
static char * _strip_html(char *s);
static void _free_items(rss_feed_t *r);
static void _parse_items_rss(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur);
static void _parse_items_atom(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur);
static void _parse_channel_rss(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur);
static void _parse_channel_atom(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur);
static void _parse_feed(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur);
static void _character_callback(void *user_data, const xmlChar* ch, int len);
static void _endDocument (void *user_data);
static void _startDocument (void *user_data);


void init_parser(void) {
  // supposed to fix my threading issues?
  xmlInitParser();
}

void cleanup_parser(void) {
  xmlCleanupParser();
}

// Callback used by libcurl
static size_t WriteMemoryCallback(void *ptr, size_t size,size_t nmemb, void *data) {
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    fprintf(stderr,"Not enough memory (realloc returned NULL in curl callback)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// Callback used by SAX to strip html
static void _character_callback(void *user_data, const xmlChar* ch, int len) {
  struct sax_parser *sp = user_data;
  sp->buffer = realloc(sp->buffer,sizeof(char) * (sp->bytes_copied + len + 1));
  if(sp->buffer == NULL) {
    fprintf(stderr,"Not enough memory (realloc returned NULL in sax callback)\n");
    exit(EXIT_FAILURE);
  }
  memcpy(&(sp->buffer[sp->bytes_copied]), (char *)ch, len+1);
  sp->bytes_copied += len;
}

// Callback used for end document
static void _endDocument (void *user_data) {
  struct sax_parser *sp = user_data;
  sp->buffer[sp->bytes_copied] = '\0';
  sp->final = malloc(sizeof(char) * sp->bytes_copied+1);
  if(sp->buffer == NULL) {
    fprintf(stderr,"Not enough memory (malloc returned NULL in sax endDocument callback)\n");
    exit(EXIT_FAILURE);
  }
  strncpy(sp->final,sp->buffer,sp->bytes_copied+1);
  if(sp->buffer != NULL)
    free(sp->buffer);
}

// I hate this
static void _startDocument (void *user_data) {
  struct sax_parser *sp = user_data;
  if(sp->buffer != NULL)
    free(sp->buffer);
  sp->final = NULL;
}

// Used to strip html from text
// Returns character array that needs to be FREED!
static char * _strip_html(char *s) {
  int mem_base;
  mem_base = xmlMemBlocks();
  struct sax_parser sp;
  // init our sax parser struct
  sp.buffer = NULL;
  sp.final = NULL;
  sp.bytes_copied = 0;
  // setup our callbacks
  xmlSAXHandler handler; bzero(&handler, sizeof(xmlSAXHandler));
  handler.characters = &_character_callback;
  handler.endDocument = &_endDocument;
  handler.startDocument = &_startDocument;

  // cross our fingers
  htmlSAXParseDoc((xmlChar*)s,"utf-8", &handler, &sp);

#ifdef DEBUG
  if (mem_base != xmlMemBlocks()) {
    printf("Leak of %d blocks found in htmlSAXParseDoc",
      xmlMemBlocks() - mem_base);
  }
#endif

  return sp.final;
}

// Internal function used to load url to memory
// TODO:  Have the curl stuff be global so we can use the same
// handle for the entire duration of the program
static struct MemoryStruct _load_url(char *url, const int auth, const char *username, const char *password) {
  CURL *curl_handle;
  struct MemoryStruct chunk;
  int rcode = 0;

  chunk.memory = malloc(1);
  chunk.size = 0;
  chunk.errored = 0;
  if ((rcode = curl_global_init(CURL_GLOBAL_ALL)) != 0) {
      printf("Couldn't do global curl initialization.\nError code: %d\n", rcode);
      chunk.errored = 1;
      return chunk;
  }
  /* init the curl session */
  if ((curl_handle = curl_easy_init()) == NULL) {
      printf("Couldn't init curl session.\n");
      chunk.errored = 1;
      return chunk;
  }
  /* specify URL to get */
  if ((rcode = curl_easy_setopt(curl_handle, CURLOPT_URL, url)) != 0) {
      printf("Couldn't setopt CURLOPT_URL.\nError code: %d", rcode);
      chunk.errored = 1;
      return chunk;
  }
  /* send all data to this function  */
  if ((rcode = curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback)) != 0) {
      printf("Couldn't setopt CURLOPT_WRITEFUNCTION.\nError code: %d", rcode);
      chunk.errored = 1;
      return chunk;
  }
  /* we pass our 'chunk' struct to the callback function */
  if ((rcode = curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk)) != 0) {
      printf("Couldn't setopt CURLOPT_WRITEDATE.\nError code: %d", rcode);
      chunk.errored = 1;
      return chunk;
  }
  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  if ((rcode = curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0")) != 0) {
      printf("Couldn't setopt CURLOPT_USERAGENT.\nError code: %d", rcode);
      chunk.errored = 1;
      return chunk;
  }
  // Gmail part
  // XXX: check for errors here!
  if(auth == TRUE) {
    char login[512];
    snprintf(login,512,"%s:%s",username,password);
    curl_easy_setopt(curl_handle, CURLOPT_HTTPAUTH, CURLAUTH_BASIC);
    curl_easy_setopt(curl_handle, CURLOPT_USERPWD, login);
  }
  /* get it! */
  if ((rcode = curl_easy_perform(curl_handle)) != 0 ) {
      printf("Couldn't download.\nError code: %d", rcode);
      chunk.errored = 1;
      return chunk;
  }
  /* cleanup curl stuff */
  curl_easy_cleanup(curl_handle);
  /*
   * Now, our chunk.memory points to a memory block that is chunk.size
   * bytes big and contains the remote file.
   *
   * Do something nice with it!
   *
   * You should be aware of the fact that at this point we might have an
   * allocated data block, and nothing has yet deallocated that data. So when
   * you're done with it, you should free() it as a nice application.
   */
#ifdef DEBUG
  printf("%lu bytes retrieved\n", (long)chunk.size);
#endif

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return chunk;
}

// free's the rss item linked list
static void _free_items(rss_feed_t *r) {
  int i = 0;
  rss_item_t *ri;
  rss_item_t *temp;
  if (r != NULL) {
      ri = r->first;
      while(ri != NULL) {
        temp = NULL;
#ifdef DEBUG
          printf("Freeing article %02d\n",i);
#endif
          if(ri != NULL) {
            temp = ri->next;
            free(ri);
          }
          ri=temp;
          i++;
      }
  }
}

// Populates the rss item linked list
static void _parse_items_rss(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur) {
  xmlChar *key;
  rss_item_t *ri;

  ri = malloc(sizeof(rss_item_t));
  ri->next = NULL;
  // Drilling down into the item children
  cur = cur->xmlChildrenNode;
  // Cycle through the item child elements, dump these values into our rss item
  while (cur != NULL) {
    if (xmlStrcmp(cur->name, (const xmlChar *)"title") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->title,(char *)key,CHARMAX);
        xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->link,(char *)key,CHARMAX);
        xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"pubDate") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->pubdate,(char *)key,CHARMAX);
        xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"description") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        char *stripped = _strip_html((char *)key);
        if(stripped != NULL) {
          strncpy(ri->desc,(char *)stripped,CHARMAX);
          free(stripped);
        }
        xmlFree(key);
      }
    }
    cur = cur->next;
  }
  if(r->first == NULL) {
    r->first = ri;
    r->last = ri;
  }
  r->last->next=ri;
  r->last = ri;
  r->articles++;
}

static void _parse_items_atom(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur) {
  xmlChar *key;
  rss_item_t *ri;

  ri = malloc(sizeof(rss_item_t));
  ri->next = NULL;
  // Drilling down into the item children
  cur = cur->xmlChildrenNode;
  cur = cur->next;
  // Cycle through the item child elements, dump these values into our rss item
  while (cur != NULL) {
    if (xmlStrcmp(cur->name, (const xmlChar *)"title") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->title,(char *)key,CHARMAX);
        xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
      key = xmlGetProp(cur,(const xmlChar *)"href");
      if (key != NULL) {
        strncpy(ri->link,(char *)key,CHARMAX);
        xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"updated") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->pubdate,(char *)key,CHARMAX);
        xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"summary") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        char *stripped = _strip_html((char *)key);
        if(stripped != NULL) {
          strncpy(ri->desc,(char *)stripped,CHARMAX);
          free(stripped);
        }
        xmlFree(key);
      }
    }
    cur = cur->next;
  }
  if(r->first == NULL) {
    r->first = ri;
    r->last = ri;
  }
  r->last->next=ri;
  r->last = ri;
  r->articles++;
}

static void _parse_channel_rss(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur) {
  xmlChar *key;
  while (cur != NULL) {
    if (xmlStrcmp(cur->name, (const xmlChar *)"title") == 0) {
     key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if (key != NULL) {
      strncpy(r->title,(char *)key,CHARMAX);
      xmlFree(key);
     }
    } else if  (xmlStrcmp(cur->name, (const xmlChar *)"description") == 0) {
     // Should html be stripped from channel descriptions?
     key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if (key != NULL) {
       strncpy(r->desc,(char *)key,CHARMAX);
       xmlFree(key);
     }
    } else if  (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
     key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if (key != NULL) {
       strncpy(r->link,(char *)key,CHARMAX);
       xmlFree(key);
     }
    }
    cur=cur->next;
  }
}

static void _parse_channel_atom(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur) {
  xmlChar *key;
  cur = cur->next;
  while(cur != NULL) {
    if (xmlStrcmp(cur->name, (const xmlChar *)"title") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
       strncpy(r->title,(char *)key,CHARMAX);
       xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"subtitle") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
       strncpy(r->link,(char *)key,CHARMAX);
       xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
      xmlChar *link_type;
      link_type = xmlGetProp(cur,(const xmlChar *)"rel");
      // We only care about the self link, not the alternate link
      if(xmlStrcmp(link_type, (const xmlChar *)"self") == 0) {
        key = xmlGetProp(cur,(const xmlChar *)"href");
        if (key != NULL) {
          strncpy(r->link,(char *)key,CHARMAX);
          xmlFree(key);
        }
      }
      if(link_type)
        xmlFree(link_type);
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"updated") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
       strncpy(r->pubdate,(char *)key,CHARMAX);
       xmlFree(key);
      }
    } else if (xmlStrcmp(cur->name, (const xmlChar *)"entry") == 0) {
      _parse_items_atom(r,doc,cur);
    }
    cur = cur->next;
  }
}


// Grabs the title, link, and description of an RSS reed, then calls
// _parse_items_rss to handle the linked list
// TODO:  Find the correct way to dynamically support namespaces
static void _parse_feed(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur) {
  //TODO:  Clean this horrible mess up!
  if  (xmlStrcmp(cur->name, (const xmlChar *)"feed") == 0)
    _parse_channel_atom(r,doc,cur->xmlChildrenNode);
  else {

  // Channel
  cur = cur->xmlChildrenNode;
  // HACK:  This deals with xml files that have namespaces
  if(cur->xmlChildrenNode == NULL)
    cur = cur->next;
  // This is a really bad way to find out if the current
  // xml file being parsed has a namespace.  If so, I assume
  // it is slashdot, and parse the channel info from the child
  // _parse_items should be handling this
  if(cur->ns != NULL) {
    _parse_channel_rss(r,doc,cur->xmlChildrenNode);
  }
  else {
    cur = cur->xmlChildrenNode;
    _parse_channel_rss(r,doc,cur);
  }
  // Now we will populate the feed information
  while (cur != NULL) {
    if  (xmlStrcmp(cur->name, (const xmlChar *)"item") == 0)
      _parse_items_rss(r, doc, cur);
    cur=cur->next;
  }
  }
}

// public function, returns an rss feed item
// reload arg will determine if the feed needs to simply
// be loaded, or if it needs to be reloaded
rss_feed_t *
load_feed(char *url, int reload,
          rss_feed_t *feed, const int auth,
          const char *username, const char *password)
{
  rss_feed_t *rf = NULL;
  xmlDocPtr doc;
  xmlNodePtr cur;
  struct MemoryStruct buffer;

  if (reload == TRUE) {
    // Free old articles
    rf = feed;
    _free_items(rf);
  // Here we are going to load the url into memory
    buffer = _load_url(rf->url, auth, username, password);
  } else
    buffer = _load_url(url, auth, username, password);

  // if an error ocurred during curl shit, exit
  // XXX: this is ugly!
  if (buffer.errored)
      return NULL;

  // do xml parsing
  doc = xmlReadMemory(buffer.memory,buffer.size,"noname.xml",NULL,0);
  if (doc == NULL) {
    fprintf(stderr,"Unable to parse xml from %s\n",url);
    return NULL;
  }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL) {
    fprintf(stderr,"empty document\n");
    xmlFreeDoc(doc);
    return NULL;
  }

  if (reload == FALSE) {
    rf = malloc(sizeof(rss_feed_t));
    assert(rf != NULL);
    strncpy(rf->url,url,strlen(url)+1);
  }

  // TODO: Not clean
  rf->first = NULL;
  rf->articles = 0;
  _parse_feed(rf, doc, cur);

  if(buffer.memory)
    free(buffer.memory);
  if(doc)
    xmlFreeDoc(doc);

  return rf;
}

// public function to print an rss feed
void print_feed(rss_feed_t *r) {
  rss_item_t *ri;
  int article_number = 1;

  printf("Title: %s\n", r->title);
  printf("Link: %s\n", r->link);
  printf("Description: %s\n", r->desc);
  printf("Publication Date: %s\n", r->pubdate);
  printf("------------------------------------------------------------------------------\n");
  ri = r->first;
  while(ri != NULL) {
    printf("%03d: %s\n",article_number++, ri->title);
    printf("\t%s\n",ri->desc);
    ri = ri->next;
  }
}

// public function to cleanup an rss feed
void free_feed(rss_feed_t *rf) {
    if(rf != NULL){
        if(rf->first != NULL)
            _free_items(rf);
        free(rf);
    }
}

rss_item_t * get_item(rss_feed_t *rf, int index) {
  rss_item_t *ri = rf->first;
  for(int i=0;i<index && ri->next != NULL;i++)
  ri = ri->next;
  return ri;
}
