#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>         // uint16_t
#include <string.h>         // memset()
#include <unistd.h>         // read(), write(), close()
#include <sys/socket.h>     // socket(), bind(), listen(), accept()
#include <sys/types.h>      // socklen_t, ssize_t
#include <netinet/in.h>     // struct sockaddr_in
#include <arpa/inet.h>      // htonl(), htons(), ntohl(), ntohs()

const uint16_t PORT=8080;

int main(){

    int sockfd=socket(AF_INET, SOCK_STREAM, 0);
    if (sockfd==-1){
        perror("Error with creating server socket!");
        return 1;
    }

    /*
        * setsockopt() configures options on an existing socket file descriptor.
        * 
        * Target Option: SO_REUSEADDR (Socket Option: Reuse Address)
        * Level: SOL_SOCKET (Socket-Level API, as opposed to IPPROTO_TCP)
        * 
        * WHY THIS IS NEEDED:
        * When a TCP connection closes, the socket enters the TIME_WAIT state 
        * (typically lasting 30 to 120 seconds). During this time, the OS kernel 
        * prevents any new process from binding to the same IP/Port pair to avoid 
        * delayed or duplicate in-flight packets from corrupting new connections.
        * 
        * Without SO_REUSEADDR, restarting the server immediately after closing it 
        * causes bind() to fail with "Address already in use" (EADDRINUSE).
        * 
        * Setting opt = 1 enables this flag, telling the kernel to allow bind() 
        * to reclaim the local port immediately, even if previous sockets on that 
        * port are still sitting in the TIME_WAIT state.
    */
    int opt = 1;
    if (setsockopt(sockfd, SOL_SOCKET, SO_REUSEADDR, &opt, sizeof(opt)) == -1) {
        perror("setsockopt SO_REUSEADDR failed");
        close(sockfd);
        return 1;
    }

    struct sockaddr_in server_addr, client_addr;
    memset(&server_addr, 0, sizeof(server_addr));   // clear struct memory and fill with 0's

    // Both sin_port and sin_addr has to be in network byte order (big endian)
    /*
        struct sockaddr_in {
            uint16_t sin_family;        // Protocol family (always AF_INET) 
            uint16_t sin_port;          // Port number in network byte order 
            struct in_addr sin_addr;    // IP address in network byte order 
            unsigned char sin_zero[8];  // Pad to sizeof(struct sockaddr) 
        };
    */
    server_addr.sin_family=AF_INET;
    server_addr.sin_port=htons(PORT);
    server_addr.sin_addr.s_addr=htonl(INADDR_ANY); // INADDR_ANY (IP: 0.0.0.0) tells the kernel to route incoming traffic from any interface (localhost, VM network, etc.) to this socket

    if(bind(sockfd, (struct sockaddr *)&server_addr, sizeof(server_addr))==-1){
        perror("Error binding the socket with socket address!");
        close(sockfd);
        return 1;
    }

    // Enables the kernel to perform handshakes, marks the socket as passive (server) and listens for incoming SYN packets. The kernel handles the 3 way handshake. When done the connection is established and it is moved into an accept queue (whose length is specified in the backlog argument in listen). accept() function pulls connections off of this queue.

    if(listen(sockfd, 5)==-1){
        perror("Error listening on socket!");
        close(sockfd);
        return 1;
    }

    // Kernel populates client_addr and updates client_len upon accepting connection
    socklen_t client_len=sizeof(client_addr);   
    int connfd=accept(sockfd, (struct sockaddr *)&client_addr, &client_len);
    if (connfd==-1){
        perror("Error accepting connection!");
        close(sockfd);
        return 1;
    }

    // INET_ADDRSTRLEN is 16 bytes
    char client_ip[INET_ADDRSTRLEN];
    inet_ntop(AF_INET, &client_addr.sin_addr, client_ip, sizeof(client_ip));

    printf("Connection to %s:%d succeeded!\n", client_ip, ntohs(client_addr.sin_port));

    char buffer[1024];
    ssize_t bytes_read; // ssize_t is signed size, this is because the read function can return -1 on error

    // read() blocks the process (sleep state) until the kernel's receive queue gets data from NIC interrupts.
    // Returns bytes read into user space, 0 on client FIN (EOF), or -1 on error.
    while((bytes_read=read(connfd, buffer, sizeof(buffer)))>0){

        // Copies payload bytes back into the kernel's send queue to transmit back over TCP
        if(write(connfd, buffer, bytes_read)==-1){
            perror("Error writing to client!");
            break;
        }
    }

    // bytes_read == 0 implies graceful client disconnect (TCP FIN received)
    if (bytes_read==-1){
        fprintf(stderr, "Connection reset / read error from %s:%d\n", client_ip, ntohs(client_addr.sin_port));
        close(connfd);  // kernel sends a FIN to the client to close the connection
        close(sockfd);  // releases port 8080 back to the kernel
        return 1;
    }

    printf("Client %s:%d disconnected gracefully.\n", client_ip, ntohs(client_addr.sin_port));

    close(connfd);  // kernel sends a FIN to the client to close the connection
    close(sockfd);  // releases port 8080 back to the kernel

    return 0;
}