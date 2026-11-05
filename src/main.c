#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>



#include "client/listener.h"
#include "common/log.h"
#include "server/upstream.h"



/*facem conexiunea client server(proxy) ; server(proxy) -> server real
 pe acest branch*/

//trebuie folosit: ./HTTP_Proxy_Project [port proxy] [ip server] [port server real]






int main(int argc, char** argv)
{
    int port;
    if(argc > 1)
        port = atoi(argv[1]);
    else
        port = 8080;    //valoare implicita 


    const char* host;
    if(argc > 2)
        host = strdup(argv[2]);
    else
        host = strdup("127.0.0.1");   //ip loopback implicit
        
    int server_port;
    if(argc > 3)
        server_port = atoi(argv[3]);
    else
        server_port = 80;   //valoare implicita    
    

    int lfd = listener_start("127.0.0.1", port);
    if(lfd < 0)
        return 1;
    
    log_line("Proxy pornit pe 127.0.0.1: %d", port);

    for(;;)
    {
        int client = listener_accept(lfd);
        if(client < 0)
            continue;
        log_line("Client conectat");

        int server = upstream_connect(host, server_port);
        if(server < 0)
        {
            log_line("Nu ma pot conecta la %s:%d", host, server_port);
        }
        else
        {
            log_line("Conectat la server %s:%d", host, server_port);
            close(server);
        }
        close(client);
    }
    return 0;
}