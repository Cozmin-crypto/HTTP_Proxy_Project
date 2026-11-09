#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

#include "client/client_handler.h"
#include "client/listener.h"
#include "common/log.h"

/* Etapa 1: proxy simplu, un singur client pe rand, doar GET.
   Folosire: ./proxy [port] [ip]    (implicit 8080 si 127.0.0.1) */
int main(int argc, char **argv) {
    int port = (argc > 1) ? atoi(argv[1]) : 8080;
    const char *ip = (argc > 2) ? argv[2] : "127.0.0.1";

    signal(SIGPIPE, SIG_IGN);

    int lfd = listener_start(ip, port);
    if (lfd < 0) return 1;
    log_line("Proxy pornit pe %s:%d", ip, port);

    for (;;) {
        int c = listener_accept(lfd);
        if (c < 0) continue;
        handle_client(c);
    }
    close(lfd);
    return 0;
}