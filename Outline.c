/* CITS3002 COMPUTER NETWORKS PROJECT 2024*/ 

/* 
student: Arush Kathal 23730792 
student: Armaan Josan 24001588 
*/


B(c) -> C(py) -> A(c) -> D(py)

/* NOT ACTUALLY RUNNABLE CODE. JUST A HELPFUL GUIDELINE TO STRUCTURE OF STATION-SERVER.C */


/* changes: 
   * udp-handling line72, changed neighbor[0] to socket.gethostbyname(neighbor[0])
   * udp-handling line116, changed found_routes = find_routes(station_name, previous_station_name, routes, parse_time_param(last_catch_parts[5])) to:  

        current_t  = parse_time_param(last_catch_parts[6][:-1])
        if current_t is None:
            print(f"Invalid time format in UDP message. Ignoring Query")
            return
        found_routes = find_routes(station_name, previous_station_name, routes, current_t)
   * udp-handling line124: changed response_message = message.replace(f"by {station_name}", "") to previous_station_name. 

    (at this point, 3 hops and returned tcp message working but not correct route). 

   * udp-handling line49: added: 
        current_time = parse_time_param(query_time)
        if current_time is None:
            print(f"Invalid time format in UDP message: {query_time}")
            return
        
        current_datetime = datetime.datetime.combine(datetime.date.today(), current_time)
        current_datetime = current_datetime + datetime.timedelta(hours=1, minutes=30)
        current_time = current_datetime.time() 

        query_time = current_time.strftime("%H:%M")

        found_routes = find_routes(station_name, destination_station, routes, current_time)
        

   * udp-handling line131: changed found_routes = find_routes1(station_name, previous_station_name, routes, current_t) to found_routes1. (must import). 
        changed found_routes1 to: 
            def find_routes1(from_station, to_station, routes, current_time):
                results = []
                print("From station:", from_station)
                print("To station:", to_station)
                for route in routes:
                    arrival_time_str = route['departure_time']
                    arrival_time = datetime.datetime.strptime(arrival_time_str, "%H:%M").time()
                    if route['departing_from'] == from_station and route['arrival_station'] == to_station and arrival_time <= current_time:
                        results.append(route)
                return results
    -> change so that intermediate station finds journey with arrival time less than depart time, instead of depart time > original query departure time. 
   * -> also changed find_routes1 from 'for route in routes' to 'for route in routes[::-1]' in order to search from last route first. 

   * http_handling line91: added final_response = final_response.replace('\n', '<br>') 
   * station-server.c line831: changed departure_time2 to 'int departure_time2 = ((((departure_hour * 60 + departure_minute + 90) / 60) % 24) * 100) + ((departure_minute + 90) % 60);'
   * station-server.c line828: added 'continue;' after "printf(stdout, "station%s: could not successfuly scan QUERY. ignoring request.\n", argv[3]); ". 
   * station-server.c line831: some changes accounting for over 24h


   * need to change: C code intermediate error ignoring request rather than closing down. 
   */




#import <headerfiles.h> 

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
            if (1)                  // done 
                return route        // done
            else(
                result = udp_broadcast_neightbors(destination) 
                return result + distance_to_neighbor
            )
        
            close_connection()      // done 
        
        else(

            select(udp_port)        // done 
                if (1) 
                    detect_file_change()
                    check_file()
                    if (1) 
                        return route                                        // done
                    else( 
                        result = udp_broadcast_neightbors(destination) 
                        return result + distance_to_neighbor
                    )


        )


}