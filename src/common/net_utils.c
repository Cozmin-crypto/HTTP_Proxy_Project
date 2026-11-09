#include "net_utils.h"

#include <errno.h>
#include <string.h>
#include <sys/socket.h>
#include <sys/time.h>

int send_all(int fd, const void *buf, size_t len) {
    const char *p = buf;
    while (len > 0) {
        ssize_t n = send(fd, p, len, MSG_NOSIGNAL);
        if (n < 0) {
            if (errno == EINTR) continue;
            return -1;
        }
        p   += n;
        len -= (size_t)n;
    }
    return 0;
}

int set_timeouts(int fd, int seconds) {
    struct timeval tv = { .tv_sec = seconds, .tv_usec = 0 };
    if (setsockopt(fd, SOL_SOCKET, SO_RCVTIMEO, &tv, sizeof tv) < 0) return -1;
    if (setsockopt(fd, SOL_SOCKET, SO_SNDTIMEO, &tv, sizeof tv) < 0) return -1;
    return 0;
}

void reader_init(Reader *r, int fd) {
    r->fd  = fd;
    r->pos = 0;
    r->len = 0;
}

ssize_t reader_read(Reader *r, void *out, size_t n) {
    if (r->pos == r->len) {
        ssize_t got;
        do {
            got = recv(r->fd, r->buf, sizeof r->buf, 0);
        } while (got < 0 && errno == EINTR);
        if (got <= 0) return got;           /* 0 = EOF, -1 = eroare (inclusiv timeout) */
        r->pos = 0;
        r->len = (size_t)got;
    }
    size_t avail = r->len - r->pos;
    if (n > avail) n = avail;
    memcpy(out, r->buf + r->pos, n);
    r->pos += n;
    return (ssize_t)n;
}

int reader_read_exact(Reader *r, void *out, size_t n) {
    char *p = out;
    while (n > 0) {
        ssize_t g = reader_read(r, p, n);
        if (g <= 0) return -1;
        p += g;
        n -= (size_t)g;
    }
    return 0;
}

int reader_read_line(Reader *r, char *line, size_t cap) {
    size_t i = 0;
    for (;;) {
        char c;
        ssize_t g = reader_read(r, &c, 1);
        if (g <= 0) return -1;
        if (c == '\n') break;
        if (i + 1 >= cap) return -1;        /* linie prea lunga */
        line[i++] = c;
    }
    if (i > 0 && line[i - 1] == '\r') i--;
    line[i] = '\0';
    return 0;
}
