#ifndef COMMON_H
#define COMMON_H

#define TRUE 1
#define FALSE 0

#define RC_NAME ".rssreaderrc"

/* I'm making this a structure so that we can
   support other kind of proxies in the future. */
typedef struct {
    char *http;
} proxy_config;

proxy_config proxies;

#endif
