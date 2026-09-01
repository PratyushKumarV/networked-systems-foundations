#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>

const int PORT=8080;

int main(){
    int sockfd;
    if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1){
        fprintf(stderr, "Failed to create socket");
        return 1;
    }

    int opt=1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==-1){
        fprintf(stderr, "Failed to set socket option");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){
        fprintf(stderr, "Failed to bind server address to socket");
        return 1;
    }

    if(listen(sockfd, 5)==-1){
        fprintf(stderr, "Server unable to listen to requests");
        return 1;
    }

    while(1){
        int connfd;
        socklen_t client_len=sizeof(client_addr);
        if((connfd=accept(sockfd, (struct sockaddr*)&client_addr, &client_len))==-1){
            fprintf(stderr, "Failed to accept connection");
            return 1;
        }
    }


    return 0;
}