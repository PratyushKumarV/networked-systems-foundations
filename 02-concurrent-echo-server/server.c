#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include <stdint.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>


const uint16_t PORT=8080;

void *handle_client(void * args);
typedef struct{
    int connfd;
    struct sockaddr_in client_addr;
    char client_ip[INET_ADDRSTRLEN];
}client_args;

int main(){
    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd==-1){
        perror("Error creating socket!");
        return 1;
    }

    int opt=1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==-1){
        perror("setsockop SO_REUSEADDR failed!");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    /*
    struct sockaddr_in {
        sa_family_t     sin_family;     // AF_INET 
        in_port_t       sin_port;       // Port number 
        struct in_addr  sin_addr;       // IPv4 address
    };

    */

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
        perror("Error binding the socket with the socket address!");
        close(sockfd);
        return 1;
    }

    if(listen(sockfd, 5)==-1){
        perror("Error listening on socket!");
        close(sockfd);
        return 1;
    }

    while(1){
        socklen_t client_len=sizeof(client_addr);
        int connfd=accept(sockfd, (struct sockaddr*)&client_addr, &client_len);
        if(connfd==-1){
            perror("Error accepting connection!");
            continue;
        }

        char client_ip[INET_ADDRSTRLEN];
        inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

        printf("Connection to %s:%d succeeded\n", client_ip, ntohs(client_addr.sin_port));

        // Concurrent execution through threads
        pthread_t client;
        client_args* args = (client_args*)malloc(sizeof(client_args));
        args->connfd = connfd;
        args->client_addr = client_addr;
        strncpy(args->client_ip, client_ip, sizeof(args->client_ip)-1);
        (args->client_ip)[sizeof(args->client_ip)-1]='\0';

        int ret;

        if((ret=pthread_create(&client, NULL, handle_client, (void*)args))!=0){
            fprintf(stderr, "Error creating client thread!: %s\n", strerror(ret));
            close(connfd);
            free(args);
            continue;
        }

        pthread_detach(client);

    }


    return 0;
}

void *handle_client(void * args){
    char buffer[1024];
    ssize_t bytes_read;
    int write_flag=0;
    client_args *params=(client_args *)args;

    while((bytes_read=read(params->connfd, buffer, sizeof(buffer)))>0){
        if(write(params->connfd, buffer, bytes_read)==-1){
            write_flag=1;
            break;
        }
    }

    if(write_flag){
        perror("Error writing to client!");
    }
    else if(bytes_read==-1){
        perror("Error reading from client!");
    }else{
        printf("Client %s:%d disconnected gracefully.\n", params->client_ip, ntohs(params->client_addr.sin_port));
    }

    
    close(params->connfd);
    free(params);
    return NULL;
}