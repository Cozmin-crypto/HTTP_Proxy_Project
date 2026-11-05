#include "upstream.h"

#include <netdb.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

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