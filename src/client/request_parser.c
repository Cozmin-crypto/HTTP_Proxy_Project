#include "request_parser.h"

#include <stdio.h>
#include <string.h>
#include <strings.h>

/* Etapa 1: citim linia de start si headerele. Body-ul vine in Etapa 2. */
int request_parse(int fd, HttpRequest *req) {
    Reader r;
    reader_init(&r, fd);

    char line[MAX_URL + 64];
    if (reader_read_line(&r, line, sizeof line) < 0) return -1;

    char method[16], url[MAX_URL], version[16];
    if (sscanf(line, "%15s %4095s %15s", method, url, version) != 3) return -1;

    snprintf(req->method, sizeof req->method, "%s", method);
    snprintf(req->url, sizeof req->url, "%s", url);
    snprintf(req->version, sizeof req->version, "%s", version);

    if (http_read_headers(&r, req->headers, &req->header_count) < 0) return -1;

    /* CONNECT host:port (HTTPS) - doar il recunoastem, handler-ul raspunde 501 */
    if (strcasecmp(method, "CONNECT") == 0) {
        char fake[MAX_URL + 16];
        snprintf(fake, sizeof fake, "http://%s", url);
        if (http_parse_url(fake, req->host, sizeof req->host, &req->port,
                           req->path, sizeof req->path) < 0) return -1;
        return 0;
    }

    /* Browserul catre proxy trimite URL absolut: GET http://host/cale HTTP/1.1.
       Daca vine doar calea (/cale), luam serverul din headerul Host. */
    if (url[0] == '/') {
        const char *h = header_get(req->headers, req->header_count, "Host");
        if (!h) return -1;
        int w = snprintf(req->url, sizeof req->url, "http://%s%s", h, url);
        if (w < 0 || (size_t)w >= sizeof req->url) return -1;
    }
    if (http_parse_url(req->url, req->host, sizeof req->host, &req->port,
                       req->path, sizeof req->path) < 0) return -1;

    if (!header_get(req->headers, req->header_count, "Host")) {
        char hv[300];
        if (req->port == 80) snprintf(hv, sizeof hv, "%s", req->host);
        else                 snprintf(hv, sizeof hv, "%s:%d", req->host, req->port);
        header_set(req->headers, &req->header_count, "Host", hv);
    }
    return 0;
}
