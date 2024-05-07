
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h> 
#include <string.h>

#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h> 




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

    status = bind(TCPsocket, results->ai_addr, results->ai_addrlen); 
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


    /* accept */



    struct sockaddr_storage their_addr; 
    socklen_t addr_size = sizeof their_addr; 

    int connectedSocket = accept(TCPsocket, (struct sockaddr *)&their_addr, &addr_size); 
    if (connectedSocket == -1){ 
        perror("accept() error: "); 
        exit(EXIT_FAILURE);  
    }


    /* receive */ 
    
    int receivelen = 2000; 
    char buf[receivelen]; 
    int bytes_received = recv(connectedSocket, &buf, receivelen, 0); 
    if (bytes_received == -1){ 
        perror("recv() error: "); 
        exit(EXIT_FAILURE); 
    }
    printf("%i, message: %s\n", bytes_received, buf); 


    /* send */

    char *msg = "HTTP/1.1 200 OK\r\n"
                "Content-Type: text/html\r\n"
                "Connection: Closed\r\n"
                "\r\n" 
                "<!DOCTYPE html>\r\n"
                "<html>\r\n"
                "<head>\r\n"
                "<style>\r\n"
                "body {\r\n"
                "    font-family: Arial, sans-serif;\r\n"
                "    background-color: #f0f0f0;\r\n"
                "}\r\n"
                "h1 {\r\n"
                "    color: #333;\r\n"
                "}\r\n"
                ".container {\r\n"
                "    width: 80%;\r\n"
                "    margin: 0 auto;\r\n"
                "    background-color: #fff;\r\n"
                "    padding: 20px;\r\n"
                "    border-radius: 10px;\r\n"
                "}\r\n"
                "</style>\r\n"
                "</head>\r\n"
                "<body>\r\n"
                "<div class=\"container\">\r\n"
                "<h1> I built this shit, brick by brick. -Arush </h1>\r\n"
//                "<p>This is a paragraph of text.</p>\r\n"
//                "<p>You can add more content here.</p>\r\n"
                "</div>\r\n"
                "</body>\r\n"
                "</html>\r\n";;
    
    int len, bytes_sent; 
    len = strlen(msg); 
    bytes_sent = send(connectedSocket, msg, len, 0); 
    if (bytes_sent == -1){ 
        perror("send() error: "); 
        exit(EXIT_FAILURE); 
    }



    freeaddrinfo(results); 
    status = shutdown(TCPsocket, 2);
    if (status == -1){ 
        perror("shutdown() error: "); 
        exit(EXIT_FAILURE); 
    }
    status = close(TCPsocket); 
    if (status == -1){ 
        perror("close() error: "); 
        exit(EXIT_FAILURE); 
    }
    






    /* REGULAR UNCONNECTED DATAGRAM SOCKETS */ 

    // requires using sendto() and recvfrom() 



    /* UDP GetAddrInfo */


    struct addrinfo InputAddr;
    struct addrinfo *OutputAddr;  

    memset(&InputAddr, 0, sizeof InputAddr); // make sure the struct is empty
    InputAddr.ai_family = AF_UNSPEC;   
    InputAddr.ai_socktype = SOCK_DGRAM; 
    InputAddr.ai_flags = AI_PASSIVE;     

    char *port = argv[2]; 

    status = getaddrinfo(NULL, port, &InputAddr, &OutputAddr); 
    if ( status != 0){ 
        fprintf(stderr, "getaddrinfo error: %s\n", gai_strerror(status));
        exit(EXIT_FAILURE);
    }


    /* UDP Socket */


    int Mysocket = socket(OutputAddr->ai_family, OutputAddr->ai_socktype, OutputAddr->ai_protocol); 
    if (Mysocket == -1){ 
        perror("socket() error: "); 
        return(EXIT_FAILURE); 
    }

    printf("\n\n\n\n my own UDP socket %d opened\n", Mysocket);


    /* bind */

    status = bind(Mysocket, OutputAddr->ai_addr, OutputAddr->ai_addrlen); 
    if (status == -1){ 
        perror("bind() error: "); 
        return(EXIT_FAILURE); 
    }


        

    switch(atoi(argv[2])){ 
        case 2606: 
            

            /* Sending Datagram */

            struct sockaddr_in destination_addr;                   // destination details required for sendto()
            int destport = atoi(argv[3]); 

            destination_addr.sin_family = AF_INET;
            destination_addr.sin_port = htons(destport); 
            destination_addr.sin_addr.s_addr = INADDR_ANY;         //can assign INADDR_ANY to bind to your local IP address (like the AI_PASSIVE flag)



            char *UDPmessage = "Hello, Station Server 2!";
            int UDPmessagelen = strlen(UDPmessage);
            int UDPbytes_sent = sendto(Mysocket, UDPmessage, UDPmessagelen, 0, (struct sockaddr *)&destination_addr, sizeof(destination_addr));

            if (UDPbytes_sent == -1) {
                perror("sendto");
            }


            break; 
        case 2608:
            {
            struct sockaddr_in incoming_addr; 
            socklen_t incoming_addr_len = sizeof(struct sockaddr_in);
            int BUFFER_SIZE = 1000; 
            char incomingmessage[BUFFER_SIZE]; 

            
            int UDPbytes_received = recvfrom(Mysocket, incomingmessage, BUFFER_SIZE, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_len); 
            if (UDPbytes_received == -1){ 
                perror("recfrom() error: "); 
                close(Mysocket); 
                exit(EXIT_FAILURE); 
            }

            printf("Message received from Station1 : %s\n", incomingmessage);

            }
            break; 
        default:
            fprintf(stderr, "not sure which input given \n"); 
            exit(EXIT_FAILURE); 
            
    }

        
    freeaddrinfo(OutputAddr); 




    return 0; 
}









/*

GET /?to=Perth_Stn HTTP/1.1

GET / HTTP/1.1
Host: localhost:4444
Connection: keep-alive
sec-ch-ua: "Chromium";v="124", "Google Chrome";v="124", "Not-A.Brand";v="99"
sec-ch-ua-mobile: ?0
sec-ch-ua-platform: "Windows"
Upgrade-Insecure-Requests: 1
User-Agent: Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/124.0.0.0 Safari/537.36 */
//  Accept: text/html,application/xhtml+xml,application/xml;q=0.9,image/avif,image/webp,image/apng,*/*;q=0.8,application/signed-exchange;v=b3;q=0.7
/* 
Sec-Fetch-Site: none
Sec-Fetch-Mode: navigate
Sec-Fetch-User: ?1
Sec-Fetch-Dest: document
Accept-Encoding: gzip, deflate, br, zstd
Accept-Language: en-AU,en-GB;q=0.9,en-US;q=0.8,en;q=0.7

*/
