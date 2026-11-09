#ifndef CLIENT_HANDLER_H
#define CLIENT_HANDLER_H

/* Trateaza un client: citeste cererea, o trimite la server, intoarce raspunsul.
   Inchide socketul la final. */
void handle_client(int fd);

#endif
