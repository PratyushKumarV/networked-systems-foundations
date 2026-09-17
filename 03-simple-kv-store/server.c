#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include <pthread.h>
#include <ctype.h>

#define SIZE 1031

typedef struct node{
    char *key;
    char *value;
    struct node *next;
}node;

unsigned long hash(char *str);
void process(char *input, node **arr, int connfd);

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

    // initialize the hash map in memory (as an array of node pointers)
    node **arr=(node**)malloc(sizeof(node*)*SIZE);
    for(int i=0;i<SIZE;i++){
        arr[i]=NULL;
    }
    

    while(1){
        int connfd;
        socklen_t client_len=sizeof(client_addr);
        if((connfd=accept(sockfd, (struct sockaddr*)&client_addr, &client_len))==-1){
            fprintf(stderr, "Failed to accept connection");
            continue;
        }


        char buff[1024];
        ssize_t bytes_read;
        
        while((bytes_read=read(connfd, buff, sizeof(buff)-1))>0){
            buff[bytes_read]='\0';
            process(buff, arr, connfd);
        }

        close(connfd);


    }
    close(sockfd);
    free(arr);

    return 0;
}

unsigned long hash(char *str){
    unsigned long h=5381;
    for(size_t i=0;i<strlen(str);i++){
        h=((h<<5)+h) + str[i];
    }
    return h;
}

void process(char *input, node **arr, int connfd){
    char *delims=" \n";

    char *token=strtok(input, delims); 
    char *key=strtok(NULL, delims); // Extract key
    char *value=strtok(NULL, delims); // Extract value (If value not given then it remains NULL)

    if(key==NULL){
        if(write(connfd, "NO KEY PROVIDED\n", strlen("NO KEY PROVIDED\n"))==-1){
            fprintf(stderr, "Failed to write to client");
        }
        return;
    }

    unsigned long h=hash(key);

    int index=h%SIZE;

    // convert method name to upper case
    for(size_t i=0;i<strlen(token);i++){
        token[i]=toupper(token[i]);
    }

    if (strcmp(token, "SET")==0){

        if(value==NULL){
            if(write(connfd, "NO VALUE PROVIDED\n", strlen("NO VALUE PROVIDED\n"))==-1){
                fprintf(stderr, "Failed to write to client");
            }
            return;
        }

        node *new=(node*)malloc(sizeof(node));
        new->key=strdup(key);  // strdup is used so that actual memory is allocated for key and is stored in arr. If strdup is not used then the key assigned locally (within process) is used which is teared down when the process function returns. Using strdup, it internally allocates memory for the string, copies the contents of the string and returns a pointer to the newly allocated memory block.
        new->value=strdup(value); // same reasoning as above
        new->next=NULL;

        if(arr[index]==NULL){ // if there is no node present in the index
            arr[index]=new;
        }else{ // if there is already a node/chain present in the index
            node *curr=arr[index];
            while(strcmp(curr->key, key)!=0 && curr->next!=NULL){
                curr=curr->next;
            }
            if(strcmp(curr->key, key)==0){ // key already exists
                free(curr->value);
                curr->value=new->value;
                free(new->key);
                free(new);
            }else{
                curr->next=new;
            }
            
        }

        if(write(connfd, "OK\n", strlen("OK\n"))==-1){
            fprintf(stderr, "Failed to write response to client");
        }

    }else if(strcmp(token, "GET")==0){
        node *curr=arr[index];
        while(curr!=NULL && strcmp(curr->key, key)!=0){
            curr=curr->next;
        }

        if(curr==NULL){
            // write back to client
            if(write(connfd, "NOT FOUND\n", strlen("NOT FOUND\n"))==-1){
                fprintf(stderr, "Failed to write to client");
            }
        }else{
            // write back to client
            char buff[1024];
            int len=snprintf(buff, sizeof(buff), "VALUE %s\n", curr->value); // used to write a formatted string to a buffer (up to sizeof(buff)-1, one char reserved for '\0'), returns the length of the formatted string (excluding '\0')
            if(write(connfd, buff, len)==-1){ // writes back only the length of the formatted string, not the entire buffer (without including '\0').
                fprintf(stderr, "Failed to write to client");
            }
        }
        
    }else if(strcmp(token, "DEL")==0){
        node *curr=arr[index], *prev=NULL;
        while(curr!=NULL && strcmp(curr->key, key)!=0){
            prev=curr;
            curr=curr->next;
        }

        if(curr==NULL){
            // write back to client
            if(write(connfd, "NOT FOUND\n", strlen("NOT FOUND\n"))==-1){
                fprintf(stderr, "Failed to write to client");
            }
        }else if(prev==NULL && curr!=NULL){ // we have to delete the head of the chain (curr is head)
            arr[index]=curr->next;
            free(curr->key);
            free(curr->value);
            free(curr);

            // write back to client
            if(write(connfd, "DELETED\n", strlen("DELETED\n"))==-1){
                fprintf(stderr, "Failed to write to client");
            }
        }else{
            prev->next=curr->next;
            free(curr->key); 
            free(curr->value); // free the memory allocated using strdup for both key and value before freeing the memory for the node
            free(curr);
            
            // write back to client
            if(write(connfd, "DELETED\n", strlen("DELETED\n"))==-1){
                fprintf(stderr, "Failed to write to client");
            }
        }
        
    }else{
        if(write(connfd, "INVALID TOKEN\n", strlen("INVALID TOKEN\n"))==-1){
            fprintf(stderr, "Failed to write to client");
        }
    }

}