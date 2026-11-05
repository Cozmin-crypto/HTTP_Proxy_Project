#ifndef LISTENER_H
#define LISTENER_H

//se asculta pe ip:port, se creeaza socketul sau eroare = -1
int listener_start(const char *ip, int port);


int listener_accept(int listen_fd);


#endif 