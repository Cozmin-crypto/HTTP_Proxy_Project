#ifndef HTTP_H
#define HTTP_H

#include <stddef.h>
#include "net_utils.h"

/* ---------- limite ---------- */
#define MAX_HEADERS 64
#define MAX_NAME    128
#define MAX_VALUE   4096
#define MAX_URL     4096
#define MAX_BODY    (32u * 1024u * 1024u)

/* ---------- structuri comune (CONTRACTUL dintre client_side si server_side) ---------- */
typedef struct {
    char name[MAX_NAME];
    char value[MAX_VALUE];
} HttpHeader;

typedef struct {
    char method[16];
    char url[MAX_URL];      /* exact cum a venit de la browser */
    char host[256];
    int  port;
    char path[MAX_URL];
    char version[16];
    HttpHeader headers[MAX_HEADERS];
    int  header_count;
    char  *body;            /* alocat cu malloc, NULL daca nu exista */
    size_t body_len;
} HttpRequest;

typedef struct {
    char version[16];
    int  status_code;
    char reason[128];
    HttpHeader headers[MAX_HEADERS];
    int  header_count;
    char  *body;            /* alocat cu malloc, NULL daca nu exista */
    size_t body_len;
} HttpResponse;

/* ---------- functia de legatura: A o apeleaza, B o implementeaza ---------- */
#define UPSTREAM_OK           0
#define UPSTREAM_ERR         (-1)   /* eroare generica */
#define UPSTREAM_ERR_DNS     (-2)   /* nu s-a gasit serverul */
#define UPSTREAM_ERR_CONNECT (-3)   /* conectare esuata */
#define UPSTREAM_ERR_TIMEOUT (-4)   /* serverul nu a raspuns la timp */

/* Trimite cererea la serverul real si completeaza resp.
   Returneaza UPSTREAM_OK sau un cod UPSTREAM_ERR_*.
   Daca cererea e blocata de reguli, returneaza OK cu un raspuns 403. */
int upstream_fetch(HttpRequest *req, HttpResponse *resp);

/* ---------- functii utile (implementate in http.c) ---------- */
const char *header_get(const HttpHeader *h, int count, const char *name);
int         header_set(HttpHeader *h, int *count, const char *name, const char *value);
void        header_remove(HttpHeader *h, int *count, const char *name);

int  http_parse_url(const char *url, char *host, size_t hostcap, int *port,
                    char *path, size_t pathcap);
int  http_read_headers(Reader *r, HttpHeader *h, int *count);
int  http_read_until_close(Reader *r, char **body, size_t *len);
int  http_read_body(Reader *r, const HttpHeader *h, int count, int until_close,
                    char **body, size_t *len);
void http_sync_body_headers(HttpHeader *h, int *count, size_t body_len);

char *http_serialize_request(const HttpRequest *req, size_t *len);
char *http_serialize_response(const HttpResponse *resp, size_t *len);

void request_free(HttpRequest *req);
void response_free(HttpResponse *resp);
void response_make_error(HttpResponse *resp, int code, const char *reason, const char *msg);

#endif
