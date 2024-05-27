
/* CITS3002 COMPUTER NETWORKS PROJECT 2024*/ 

/* 
student: Arush Kathal 23730792 
student: Armaan Josan 24001588 
*/



/* Really sorry for the one big file of code! I tried to clean it up as much as possible :) */

#include <stdio.h> 
#include <stdlib.h> 
#include <unistd.h>
#include <errno.h> 
#include <string.h>
#include <stdbool.h> 
#include <ctype.h> 
#include <sys/time.h> 
#include <sys/types.h>
#include <sys/socket.h> 
#include <netdb.h> 

#define MAX_ROUTES                  500
#define MAX_CHAR                    2000
#define MAX_FILEPATH                500
#define MAX_SINGLE_FILE_LINE        1000
#define MAX_STATION_NAME            60
#define URL_HEADER_TIME_LEN         7



struct timetable_routes {                                                       // struct. to store timetable file information.
    int departure_time;  
    char route_name[MAX_CHAR];
    char departing_from[MAX_CHAR];
    int arrival_time;
    char arrival_station[MAX_CHAR];

} routenumber[MAX_ROUTES]; 

struct timetable_routes mystation_routes[MAX_ROUTES];                       


typedef struct Neighbor {                                                       // struct. to store commandline neighbour information. 
    char host[MAX_STATION_NAME];
    int port;
} Neighbor; 

int num_neighbors; 








/* function to read the timetable file and store in struct. */



int readtt_file(char* station){ 

    char file_path[MAX_FILEPATH];
    snprintf(file_path, MAX_FILEPATH + 10, "tt-%s", station);
    FILE *file = fopen(file_path, "r");
    if (file == NULL) {
        fprintf(stderr, "Error opening file.\n");
        exit(EXIT_FAILURE);
    }

    char line[MAX_SINGLE_FILE_LINE];
    int route_index = 0;
    fgets(line, sizeof(line), file);                                            // skipping first few lines
    fgets(line, sizeof(line), file);    
    printf("\n");

    while (fgets(line, sizeof(line), file) != NULL) {
        if (line[0] != '#') {
            struct timetable_routes route;                                      // read and stor information. 
            int dep1, dep2, arr1, arr2; 
            int x = sscanf(line, "%d:%d,%[^,],%[^,],%d:%d,%[^\n]",
                       &dep1, &dep2,
                       route.route_name,
                       route.departing_from,
                       &arr1, &arr2,
                       route.arrival_station);
            
            if (x == 7) {
                //route.arrival_station[strcspn(route.arrival_station, "\n")] = '\0';
                route.departure_time = (dep1 * 100) + dep2; 
                route.arrival_time = (arr1 * 100) + arr2; 
                memcpy(&mystation_routes[route_index], &route, sizeof(struct timetable_routes));
                route_index++;
            } 
            else {

                fprintf(stderr, "Error parsing line: %s\n", line);
            }
        }
    }

    fclose(file);
    return route_index; 
}







/* function to extract the time from the URL HTML request */



int extract_depart_time(char* message_received, char* depart_time) {
    
    char *url_time = strstr(message_received, "leave=");                           // needs to find position of '=' and '&'
    if (url_time == NULL) {
        printf("'leave=' not found in the string.\n");
        return(0); 
    }

    // Calculate the length of the substring
    url_time += strlen("leave=");
    // Copy the substring from str
    strncpy(depart_time, url_time, URL_HEADER_TIME_LEN);

    depart_time[URL_HEADER_TIME_LEN] = '\0'; // Null-terminate the string
    char string_time[5]; 
    int offset = 0; 
    for (int i = 0; i < URL_HEADER_TIME_LEN; i++){ 
        if (i == 2 || i == 3 || i == 4 ){ 
            offset++;
            continue; 
        }
        string_time[i-offset] = url_time[i]; 
    }
    string_time[5] = '\0'; 
    int time = atoi(string_time); 
    return time; 
}












/* function to return pointer to destination from the URL HTML request */




