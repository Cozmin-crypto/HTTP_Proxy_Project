#include "http.h"

#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <strings.h>

/* ======================= headere ======================= */

const char *header_get(const HttpHeader *h, int count, const char *name) {
    for (int i = 0; i < count; i++)
        if (strcasecmp(h[i].name, name) == 0) return h[i].value;
    return NULL;
}

int header_set(HttpHeader *h, int *count, const char *name, const char *value) {
    for (int i = 0; i < *count; i++) {
        if (strcasecmp(h[i].name, name) == 0) {
            snprintf(h[i].value, MAX_VALUE, "%s", value);
            return 0;
        }
    }
    if (*count >= MAX_HEADERS) return -1;
    snprintf(h[*count].name, MAX_NAME, "%s", name);
    snprintf(h[*count].value, MAX_VALUE, "%s", value);
    (*count)++;
    return 0;
}

void header_remove(HttpHeader *h, int *count, const char *name) {
    int j = 0;
    for (int i = 0; i < *count; i++) {
        if (strcasecmp(h[i].name, name) == 0) continue;
        if (i != j) h[j] = h[i];
        j++;
    }
    *count = j;
}

/* "Nume: valoare" pana la linia goala */
int http_read_headers(Reader *r, HttpHeader *h, int *count) {
    char line[MAX_NAME + MAX_VALUE + 8];
    *count = 0;
    for (;;) {
        if (reader_read_line(r, line, sizeof line) < 0) return -1;
        if (line[0] == '\0') return 0;

        char *colon = strchr(line, ':');
        if (!colon) return -1;
        *colon = '\0';
        if (strlen(line) >= MAX_NAME || *count >= MAX_HEADERS) return -1;

        char *value = colon + 1;
        while (*value == ' ' || *value == '\t') value++;

        memcpy(h[*count].name, line, strlen(line) + 1);   /* lungimea e verificata mai sus */
        snprintf(h[*count].value, MAX_VALUE, "%s", value);
        (*count)++;
    }
}

/* Dupa ce body-ul e decodat sau modificat: scoate chunked si pune Content-Length corect */
void http_sync_body_headers(HttpHeader *h, int *count, size_t body_len) {
    char tmp[32];
    header_remove(h, count, "Transfer-Encoding");
    snprintf(tmp, sizeof tmp, "%zu", body_len);
    header_set(h, count, "Content-Length", tmp);
}

/* ======================= URL ======================= */

/* "http://host:port/cale?x=1" -> host, port, path */
int http_parse_url(const char *url, char *host, size_t hostcap, int *port,
                   char *path, size_t pathcap) {
    if (strncasecmp(url, "http://", 7) != 0) return -1;
    const char *p = url + 7;

    const char *slash = strchr(p, '/');
    size_t hplen = slash ? (size_t)(slash - p) : strlen(p);
    char hp[300];
    if (hplen == 0 || hplen >= sizeof hp) return -1;
    memcpy(hp, p, hplen);
    hp[hplen] = '\0';

    *port = 80;
    char *colon = strrchr(hp, ':');
    if (colon) {
        *colon = '\0';
        int pt = atoi(colon + 1);
        if (pt <= 0 || pt > 65535) return -1;
        *port = pt;
    }
    if (hp[0] == '\0' || strlen(hp) >= hostcap) return -1;
    snprintf(host, hostcap, "%s", hp);

    const char *pp = slash ? slash : "/";
    if (strlen(pp) >= pathcap) return -1;
    snprintf(path, pathcap, "%s", pp);
    return 0;
}

/* ======================= citirea body-ului ======================= */

static int grow(char **b, size_t *cap, size_t need) {
    if (need + 1 <= *cap) return 0;
    size_t nc = *cap ? *cap : 8192;
    while (nc < need + 1) nc *= 2;
    char *nb = realloc(*b, nc);
    if (!nb) return -1;
    *b = nb;
    *cap = nc;
    return 0;
}

int http_read_until_close(Reader *r, char **body, size_t *len) {
    char *b = NULL;
    size_t cap = 0, n = 0;
    if (grow(&b, &cap, 0) < 0) return -1;
    for (;;) {
        if (grow(&b, &cap, n + 4096) < 0 || n + 4096 > MAX_BODY) { free(b); return -1; }
        ssize_t g = reader_read(r, b + n, cap - n - 1);
        if (g < 0) { free(b); return -1; }
        if (g == 0) break;
        n += (size_t)g;
    }
    b[n] = '\0';
    *body = b;
    *len = n;
    return 0;
}

static int read_chunked(Reader *r, char **body, size_t *len) {
    char *b = NULL;
    size_t cap = 0, n = 0;
    char line[256];
    if (grow(&b, &cap, 0) < 0) return -1;

    for (;;) {
        if (reader_read_line(r, line, sizeof line) < 0) goto fail;
        char *end;
        unsigned long sz = strtoul(line, &end, 16);   /* ignora extensiile dupa ';' */
        if (end == line) goto fail;
        if (sz == 0) break;
        if (n + sz > MAX_BODY || grow(&b, &cap, n + sz) < 0) goto fail;
        if (reader_read_exact(r, b + n, sz) < 0) goto fail;
        n += sz;
        if (reader_read_line(r, line, sizeof line) < 0) goto fail;  /* CRLF dupa chunk */
    }
    for (;;) {                                        /* trailere, pana la linia goala */
        if (reader_read_line(r, line, sizeof line) < 0) goto fail;
        if (line[0] == '\0') break;
    }
    b[n] = '\0';
    *body = b;
    *len = n;
    return 0;
fail:
    free(b);
    return -1;
}

