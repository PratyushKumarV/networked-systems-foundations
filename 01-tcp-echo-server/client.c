#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>
#include <string.h>
#include <unistd.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <arpa/inet.h>

const uint16_t PORT=8080;

int main(){

    int clientfd=socket(AF_INET, SOCK_STREAM, 0);
    if(clientfd==-1){
        perror("Error with creating socket!");
        return 1;
    }

    struct sockaddr_in server_addr;
    memset(&server_addr, 0, sizeof(server_addr));

    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    inet_pton(AF_INET, "127.0.0.1", &server_addr.sin_addr);

    if(connect(clientfd, (struct sockaddr *)&server_addr, sizeof(server_addr))==-1){ 
        perror("Error establishing connection");
        close(clientfd);
        return 1;
    }

    char send_buffer[1024], recv_buffer[1024];

    while(1){
        printf("Enter message to echo (type 'exit' to quit): ");
        fflush(stdout); // immediatley prints the contents of printf to the terminal

        // reads till a newline character is encountered
        if(fgets(send_buffer, sizeof(send_buffer), stdin)==NULL){ // handles Ctrl+D (EOF)
            close(clientfd);
            break;
        } 

        // input string has to be stripped of trailing \n characters, strcspn returns the length of the initial segment of send_buffer without \n
        send_buffer[strcspn(send_buffer, "\n")]='\0';

        if(strcmp(send_buffer, "exit")==0){
            close(clientfd);
            break;
        }

        // send string to server
        if(write(clientfd, send_buffer, strlen(send_buffer))==-1){
            perror("Error writing to server!");
            close(clientfd);
            break;
        }

        memset(recv_buffer, 0, sizeof(recv_buffer));

        int bytes_read=read(clientfd, recv_buffer, sizeof(recv_buffer)-1); // sizeof(recv_buffer)-1 for accomodating one \0 character

        if (bytes_read>0){
            recv_buffer[bytes_read]='\0';
        }else if(bytes_read<=0){
            if(bytes_read==0){
                printf("Server closed the connection.\n");
            }else{
                perror("Server disconnected or error occured.\n");
            }
            close(clientfd);
            break;
        }

        printf("Server Echo: %s\n", recv_buffer);

    }



    return 0;
}