#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>

#define SIZE 1031

typedef struct{
    char *key;
    char *value;
    node *next;
}node;

unsigned long hash(char *str);

const int PORT=8080;

int main(){
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if(sockfd==-1){
        fprintf(stderr, "Failed to open socket");
        return 1;
    }

    int opt=1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==-1){
        fprintf(stderr, "Failed to set socket options");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){
        fprintf(stderr, "Failed to bind socket");
        close(sockfd);
        return 1;
    }

    if(listen(sockfd, 5)==-1){
        fprintf(stderr, "Failed to listen");
        close(sockfd);
        return 1;
    }

    while(1){
        int connfd;
        socklen_t client_len=sizeof(client_addr);
        if((connfd=accept(sockfd, (struct sockaddr*)&client_addr, &client_len))==-1){
            fprintf(stderr, "Failed to accept connect");
            continue;
        }


        char buff[1024];
        ssize_t bytes_read;
        
        while((bytes_read=read(connfd, buff, sizeof(buff)-1))>0){

            
        }


    }
    close(sockfd);

    return 0;
}

unsigned long hash(char *str){
    unsigned long h=5381;
    for(int i=0;i<=strlen(str);i++){
        
    }
}