/* Alege singur metoda: chunked, Content-Length sau (daca until_close) pana la inchidere */
int http_read_body(Reader *r, const HttpHeader *h, int count, int until_close,
                   char **body, size_t *len) {
    *body = NULL;
    *len = 0;

    const char *te = header_get(h, count, "Transfer-Encoding");
    const char *cl = header_get(h, count, "Content-Length");

    if (te && strcasestr(te, "chunked")) return read_chunked(r, body, len);

    if (cl) {
        char *end;
        unsigned long n = strtoul(cl, &end, 10);
        if (end == cl || n > MAX_BODY) return -1;
        if (n == 0) return 0;
        char *b = malloc(n + 1);
        if (!b) return -1;
        if (reader_read_exact(r, b, n) < 0) { free(b); return -1; }
        b[n] = '\0';
        *body = b;
        *len = n;
        return 0;
    }
    if (until_close) return http_read_until_close(r, body, len);
    return 0;
}

/* ======================= serializare ======================= */

typedef struct { char *data; size_t len, cap; } Buf;

static int buf_add(Buf *b, const void *p, size_t n) {
    if (grow(&b->data, &b->cap, b->len + n) < 0) return -1;
    memcpy(b->data + b->len, p, n);
    b->len += n;
    b->data[b->len] = '\0';
    return 0;
}

static int buf_addf(Buf *b, const char *fmt, ...) {
    char tmp[MAX_NAME + MAX_VALUE + MAX_URL + 64];
    va_list ap;
    va_start(ap, fmt);
    int n = vsnprintf(tmp, sizeof tmp, fmt, ap);
    va_end(ap);
    if (n < 0 || (size_t)n >= sizeof tmp) return -1;
    return buf_add(b, tmp, (size_t)n);
}

static int skip_header(const char *name) {
    static const char *skip[] = { "Connection", "Proxy-Connection", "Keep-Alive",
        "Proxy-Authenticate", "Proxy-Authorization", "TE", "Trailer", "Upgrade",
        "Transfer-Encoding", "Content-Length", NULL };
    for (int i = 0; skip[i]; i++)
        if (strcasecmp(name, skip[i]) == 0) return 1;
    return 0;
}

static int add_headers(Buf *b, const HttpHeader *h, int count, size_t body_len) {
    for (int i = 0; i < count; i++) {
        if (skip_header(h[i].name)) continue;
        if (buf_addf(b, "%s: %s\r\n", h[i].name, h[i].value) < 0) return -1;
    }
    if (buf_addf(b, "Connection: close\r\n") < 0) return -1;

    if (body_len > 0) {
        if (buf_addf(b, "Content-Length: %zu\r\n", body_len) < 0) return -1;
    } else {
        /* fara body: pastram Content-Length original (ex. raspuns la HEAD) */
        const char *cl = header_get(h, count, "Content-Length");
        if (cl && buf_addf(b, "Content-Length: %s\r\n", cl) < 0) return -1;
    }
    return buf_add(b, "\r\n", 2);
}

/* Cererea spre server: URL relativ, fara headere hop-by-hop, Connection: close */
char *http_serialize_request(const HttpRequest *req, size_t *len) {
    Buf b = {0};
    if (buf_addf(&b, "%s %s HTTP/1.1\r\n", req->method, req->path) < 0 ||
        add_headers(&b, req->headers, req->header_count, req->body_len) < 0 ||
        (req->body_len > 0 && buf_add(&b, req->body, req->body_len) < 0)) {
        free(b.data);
        return NULL;
    }
    *len = b.len;
    return b.data;
}

/* Raspunsul spre browser */
char *http_serialize_response(const HttpResponse *resp, size_t *len) {
    Buf b = {0};
    if (buf_addf(&b, "HTTP/1.1 %d %s\r\n", resp->status_code, resp->reason) < 0 ||
        add_headers(&b, resp->headers, resp->header_count, resp->body_len) < 0 ||
        (resp->body_len > 0 && buf_add(&b, resp->body, resp->body_len) < 0)) {
        free(b.data);
        return NULL;
    }
    *len = b.len;
    return b.data;
}

/* ======================= eliberare / erori ======================= */

void request_free(HttpRequest *req) {
    free(req->body);
    req->body = NULL;
    req->body_len = 0;
}

void response_free(HttpResponse *resp) {
    free(resp->body);
    resp->body = NULL;
    resp->body_len = 0;
}

void response_make_error(HttpResponse *resp, int code, const char *reason, const char *msg) {
    response_free(resp);
    memset(resp, 0, sizeof *resp);

    snprintf(resp->version, sizeof resp->version, "HTTP/1.1");
    resp->status_code = code;
    snprintf(resp->reason, sizeof resp->reason, "%s", reason);

    size_t n = strlen(msg) + 2;
    resp->body = malloc(n);
    if (resp->body) {
        snprintf(resp->body, n, "%s\n", msg);
        resp->body_len = strlen(resp->body);
    }
    header_set(resp->headers, &resp->header_count, "Content-Type", "text/plain; charset=utf-8");
    http_sync_body_headers(resp->headers, &resp->header_count, resp->body_len);
}