void extract_destination_station(char* message_received, char* destination_station) {
    // Find the position of '=' and '&'
    char* equal_sign = strchr(message_received, '=');
    char* ampersand_sign = strchr(equal_sign, '&');
     
    if (equal_sign == NULL) {
        printf("No '=' sign found.\n");
        exit(EXIT_FAILURE); 
    }
    if (ampersand_sign == NULL) {
        printf("No '&' sign found after '='.\n");
        exit(EXIT_FAILURE);
    }

    // Calculate the length of the substring
    int substring_length = ampersand_sign - equal_sign - 1;

    // Copy the substring from str
    strncpy(destination_station, equal_sign + 1, substring_length);
    destination_station[substring_length] = '\0'; // Null-terminate the string

    return; 
}










/* My main function. This is where the server operates until it is shutdown. */



int main(int argc, char* argv[]) {

    if (argc < 5){
        fprintf(stderr, "Incorrect usage. Usage: ./station-server [station_name] [tcp_port] [myUDP_port] [station2_host:station2_UDPport] ... \n"); 
        exit(EXIT_FAILURE); 
    }








    /* Parsing and Storing Neighbour Station information from Commandline*/



    Neighbor *neighbors = NULL;
    int num_neighbors = argc - 4;

    neighbors = malloc(num_neighbors * sizeof(Neighbor));
    if (neighbors == NULL) {
        fprintf(stderr, "Memory allocation failed.\n");
        return 1;
    }
    // Parsing neighbor stations
    for (int i = 0; i < num_neighbors; i++) {
        char host[MAX_STATION_NAME];
        int port;
        sscanf(argv[i + 4], "%[^:]:%d", host, &port);
        if (strlen(host) > 0 && port > 0) {
            strcpy(neighbors[i].host, host);
            neighbors[i].port = port;
        } 
        else {
            fprintf(stderr, "\nInvalid neighbor format: %s\n", argv[i + 4]);
            free(neighbors);
            return 1;
        }
    }
    // Printing neighbor stations
    printf("\nNeighbor stations:\n");
    for (int i = 0; i < num_neighbors; i++) {
        printf("Host: %s, Port: %d\n", neighbors[i].host, neighbors[i].port);
    }











    /* Calling Timetable-File Reading Function.*/

    int num_of_routes = readtt_file(argv[1]); 
    
    













    /* Initialising the TCP Socket for the Station. */





    struct addrinfo hints;                                      // input structure for getaddrinfo()
    struct addrinfo *results;                                   // structure for output of getaddrinfo() 
    memset(&hints, 0, sizeof hints);                            // make sure struct. is empty 

    hints.ai_family = AF_UNSPEC;                                // don't care IPv4 or IPv6  
    hints.ai_socktype = SOCK_STREAM;                            // specify TCP/IP    
    hints.ai_flags = AI_PASSIVE;                                // set local host address 

    char *portnum = argv[2]; 
    int status = getaddrinfo(NULL, portnum, &hints, &results); 
    if (status != 0){ 
        fprintf(stderr, "getaddrinfo1 error: %s\n", gai_strerror(status)); 
        exit(EXIT_FAILURE); 
    }






    int TCPsocket = socket(results->ai_family, results->ai_socktype, results->ai_protocol); 
    if (TCPsocket == -1){ 
        perror("socket() error: "); 
        return(EXIT_FAILURE); 
    }  
    int yes=1;        // For setsockopt() SO_REUSEADDR, below
    setsockopt(TCPsocket, SOL_SOCKET, SO_REUSEADDR, &yes, sizeof(int));  // Lose the "address already in use" error message






    status = bind(TCPsocket, results->ai_addr, results->ai_addrlen); 
    if (status == -1){ 
        perror("C bind() error: "); 
        return(EXIT_FAILURE); 
    }





    status = listen(TCPsocket, 20); 
    if (status == -1){ 
        perror("listen() error: "); 
        return(EXIT_FAILURE); 
    }
















    /* Initialising the UDP Socket for the Station. */




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





    int Mysocket = socket(OutputAddr->ai_family, OutputAddr->ai_socktype, OutputAddr->ai_protocol); 
    if (Mysocket == -1){ 
        perror("socket() error: "); 
        return(EXIT_FAILURE); 
    }
    




    status = bind(Mysocket, OutputAddr->ai_addr, OutputAddr->ai_addrlen); 
    if (status == -1){ 
        perror("C bind() error: "); 
        return(EXIT_FAILURE); 
    }















    /* TCP and UDP Select()'s Initialisation/Set-up. */


    
    fd_set currentUDP_fds;                                          // UDP Select() Setup
    fd_set readyUDP_fds; 
    int UDPfdmax = Mysocket + 1; 

    fd_set current_sockets;                                         // UDP Select() Setup
    fd_set ready_sockets; 
    int fdmax = TCPsocket + 1; 

    // initialise current set 
    FD_ZERO(&current_sockets); 
    FD_SET(TCPsocket, &current_sockets); 

    // set timeout 
    struct timeval timeout; 















    /* Server 'up and running'. Here, we will alternate between 2 select() statements - 
       One listening for TCP connections, and one listening for UDP datagrams.         */



    while (true){                                         
    

        /* TCP select() information initialised (within loop) */

        ready_sockets = current_sockets; 
        timeout.tv_sec = 0; 
        timeout.tv_usec = 200000; 
        status = select(fdmax, &ready_sockets, NULL, NULL, &timeout);   

        if (status == -1){ 
            perror("select() error: "); 
            exit(EXIT_FAILURE); 
        }





        /* If we have received a TCP Request from the Web-Browser. */    

        else if (status){                                                      
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
                        buf[bytes_received] = '\0';                                                         // add null-byte to received message.
                        fprintf(stdout, "''''''''''''''''''''''''''''''''''''''''''''''''''''\n");
                        fprintf(stdout, "Station %s: Request Received\n", argv[3]);  








                        /* Parsing The Request. */

                        char valid_request_prefix[] = "GET /?to"; ; 
                        if (strncmp(buf, valid_request_prefix, strlen(valid_request_prefix)) == 0){     // if valid query. 
                            
                            printf("Valid Request\n");
                            char destination_station[MAX_STATION_NAME + 1]; 
                            extract_destination_station(buf, destination_station); 
                            printf("Destination:          %s\n", destination_station); 
                            char depart_time_str[URL_HEADER_TIME_LEN]; 
                            int depart_time = extract_depart_time(buf, depart_time_str); 
                            printf("Departure time:       %i\n", depart_time); 

                            int target_route = -1; 
                            char return_message[4500]; 
                            num_of_routes = readtt_file(argv[1]);                                        /* CHECK/DETECT TIMETABLE CHANGE */         






                            /* Checking Our Own Timetable. */

                            for (int j = 0; j < num_of_routes; j++){ 
                                if ((mystation_routes[j].departure_time >= depart_time) && (strcmp(mystation_routes[j].arrival_station, destination_station) == 0)){
                                    target_route = j; 
                                    break; 
                                }

                            }

                            if (target_route != -1){       // success. 
                                
                                char formatted_time1[10];
                                char formatted_time2[10];
                                snprintf(formatted_time1, URL_HEADER_TIME_LEN + 10,  "%02d:%02d", mystation_routes[target_route].departure_time / 100, mystation_routes[target_route].departure_time % 100);
                                snprintf(formatted_time2, URL_HEADER_TIME_LEN + 10, "%02d:%02d", mystation_routes[target_route].arrival_time / 100, mystation_routes[target_route].arrival_time % 100);
                                
                                sprintf(return_message, "catch %s from %s, at time %s, to arrive at %s at time %s",  
                                mystation_routes[target_route].route_name, argv[1], formatted_time1, 
                                mystation_routes[target_route].arrival_station, formatted_time2); 

                            }





                            /* Did Not Find in our own Timetable. */
                            else{ 






                                /* Creating QUERY to send/ask neighbour stations. */

                                sleep(5);                                                  // sleep until other QUERY's are complete (~2sec).  
                                int sent = 0; 
                
                                for (int y = 0; y < num_neighbors; y++){                   // send QUERY to all neighbours. 

                                    struct sockaddr_in destination_addr;                   // destination details required for sendto()
                                    int destport = neighbors[y].port; 
                                    destination_addr.sin_family = AF_INET;
                                    destination_addr.sin_port = htons(destport); 
                                    struct hostent *host = gethostbyname(neighbors[y].host);
                                    if (host == NULL) {
                                        fprintf(stderr, "gethostbyname error: %s\n", hstrerror(h_errno));
                                        continue; 
                                    }
                                    // Copy the resolved IP address to the sockaddr_in structure
                                    memcpy(&destination_addr.sin_addr, host->h_addr, host->h_length);
                                    char formatted_time1[10];
                                    snprintf(formatted_time1, URL_HEADER_TIME_LEN + 10, "%02d:%02d", depart_time / 100, depart_time % 100);
                                    char UDPmessage[200];
                                    sprintf(UDPmessage, "QUERY %s %s %s", argv[1], destination_station, formatted_time1);
                                    int UDPmessagelen = strlen(UDPmessage);
                                    int UDPbytes_sent = sendto(Mysocket, UDPmessage, UDPmessagelen, 0, (struct sockaddr *)&destination_addr, sizeof(destination_addr));

                                    if (UDPbytes_sent == -1) {
                                        perror("sendto");
                                    }
                                    fprintf(stdout, "Station%s: sent message to station%i\n", argv[3], destport);   
                                    printf("%s\n", UDPmessage); 
                                    sent++; 

                                }            
                                if (sent == 0){ 
                                    strcpy(return_message, "there are no routes that fit those criteria"); 
                                }




                                /* After sending QUERY, block until we get a response. */


                                else{ 

                                    struct sockaddr_in incoming_addr; 
                                    socklen_t incoming_addr_len = sizeof(struct sockaddr_in);
                                    int BUFFER_SIZE = 500; 
                                    char queryreply[BUFFER_SIZE]; 
                                    fd_set blocknrecvfds; 
                                    struct timeval tv; 
                                    int retval; 
                                    tv.tv_sec = 4;                                                      // Set timeout period to 4 second
                                    tv.tv_usec = 0; 
                                    FD_ZERO(&blocknrecvfds); 
                                    FD_SET(Mysocket, &blocknrecvfds);
                                    retval = select(Mysocket + 1, &blocknrecvfds, NULL, NULL, &tv);
                                    if (retval == -1) {
                                        perror("select()");
                                        close(Mysocket);
                                        exit(EXIT_FAILURE);
                                    }





                                    /* If no responses, this means no station had a valid route to send back.*/

                                    else if (retval == 0) {
                                    strcpy(return_message, "there are no routes that fit those criteria");      
                                    }










                                    /* If We got a Response, read it and add a Route to that Station to the Journey. */

                                    else{ 
                                        
                                        int UDPbytes_received = recvfrom(Mysocket, queryreply, BUFFER_SIZE, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_len); 
                                        if (UDPbytes_received == -1){ 
                                            perror("recfrom() error: "); 
                                            close(Mysocket); 
                                            exit(EXIT_FAILURE); 
                                        }
                                        queryreply[UDPbytes_received] = '\0';
                                        printf("Station %s: Message received from Station %i\n", argv[3], ntohs(incoming_addr.sin_port));
                                        printf("%s\n", queryreply); 
                                        strcpy(return_message, queryreply); 
                                        char *replyprefix = "catch";
                                        if (strncmp(return_message, replyprefix, strlen(replyprefix)) == 0) {                     
                                            if (!isdigit(return_message[strlen(return_message) - 1])) {

                                                char *by_position = strstr(return_message, "by ");
                                                if (by_position == NULL) {
                                                    fprintf(stdout, "substring 'by ' error.\n"); 
                                                    exit(EXIT_FAILURE); 
                                                }
                                                char *station_name = by_position + strlen("by ");

                                                int intermediate_route = -1; 
                                                char intermediate_path[4500]; 
                                                num_of_routes = readtt_file(argv[1]);                      /* CHECK/DETECT TIMETABLE CHANGE */                             
                                                
                                                for (int k = 0; k < num_of_routes; k++){ 
                                                    if ((mystation_routes[k].departure_time >= depart_time) && (strcmp(mystation_routes[k].arrival_station, station_name) == 0)){
                                                        intermediate_route = k; 
                                                        break; 
                                                    }
                                                }
                                                if (intermediate_route == -1) {
                                                    fprintf(stdout, "intermediate route error.\n"); 
                                                    exit(EXIT_FAILURE); 
                                                }
                                                char formatted_time3[10];
                                                char formatted_time4[10];
                                                snprintf(formatted_time3, URL_HEADER_TIME_LEN + 10,  "%02d:%02d", mystation_routes[intermediate_route].departure_time / 100, mystation_routes[intermediate_route].departure_time % 100);
                                                snprintf(formatted_time4, URL_HEADER_TIME_LEN + 10, "%02d:%02d", mystation_routes[intermediate_route].arrival_time / 100, mystation_routes[intermediate_route].arrival_time % 100);
                                                
                                                sprintf(intermediate_path, "\n catch %s from %s, at time %s, to arrive at %s at time %s",  
                                                mystation_routes[intermediate_route].route_name, argv[1], formatted_time3, 
                                                mystation_routes[intermediate_route].arrival_station, formatted_time4); 
                                                size_t new_path_length = strlen(intermediate_path);
                                                strcpy(by_position, intermediate_path);
                                                by_position[new_path_length] = '\0';

                                            

                                            }
                                        
                                        }
                                    }

                                }                                
                                    
                            }   

                               
                            
                        


                            /* Now that we have a response, send it back to the Client (Webpage). */

                            char msg[15000]; 
                            sprintf(msg, "HTTP/1.1 200 OK\r\n"
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
                                        "    width: 80%%;\r\n"
                                        "    margin: 0 auto;\r\n"
                                        "    background-color: #fff;\r\n"
                                        "    padding: 20px;\r\n"
                                        "    border-radius: 10px;\r\n"
                                        "}\r\n"
                                        "</style>\r\n"
                                        "</head>\r\n"
                                        "<body>\r\n"
                                        "<div class=\"container\">\r\n"
                                        "<h3 style=\"white-space: pre-wrap;\"> %s </h3>\r\n"                // Here we include the return_message
                                        "</div>\r\n"
                                        "</body>\r\n"
                                        "</html>\r\n",
                                        return_message); 

                            int len, bytes_sent; 
                            len = strlen(msg); 
                            bytes_sent = send(connectedSocket, msg, len, 0); 
                            if (bytes_sent == -1){ 
                                perror("send() error: "); 
                                exit(EXIT_FAILURE); 
                            } 
                            printf("Client Reponse Sent: %s\n", return_message); 
                            close(connectedSocket);
                        }  
                        else{ 
                            printf("Invalid request\n"); 
                        }
                }   
            }
        }





















        /* If No TCP Connection detected, listen for UDP connections for some time (via Select()) */

        else { 

            FD_ZERO(&currentUDP_fds); 
            FD_SET(Mysocket, &currentUDP_fds); 
            readyUDP_fds = currentUDP_fds; 
            timeout.tv_sec = 0; 
            timeout.tv_usec = 200000; 


            status = select(UDPfdmax, &readyUDP_fds, NULL, NULL, &timeout);         
            if (status == -1){ 
                perror("status() error: "); 
                exit(EXIT_FAILURE); 
            }







            /* IF A UDP Datagram was Received. */

            else if (status == 1){


                char return_message[4500] = {0}; 
                struct sockaddr_in incoming_addr; 
                socklen_t incoming_addr_len = sizeof(struct sockaddr_in);
                int BUFFER_SIZE = 200; 
                char incomingmessage[BUFFER_SIZE]; 
                int UDPbytes_received = recvfrom(Mysocket, incomingmessage, BUFFER_SIZE, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_len); 
                if (UDPbytes_received == -1){ 
                    perror("recfrom() error: "); 
                    close(Mysocket); 
                    exit(EXIT_FAILURE); 
                }
                incomingmessage[UDPbytes_received] = '\0';
                printf("Station %s: Message received from Station %i\n", argv[3], ntohs(incoming_addr.sin_port));
                printf("%s\n", incomingmessage); 
                char source_station[MAX_STATION_NAME], destination[MAX_STATION_NAME];
                int departure_hour, departure_minute;
                int source_port_number = ntohs(incoming_addr.sin_port);

                
                





                /* It will Always be a QUERY Datagram, so parse the Datagram. */

                if( sscanf(incomingmessage, "QUERY %s %s %d:%d", source_station, destination, &departure_hour, &departure_minute) != 4){ 
                    fprintf(stdout, "station%s: could not successfuly scan QUERY. ignoring request.\n", argv[3]); 
                    continue; 
                }
                usleep(1000);                                                           // WAIT FOR SENDER TO SEND ALL DATAGRAM BEFORE RESPONDING      
                
                if (departure_hour >= 23){ 
                    continue; 
                }
                int departure_time2; 
                if (departure_hour >= 22){ 
                    departure_time2 = (departure_hour * 100) + departure_minute + 130;
                }
                else{ 
                    departure_time2 = ((((departure_hour * 60 + departure_minute + 90) / 60) % 24) * 100) + ((departure_minute + 90) % 60);
                }
                int target_route = -1; 


                num_of_routes = readtt_file(argv[1]);                                                   /* CHECK/DETECT TIMETABLE CHANGE */   
                for (int j = 0; j < num_of_routes; j++){ 
                            if ((mystation_routes[j].departure_time >= departure_time2) && (strcmp(mystation_routes[j].arrival_station, destination) == 0)){
                                target_route = j; 
                                break; 
                            }

                }




                /* If Required Route Found, Then Reply back with it and Add your Station Name to the end. */

                if (target_route != -1){    
                    char formatted_time1[10];
                    char formatted_time2[10];
                    snprintf(formatted_time1, URL_HEADER_TIME_LEN + 10,  "%02d:%02d", mystation_routes[target_route].departure_time / 100, mystation_routes[target_route].departure_time % 100);
                    snprintf(formatted_time2, URL_HEADER_TIME_LEN + 10, "%02d:%02d", mystation_routes[target_route].arrival_time / 100, mystation_routes[target_route].arrival_time % 100);
                    
                    sprintf(return_message, "catch %s from %s, at time %s, to arrive at %s at time %s",  
                    mystation_routes[target_route].route_name, argv[1], formatted_time1, 
                    mystation_routes[target_route].arrival_station, formatted_time2); 
                    strcat(return_message, " by "); 
                    strcat(return_message, argv[1]); 

                }






                /* Otherwise If not Found, then Forward the Datagram (QUERY) To all your Neighbours. */    



                else{ 
                    int sent = 0; 
                    for (int y = 0; y < num_neighbors; y++){ 
                        if (neighbors[y].port == source_port_number){ 
                            strcpy(return_message, "there are no routes that fit those criteria"); 
                            continue; 
                        }

                        struct sockaddr_in destination_addr;                   // destination details required for sendto()
                        int destport = neighbors[y].port; 
                        destination_addr.sin_family = AF_INET;
                        destination_addr.sin_port = htons(destport); 
                        struct hostent *host = gethostbyname(neighbors[y].host);
                        if (host == NULL) {
                            fprintf(stderr, "gethostbyname error: %s\n", hstrerror(h_errno));
                            continue; 
                        }
                        // Copy the resolved IP address to the sockaddr_in structure
                        memcpy(&destination_addr.sin_addr, host->h_addr, host->h_length);
                        char formatted_time1[10];
                        snprintf(formatted_time1, URL_HEADER_TIME_LEN + 10, "%02d:%02d", departure_time2 / 100, departure_time2 % 100);
                        char UDPmessage[200];
                        sprintf(UDPmessage, "QUERY %s %s %s", argv[1], destination, formatted_time1);
                        int UDPmessagelen = strlen(UDPmessage);
                        int UDPbytes_sent = sendto(Mysocket, UDPmessage, UDPmessagelen, 0, (struct sockaddr *)&destination_addr, sizeof(destination_addr));
                        if (UDPbytes_sent == -1) {
                            perror("sendto");
                            exit(EXIT_FAILURE); 
                        }
                        fprintf(stdout, "Station%s: sent message to station%i\n", argv[3], destport); 
                        printf("%s\n", UDPmessage);  
                        sent++; 

                    }
                    if (sent == 0){ 
                        sleep(2);         // account for other stations processesing query before replying. 
                    }












                    /*  Then, Listen for a UDP Response. If we get one, then a valid route has been found, otherwise it has not. */



                    else { 
                        struct sockaddr_in incoming_addr; 
                        socklen_t incoming_addr_len = sizeof(struct sockaddr_in);
                        int BUFFER_SIZE = 500; 
                        char queryreply[BUFFER_SIZE]; 
                        fd_set blocknrecvfds; 
                        struct timeval tv; 
                        int retval; 
                        // Set timeout period to 0.5 seconds
                        tv.tv_sec = 0;
                        tv.tv_usec = 500000;

                        FD_ZERO(&blocknrecvfds); 
                        FD_SET(Mysocket, &blocknrecvfds);
                        retval = select(Mysocket + 1, &blocknrecvfds, NULL, NULL, &tv);
                        if (retval == -1) {
                            perror("select()");
                            close(Mysocket);
                            exit(EXIT_FAILURE);
                        }
                        else if (retval == 0) {
                            strcpy(return_message, "there are no routes that fit those criteria"); 
                            // no possible routes
                        }








                        /* If Response, send response back and add our own Station Name to the end. */

                        else{ 
                            
                            int UDPbytes_received = recvfrom(Mysocket, queryreply, BUFFER_SIZE, 0, (struct sockaddr *)&incoming_addr, &incoming_addr_len); 
                            if (UDPbytes_received == -1){ 
                                perror("recfrom() error: "); 
                                close(Mysocket); 
                                exit(EXIT_FAILURE); 
                            }
                            queryreply[UDPbytes_received] = '\0';
                            printf("Station %s: Message received from Station %i\n", argv[3], ntohs(incoming_addr.sin_port));
                            printf("%s\n", queryreply); 

                            strcpy(return_message, queryreply); 

                            char *replyprefix = "catch";
                            if (strncmp(return_message, replyprefix, strlen(replyprefix)) == 0) {                                                           
                                if (!isdigit(return_message[strlen(return_message) - 1])) {
                                    char *by_position = strstr(return_message, "by ");
                                    if (by_position == NULL) {
                                        fprintf(stdout, "substring 'by ' error.\n"); 
                                        exit(EXIT_FAILURE); 
                                    }
                                    char *station_name = by_position + strlen("by ");

                                    int intermediate_route = -1; 
                                    char intermediate_path[4500]; 



                                    num_of_routes = readtt_file(argv[1]);                                           /* CHECK/DETECT TIMETABLE CHANGE */    
                                    for (int k = 0; k < num_of_routes; k++){
                                        if ((mystation_routes[k].departure_time >= departure_time2) && (strcmp(mystation_routes[k].arrival_station, station_name) == 0)){
                                            // if (mystation_routes[j].arrival_station > )                                                      // account for arriving past midnight (invalid)
                                            intermediate_route = k; 
                                            break; 
                                        }
                                    }
                                    if (intermediate_route == -1) {
                                        fprintf(stdout, "intermediate route error.\n"); 
                                        exit(EXIT_FAILURE); 
                                    }

                                    char formatted_time3[10];
                                    char formatted_time4[10];
                                    snprintf(formatted_time3, URL_HEADER_TIME_LEN + 10,  "%02d:%02d", mystation_routes[intermediate_route].departure_time / 100, mystation_routes[intermediate_route].departure_time % 100);
                                    snprintf(formatted_time4, URL_HEADER_TIME_LEN + 10, "%02d:%02d", mystation_routes[intermediate_route].arrival_time / 100, mystation_routes[intermediate_route].arrival_time % 100);
                                    
                                    sprintf(intermediate_path, "\n catch %s from %s, at time %s, to arrive at %s at time %s",  
                                    mystation_routes[intermediate_route].route_name, argv[1], formatted_time3, 
                                    mystation_routes[intermediate_route].arrival_station, formatted_time4); 
                                    size_t new_path_length = strlen(intermediate_path);
                                    strcpy(by_position, intermediate_path);
                                    by_position[new_path_length] = '\0';

                                    

                                }

                                strcat(return_message, " by "); 
                                strcat(return_message, argv[1]);                        // add our own station name to the end. 

                            }

                        } 
                    }
                        
                }







                /* REPLY BACK, with either No Route Found or a valid Route. */
                
                int return_message_len = strlen(return_message);
                int UDPbytes_sent = sendto(Mysocket, return_message, return_message_len, 0, (struct sockaddr *)&incoming_addr, incoming_addr_len);
                if (UDPbytes_sent == -1) {
                    perror("sendto");
                }
                fprintf(stdout, "Station%s: sent message to station%i\n", argv[3], source_port_number); 
                printf("%s\n", return_message); 
    
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










