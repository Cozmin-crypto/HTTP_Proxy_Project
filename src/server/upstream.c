#include "upstream.h"

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <sys/socket.h>
#include <unistd.h>

#include "common/http.h"

int upstream_connect(const char *host, int port)
{
    char portstr[16];

    snprintf(portstr, sizeof portstr, "%d", port);

    //transforma dns ul in ip

    struct addrinfo hints, *res = NULL;
    memset(&hints, 0, sizeof hints);
    hints.ai_family = AF_UNSPEC;   //IPv4 sau IPv6
    hints.ai_socktype = SOCK_STREAM; //tcp
    if(getaddrinfo(host, portstr, &hints, &res))
        return -1;
    

    //incearca pe rand fiecare adresa pana reuseste una 
    int fd = -1;
    for(struct addrinfo *p = res; p; p = p->ai_next)
    {
        fd = socket(p->ai_family, p->ai_socktype, p->ai_protocol);
        if(fd < 0)
            continue;
        if(connect(fd, p->ai_addr, p->ai_addrlen) == 0)
            break;
        close(fd);
        fd = -1;
    }

    freeaddrinfo(res);
    return fd;
}

/* ---- Etapa 1: trimite cererea la server si citeste raspunsul ----
   Cererea se trimite cu Connection: close, iar raspunsul se citeste pana inchide serverul.
   Body-ul ramane exact cum a venit; parsarea completa (chunked etc.) vine in Etapa 2. */
int upstream_fetch(HttpRequest *req, HttpResponse *resp) {
    int fd = upstream_connect(req->host, req->port);
    if (fd < 0) return UPSTREAM_ERR_CONNECT;

    /* transforma cererea in text HTTP si o trimite */
    size_t n;
    char *msg = http_serialize_request(req, &n);
    if (!msg) { close(fd); return UPSTREAM_ERR; }
    int rc = send_all(fd, msg, n);
    free(msg);
    if (rc < 0) { close(fd); return UPSTREAM_ERR; }

    Reader r;
    reader_init(&r, fd);

    /* linia de status: "HTTP/1.1 200 OK" */
    char line[1024];
    if (reader_read_line(&r, line, sizeof line) < 0) { close(fd); return UPSTREAM_ERR; }

    char version[16];
    if (sscanf(line, "%15s %d %127[^\n]", version, &resp->status_code, resp->reason) < 2) {
        close(fd);
        return UPSTREAM_ERR;
    }
    snprintf(resp->version, sizeof resp->version, "%s", version);

    /* headerele, apoi body-ul pana la inchiderea conexiunii */
    if (http_read_headers(&r, resp->headers, &resp->header_count) < 0 ||
        http_read_until_close(&r, &resp->body, &resp->body_len) < 0) {
        close(fd);
        return UPSTREAM_ERR;
    }
    close(fd);
    return UPSTREAM_OK;
}