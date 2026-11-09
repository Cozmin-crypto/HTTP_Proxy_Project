#include "client_handler.h"

#include <stdlib.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

#include "client/request_parser.h"
#include "common/http.h"
#include "common/log.h"

static void send_response(int fd, HttpResponse *resp) {
    size_t n;
    char *out = http_serialize_response(resp, &n);
    if (out) {
        send_all(fd, out, n);
        free(out);
    }
}

static void send_error(int fd, int code, const char *reason, const char *msg) {
    HttpResponse *resp = calloc(1, sizeof *resp);
    if (!resp) return;
    response_make_error(resp, code, reason, msg);
    send_response(fd, resp);
    response_free(resp);
    free(resp);
}

void handle_client(int fd) {
    HttpRequest  *req  = calloc(1, sizeof *req);
    HttpResponse *resp = calloc(1, sizeof *resp);
    if (!req || !resp) goto done;

    if (request_parse(fd, req) < 0) {
        send_error(fd, 400, "Bad Request", "Cerere HTTP invalida");
        goto done;
    }
    log_line("[REQ ] %s %s", req->method, req->url);

    if (strcasecmp(req->method, "GET") != 0) {
        send_error(fd, 501, "Not Implemented", "Etapa 1: doar GET");
        goto done;
    }

    if (upstream_fetch(req, resp) != UPSTREAM_OK) {
        send_error(fd, 502, "Bad Gateway", "Nu pot comunica cu serverul");
        goto done;
    }

    send_response(fd, resp);
    log_line("[RESP] %d %s (%zu octeti)", resp->status_code, req->host, resp->body_len);

done:
    if (req)  request_free(req);
    if (resp) response_free(resp);
    free(req);
    free(resp);
    close(fd);
}
