#ifndef NET_UTILS_H
#define NET_UTILS_H

#include <stddef.h>
#include <sys/types.h>

int send_all(int fd, const void *buf, size_t len);
int set_timeouts(int fd, int seconds);

/* Cititor cu buffer peste un socket */
typedef struct {
    int    fd;
    char   buf[8192];
    size_t pos, len;
} Reader;

void    reader_init(Reader *r, int fd);
int     reader_read_line(Reader *r, char *line, size_t cap);  /* 0 ok, -1 eroare/EOF; scoate CRLF */
ssize_t reader_read(Reader *r, void *out, size_t n);          /* >0 octeti, 0 EOF, -1 eroare */
int     reader_read_exact(Reader *r, void *out, size_t n);    /* 0 ok, -1 eroare */

#endif
