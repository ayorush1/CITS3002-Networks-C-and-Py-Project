
// #import <headerfiles.h> 

#define MACROS


int main(int argc, char* argv[]){ 


    intialise_tcp_port()            // done 
    inititalise_upd_port()          // done
    read_in_ttfile()                // done

    // start server 

    while(true): 

        select(tcp_port)            //done
        if (1) 
            accept()                //done
            parse_request()         //done
            

            detect_file_change()
            check_file()            //done
            if (1)                  // done (needs tweaking) 
                return route        // (almost) done
            else(
                result = udp_broadcast_neighbors(destination, distance_to_neighbor)  
                return result
            )
        
            close_connection()      // done 
        
        else(

            select(udp_port)        // done 
                if (1) 
                    detect_file_change()
                    check_file()
                    if (1) 
                        return route 
                    else( 
                        result = udp_broadcast_neightbors(destination) 
                        return result + distance_to_neighbor
                    )


        )


}