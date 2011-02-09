#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <curl/curl.h>
#include <libxml/xmlmemory.h>
#include <libxml/parser.h>

#include "rss.h"

// Calback used by libcurl
static size_t WriteMemoryCallback(void *ptr, size_t size, size_t nmemb, void *data)
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
static struct MemoryStruct _load_url(char *url) {
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

rss_feed load_feed(char *url) {
  rss_feed r;
  xmlDocPtr doc;
  xmlNodePtr cur;
  xmlChar *key;
  struct MemoryStruct buffer;

  buffer = _load_url(url);
  // do xml parsing
  doc = xmlReadMemory(buffer.memory,buffer.size,"noname.xml",NULL,0);
  if (doc == NULL) {
	fprintf(stderr,"Unable to parse xml from %s\n",url);
	return r;
  }

  cur = xmlDocGetRootElement(doc);

  if (cur == NULL) {
	fprintf(stderr,"empty document\n");
	xmlFreeDoc(doc);
	return r;
  }

  // TODO:
  // This needs to be a function
  printf("Root Name: %s\n",cur->name);
  // Channel
  cur = cur->xmlChildrenNode;
  // Should be the ticket!
  cur = cur->xmlChildrenNode;
  while (cur != NULL) {
	if (xmlStrcmp(cur->name, (const xmlChar *)"title") == 0) {
	  key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	  printf("Title: %s\n", key);
	  xmlFree(key);
	}
	else if  (xmlStrcmp(cur->name, (const xmlChar *)"description") == 0) {
	  key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	  printf("Description: %s\n", key);
	  xmlFree(key);
	}
	else if  (xmlStrcmp(cur->name, (const xmlChar *)"link") == 0) {
	  key = xmlNodeListGetString(doc, cur->xmlChildrenNode, 1);
	  printf("Link: %s\n", key);
	  xmlFree(key);
	}
	cur=cur->next;
  }
  
  if(buffer.memory)
	free(buffer.memory);
  if(doc)
	xmlFreeDoc(doc);
  return r;
}

