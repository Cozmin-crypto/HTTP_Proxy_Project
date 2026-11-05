#ifndef UPSTREAM_H
#define UPSTREAM_H

//se conecteaza la server : ("example.com", 80), returneaza socketul conectat sau -1 = eroare

int upstream_connect(const char *host, int port);


#endif