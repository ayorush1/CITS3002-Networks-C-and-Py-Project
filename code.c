#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h> 
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h> 
#include <sys/select.h>     // to use select
#include <sys/time.h>       // Needed for the timeval structure




int main(int argc, char* argv[]) {

    fprintf(stdout, "%s and %i\n", argv[1], argc); 
    if (argc != 4){
        fprintf(stderr, "Incorrect usage. Usage: ./station-server [tcp_port] [myUDP_port] [station2_UDP_port]\n"); 
        exit(EXIT_FAILURE); 
    }

    /* getaddrinfo */

    struct addrinfo hints;                                      // input structure for getaddrinfo()
    struct addrinfo *results;                                   // structure for output of getaddrinfo() 
    memset(&hints, 0, sizeof hints);                            // make sure struct. is empty 


    hints.ai_family = AF_UNSPEC;                                // don't care IPv4 or IPv6  (MIGHT CHANGE, ALLOWED TO ASSUME IPv4 ONLY)
    hints.ai_socktype = SOCK_STREAM;                            // specify TCP/IP    
    hints.ai_flags = AI_PASSIVE;                                // set local host address 

    char *portnum = argv[1]; 

    int status = getaddrinfo(NULL, portnum, &hints, &results); 
    if (status != 0){ 
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status)); 
        exit(EXIT_FAILURE); 
    }


    /* socket */

    int TCPsocket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); 
    if (TCPsocket == -1){ 
        perror("socket() error: "); 
        return(EXIT_FAILURE); 
    }

    printf("socket %d opened\n", TCPsocket);


    /* bind */

    status = bind(TCPsocket, results->ai_addr, results->ai_addrlen);    /* int bind(int sockfd, struct sockaddr *my_addr, int addrlen)*/
    if (status == -1){ 
        perror("bind() error: "); 
        return(EXIT_FAILURE); 
    }


     /* listen */

    status = listen(TCPsocket, 20); 
    if (status == -1){ 
        perror("listen() error: "); 
        return(EXIT_FAILURE); 
    }
    
        /* Setup for select */
    fd_set master_set;       // Master file descriptor list
    fd_set read_fds;         // Temp file descriptor list for select()
    int fdmax;               // Maximum file descriptor number

    FD_ZERO(&master_set);    // Clear the master and temp sets
    FD_SET(TCPsocket, &master_set); // Add the listener to the master set, used to add the TCP server socket to the master_set.
    fdmax = TCPsocket;       // initialized to the socket number of the TCP server socket since it's currently the highest file descriptor.

    while(1) {
        read_fds = master_set;
        struct timeval timeout = {5, 0};  // 5 seconds timeout

        int select_result = select(fdmax + 1, &read_fds, NULL, NULL, &timeout);
        if (select_result == -1) {
            perror("select error");
            exit(EXIT_FAILURE);
        } else if (select_result == 0) {
            printf("Timeout occurred! No data after 5 seconds.\n");
        } else {
            for(int i = 0; i <= fdmax; i++) {
                if (FD_ISSET(i, &read_fds)) {
                    if (i == TCPsocket) {
                        struct sockaddr_storage their_addr;
                        socklen_t addr_size = sizeof their_addr;
                        int newSocket = accept(TCPsocket, (struct sockaddr *)&their_addr, &addr_size);
                        if (newSocket == -1) {
                            perror("accept error");
                        } else {
                            FD_SET(newSocket, &master_set);
                            if (newSocket > fdmax) {
                                fdmax = newSocket;
                            }
                            printf("New connection from socket %d\n", newSocket);
                        }
                    } else {
                        char buf[256];
                        int nbytes = recv(i, buf, sizeof buf, 0);
                        if (nbytes <= 0) {
                            if (nbytes == 0) {
                                printf("Socket %d hung up\n", i);
                            } else {
                                perror("recv error");
                            }
                            close(i);
                            FD_CLR(i, &master_set);
                        } else {
                            // Echo back the received data
                            if (send(i, buf, nbytes, 0) == -1) {
                                perror("send error");
                            }
                        }
                    }
                }
            }
        }
    }

    freeaddrinfo(results);
    close(TCPsocket);
    return 0;
}
