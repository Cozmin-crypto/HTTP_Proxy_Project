#include "listener.h"


#include <arpa/inet.h>
#include <netinet/in.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>


#include "common/log.h"


int listener_start(const char *ip, int port)
{
    //creeam socketul TCP
    int fd = socket(AF_INET, SOCK_STREAM, 0);
    if(fd<0)
    {
        perror("socket");
        return -1;
    }

    //permite repornirea rapida pe acelasi port
    int yes = 1;
    setsockopt(fd, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(yes));



    //adresa pe care ascultam
    struct sockaddr_in addr;
    memset(&addr, 0, sizeof(addr));
    addr.sin_family = AF_INET;
    addr.sin_port = htons((unsigned short)port);
    if(inet_pton(AF_INET, ip, &addr.sin_addr) != 1)
    {
        log_line("adresa IP invalida: %s", ip);
        close(fd);
        return -1;
    }


    //legam socketul de adresa ip si portul

    if(bind(fd, (struct sockaddr *)&addr, sizeof addr) < 0)
    {
        perror("bind");
        close(fd);
        return -1;
    }


    //incepe ascultarea, coada de maxim 32 de conexiuni in asteptare 

    if(listen(fd, 32) < 0)
    {
        perror("listen");
        close(fd);
        return -1;
    }

    return fd;
}


int listener_accept(int listen_fd)
{
    return accept(listen_fd, NULL, NULL);
}