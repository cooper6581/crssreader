#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>
#include <assert.h>

#include "rss.h"

// Calback used by libcurl
static size_t
WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
{
  size_t realsize = size * nmemb;
  struct MemoryStruct *mem = (struct MemoryStruct *)data;

  mem->memory = realloc(mem->memory, mem->size + realsize + 1);
  if (mem->memory == NULL) {
    /* out of memory! */
    printf("not enough memory (realloc returned NULL)\n");
    exit(EXIT_FAILURE);
  }

  memcpy(&(mem->memory[mem->size]), ptr, realsize);
  mem->size += realsize;
  mem->memory[mem->size] = 0;

  return realsize;
}

// Internal function used to load url to memory
// TODO:  Have the curl stuff be global so we can use the same
// handle for the entire duration of the program
static struct MemoryStruct
_load_url(char *url)
{
  CURL *curl_handle;
  struct MemoryStruct chunk;

  chunk.memory = malloc(1);
  chunk.size = 0;
  curl_global_init(CURL_GLOBAL_ALL);
  /* init the curl session */
  curl_handle = curl_easy_init();
  /* specify URL to get */
  curl_easy_setopt(curl_handle, CURLOPT_URL, url);
  /* send all data to this function  */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEFUNCTION, WriteMemoryCallback);
  /* we pass our 'chunk' struct to the callback function */
  curl_easy_setopt(curl_handle, CURLOPT_WRITEDATA, (void *)&chunk);
  /* some servers don't like requests that are made without a user-agent
     field, so we provide one */
  curl_easy_setopt(curl_handle, CURLOPT_USERAGENT, "libcurl-agent/1.0");
  /* get it! */
  curl_easy_perform(curl_handle);
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
  printf("%lu bytes retrieved\n", (long)chunk.size);

  /* we're done with libcurl, so clean it up */
  curl_global_cleanup();

  return chunk;
}

// free's the rss item linked list
static void
_free_items(rss_feed_t *r)
{
  int i = 0;
  rss_item_t *ri;
  rss_item_t *temp;
  ri = r->first;
  while(ri != NULL) {
#ifdef DEBUG
    printf("Freeing article %02d\n",i);
#endif
    temp = ri->next;
    free(ri);
    ri=temp;
    i++;
  }
}

// Populates the rss item linked list
static void
_parse_items(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur)
{
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
    }
    else if (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->link,(char *)key,CHARMAX);
        xmlFree(key);
      }
    }
    else if (xmlStrcmp(cur->name, (const xmlChar *)"pubDate") == 0) {
      key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
      if (key != NULL) {
        strncpy(ri->pubdate,(char *)key,CHARMAX);
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

// Grabs the title, link, and description of an RSS reed, then calls
// _parse_items to handle the linked list
// TODO:  Find the correct way to dynamically support namespaces
static void
_parse_feed(rss_feed_t *r, xmlDocPtr doc, xmlNodePtr cur)
{
  xmlChar *key;

  // Channel
  cur = cur->xmlChildrenNode;
  // HACK:  This deals with xml files that have namespaces
  if(cur->xmlChildrenNode == NULL)
    cur = cur->next;
  // Should be the ticket!
  cur = cur->xmlChildrenNode;
  // Now we will populate the feed information
  while (cur != NULL) {
    if (xmlStrcmp(cur->name, (const xmlChar *)"title") == 0) {
     key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if (key != NULL) {
      strncpy(r->title,(char *)key,CHARMAX);
      xmlFree(key);
     }
    }
    else if  (xmlStrcmp(cur->name, (const xmlChar *)"description") == 0) {
     key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if (key != NULL) {
       strncpy(r->desc,(char *)key,CHARMAX);
       xmlFree(key);
     }
    }
    else if  (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
     key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
     if (key != NULL) {
       strncpy(r->link,(char *)key,CHARMAX);
       xmlFree(key);
     }
    }
    else if  (xmlStrcmp(cur->name, (const xmlChar *)"item") == 0)
      _parse_items(r, doc, cur);

    cur=cur->next;
  }
}

// public function, returns an rss feed item
rss_feed_t *
load_feed(char *url)
{
  rss_feed_t *rf = NULL;
  xmlDocPtr doc;
  xmlNodePtr cur;
  struct MemoryStruct buffer;

  // Here we are going to load the url into memory
  buffer = _load_url(url);
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

  rf = malloc(sizeof(rss_feed_t));

  assert(rf != NULL);

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
void
print_feed(rss_feed_t *r)
{
  rss_item_t *ri;
  int article_number = 1;

  printf("Title: %s\n", r->title);
  printf("Link: %s\n", r->link);
  printf("Description: %s\n", r->desc);
  printf("------------------------------------------------------------------------------\n");
  ri = r->first;
  while(ri != NULL) {
    printf("%03d: %s\n",article_number++, ri->title);
    ri = ri->next;
  }
}

// public function to cleanup an rss feed
void
free_feed(rss_feed_t *rf)
{
  if(rf->first != NULL)
    _free_items(rf);
}

rss_item_t *
get_item(rss_feed_t *rf, int index)
{
  rss_item_t *ri = rf->first;
  for(int i=0;i<index && ri->next != NULL;i++)
  ri = ri->next;
  return ri;
}
