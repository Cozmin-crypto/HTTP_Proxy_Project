#ifndef REQUEST_PARSER_H
#define REQUEST_PARSER_H

#include "common/http.h"

/* Citeste cererea de la browser de pe socketul fd si completeaza req.
   Returneaza 0 daca e ok, -1 daca cererea e gresita sau incompleta. */
int request_parse(int fd, HttpRequest *req);

#endif
