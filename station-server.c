
#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h> 
#include <string.h>
#include <stdbool.h> 

#include <sys/time.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h> 



#define MAX_ROUTES                  500
#define MAX_CHAR                    500
#define MAX_FILEPATH                500
#define MAX_SINGLE_FILE_LINE        1000

struct timetable_routes {                                                    
    char departure_time[MAX_CHAR];  
    char route_name[MAX_CHAR];
    char departing_from[MAX_CHAR];
    char arrival_time[MAX_CHAR];
    char arrival_station[MAX_CHAR];

} routenumber[MAX_ROUTES]; 

struct timetable_routes mystation_routes[MAX_ROUTES];

    

int readtt_file(char* station){ 

    char file_path[MAX_FILEPATH];
    sprintf(file_path, "tt-%s", station);
    


    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }
    

    char line[MAX_SINGLE_FILE_LINE];
    int route_index = 0;

    // Skipping first two lines
    fgets(line, sizeof(line), file);   // skipping "# station-name,longitude,latitude"
    fgets(line, sizeof(line), file);   // 
    
    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '#') {
            struct timetable_routes route;
            if (sscanf(line, "%[^,],%[^,],%[^,],%[^,],%s",
                       route.departure_time,
                       route.route_name,
                       route.departing_from,
                       route.arrival_time,
                       route.arrival_station) == 5) {
                memcpy(&mystation_routes[route_index], &route, sizeof(struct timetable_routes));
                route_index++;
            } else {
                fprintf(stderr, "Error parsing line: %s\n", line);
            }
        }
    }

    for (int i = 0; i < route_index; i++) {
    printf("Departure Time: %s, Route Name: %s, Departing From: %s, Arrival Time: %s, Arrival Station: %s\n",
            mystation_routes[i].departure_time, mystation_routes[i].route_name,
            mystation_routes[i].departing_from, mystation_routes[i].arrival_time,
            mystation_routes[i].arrival_station);
    }

    fclose(file);


    return route_index; 
}




    



