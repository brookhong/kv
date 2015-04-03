#ifndef _HTTPSERVER_H
#define _HTTPSERVER_H

typedef struct _HTTPD{
    const char * idxFileName;
    const char * port;
}HTTPD;

void _httpServer(void *pv);

#endif
