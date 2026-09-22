#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <arpa/inet.h>

const int PORT=8080;

int main(){
    int sockfd;
    if((sockfd=socket(AF_INET, SOCK_STREAM, 0))==-1){
        fprintf(stderr, "Failed to create socket\n");
        return 1;
    }

    int opt=1;
    if(setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt))==-1){
        fprintf(stderr, "Failed to set socket option\n");
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;

    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY);

    if(bind(sockfd, (struct sockaddr*)&server_addr, sizeof(server_addr))==-1){
        fprintf(stderr, "Failed to bind server address to socket\n");
        return 1;
    }

    if(listen(sockfd, 5)==-1){
        fprintf(stderr, "Server unable to listen to requests\n");
        return 1;
    }

    while(1){
        int connfd;
        socklen_t client_len=sizeof(client_addr);
        if((connfd=accept(sockfd, (struct sockaddr*)&client_addr, &client_len))==-1){
            fprintf(stderr, "Failed to accept connection\n");
            return 1;
        }

        ssize_t  bytes_read;
        char buff[4096];
        char *method, *path, *version, *fspath;
        method=(char*)malloc(16);
        path=(char*)malloc(1024);
        fspath=(char*)malloc(1031); // file system path
        version=(char*)malloc(16);

        while((bytes_read=read(connfd, buff, sizeof(buff)))>0){
            for(int i=0;i<bytes_read;i++){
                printf("%c", buff[i]);
            }

            // structure of a HTTP 1.0 request
            // <METHOD> <REQUEST-URI> HTTP/1.0\r\n
            // <Header-Name>: <Header-Value>\r\n
            // <Header-Name>: <Header-Value>\r\n
            // \r\n
            // [Optional Entity Body]

            sscanf(buff, "%s %s %s\r\n", method, path, version);
            printf("Method: %s\n", method);
            printf("HTTP Path: %s\n", path);
            printf("Version: %s\n", version);
        
            snprintf(fspath, 1031, "./www%s", path);
            printf("Filesystem path: %s\n", fspath);

            int fd;
            if((fd=open(fspath, O_RDONLY))==-1){
                fprintf(stderr, "Failed to open the file fd: %d\n", fd);
                return 1;
            }
            printf("fd: %d\n", fd);

            ssize_t bytes_read;
            char buffer[1024];
            while((bytes_read=read(fd, buffer, sizeof(buffer)))>0){
                write(STDOUT_FILENO, buffer, bytes_read);
            }

            // if(write(connfd, buff, bytes_read)==-1){
            //     fprintf(stderr, "Failed to write to client\n");
            //     return 1;
            // }
        }

    }


    return 0;
}