int main(int argc, char* argv[]) {

    if (argc != 5){
        fprintf(stderr, "Incorrect usage. Usage: ./station-server [station_name] [tcp_port] [myUDP_port] [station2_UDP_port]\n"); 
        exit(EXIT_FAILURE); 
    }


    /* read tt file */



    int num_of_routes = readtt_file(argv[1]); 
    





    /* getaddrinfo */

    struct addrinfo hints;                                      // input structure for getaddrinfo()
    struct addrinfo *results;                                   // structure for output of getaddrinfo() 
    memset(&hints, 0, sizeof hints);                            // make sure struct. is empty 


    hints.ai_family = AF_UNSPEC;                                // don't care IPv4 or IPv6  (MIGHT CHANGE, ALLOWED TO ASSUME IPv4 ONLY)
    hints.ai_socktype = SOCK_STREAM;                            // specify TCP/IP    
    hints.ai_flags = AI_PASSIVE;                                // set local host address 

    char *portnum = argv[2]; 

    int status = getaddrinfo(NULL, portnum, &hints, &results); 
    if (status != 0){ 
        fprintf(stderr, "getaddrinfo1 error: %s\n", gai_strerror(status)); 
        exit(EXIT_FAILURE); 
    }


    /* socket */

    int TCPsocket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); 
    if (TCPsocket == -1){ 
        perror("socket() error: "); 
        return(EXIT_FAILURE); 
    }
            
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    // Lose the pesky "address already in use" error message
    setsockopt(TCPsocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));

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







    /* UDP PORT SETUP */

    /* UDP GetAddrInfo */


    struct addrinfo InputAddr;
    struct addrinfo *OutputAddr;  

    memset(&InputAddr, 0, sizeof InputAddr); // make sure the struct is empty
    InputAddr.ai_family = AF_UNSPEC;   
    InputAddr.ai_socktype = SOCK_DGRAM; 
    InputAddr.ai_flags = AI_PASSIVE;     

    char *port = argv[3]; 

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


    /* UDP Select() Setup*/

    
    fd_set currentUDP_fds; 
    fd_set readyUDP_fds; 
    int UDPfdmax = Mysocket + 1; 





    /* SELECT() OR POLL()   */

    fd_set current_sockets; 
    fd_set ready_sockets; 
    int fdmax = TCPsocket + 1; 

    // initialise current set 
    FD_ZERO(&current_sockets); 
    FD_SET(TCPsocket, &current_sockets); 

    // set timeout 
    struct timeval timeout; 


    while (true){                                           // might need to change to have dynamically adjusting array of sockets (Beej 7.3) 
        
        ready_sockets = current_sockets; 
        timeout.tv_sec = 15; 
        timeout.tv_usec = 3000; 

        status = select(fdmax, &ready_sockets, NULL, NULL, &timeout);         


        printf("status is: %i\n", status); 


        if (status == -1){ 
            perror("select() error: "); 
            exit(EXIT_FAILURE); 
        }
        else if (status){                                                               // listen() heard a connection 


            for (int i = 0; i <= fdmax; i++){ 

                
                if(FD_ISSET(i, &ready_sockets)){ 


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

                        fprintf(stdout, "\n\n'''''''''''''''''''''''''''''''''''''''''''''''''''");
                        fprintf(stdout, "Station %s: Request Received\n\n", argv[3]);  

                        /* PARSE REQUEST */

                        char valid_request_prefix[] = "GET /?to"; ; 
                        
                        if (strncmp(buf, valid_request_prefix, strlen(valid_request_prefix)) == 0){ 
                            
                            printf("\nprefix's match\n"); 


                        }




                        /* send */

                        char *msg = "HTTP/1.1 200 OK\r\n"
                                    "Content-Type: text/html\r\n"
                                    "Connection: Closed \r\n"
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
                                    "<h1> 2 + 2 = 'hello world!' </h1>\r\n"
                        //            "<p>This is a paragraph of text.</p>\r\n"
                        //            "<p>You can add more content here.</p>\r\n"
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



                        /* Sending Datagram */

                        struct sockaddr_in destination_addr;                   // destination details required for sendto()
                        int destport = atoi(argv[4]); 

                        destination_addr.sin_family = AF_INET;
                        destination_addr.sin_port = htons(destport); 
                        destination_addr.sin_addr.s_addr = INADDR_ANY;         //can assign INADDR_ANY to bind to your local IP address (like the AI_PASSIVE flag)



                        char *UDPmessage = "Hello, Station Server Neighbour!";
                        int UDPmessagelen = strlen(UDPmessage);
                        int UDPbytes_sent = sendto(Mysocket, UDPmessage, UDPmessagelen, 0, (struct sockaddr *)&destination_addr, sizeof(destination_addr));

                        if (UDPbytes_sent == -1) {
                            perror("sendto");
                        }

                        fprintf(stdout, "Station%s: sent message to station%i\n", argv[3], destination_addr.sin_port); 
                        fprintf(stdout, "%s\n", UDPmessage); 
                        

                        close(connectedSocket); 


                    
                }   

            }
        }
        else { 

            fprintf(stdout, "\n\n\nSTATION%s: TCP SELECT() TIMED OUT\n\n\n", argv[3]); 



              /* UDP SELECT() CALL -> LISTENING FOR MSGS FROM OTHER STATIONS  */


            // initialise current set 
            FD_ZERO(&currentUDP_fds); 
            FD_SET(Mysocket, &currentUDP_fds); 

                
            readyUDP_fds = currentUDP_fds; 
            timeout.tv_sec = 15; 
            timeout.tv_usec = 3000; 

            status = select(UDPfdmax, &readyUDP_fds, NULL, NULL, &timeout);         

            if (status == -1){ 
                perror("status() error: "); 
                exit(EXIT_FAILURE); 
            }
            else if (status == 1){
                            
                        
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

                printf("\n\nStation %s: Message received from Station %i : %s\n\n", argv[3], incoming_addr.sin_port, incomingmessage);
            }
            else {

                fprintf(stdout, "\n\nSTATION%s: UDP SELECT() TIMED OUT\n\n", argv[3]); 

            }
            
                     
            
            
        }



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
