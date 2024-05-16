from helpers import create_response, parse_time_param, find_routes


def handle_udp_message(message, routes, my_station, udp_socket, neighbors, ongoing_queries, sender_address=None):
    print(f"Handling UDP message: {message}")
    parts = message.split()
    if len(parts) < 4:
        print(f"Invalid UDP message format: {message}")
        return
    
    if parts[0] == "QUERY":
        _, source_station, destination_station, query_time = parts
        current_time = parse_time_param(query_time)
        if current_time is None:
            print(f"Invalid time format in UDP message: {query_time}")
            return

        found_routes = find_routes(my_station, destination_station, routes, current_time)
        if found_routes:
            
            response_message = f"RESPONSE {my_station} {destination_station} {query_time} {found_routes[0]['departure_time']} {found_routes[0]['route_name']} {found_routes[0]['arrival_time']} {found_routes[0]['arrival_station']}"
            print(f"Found routes: {found_routes}")
            print(f"Response message: {response_message}")
            
            if sender_address:

                print(f"Sending UDP response to {sender_address}")
                udp_socket.sendto(response_message.encode(), sender_address)
                print(f"Sent UDP response to {sender_address}")

            else:
                print("No sender address provided for UDP response.")

        else:
            print(f"No route found from {my_station} to {destination_station} for time {query_time}")



    elif parts[0] == "RESPONSE":
        _, source_station, destination_station, query_time, departure_time, route_name, arrival_time, arrival_station = parts
        print(f"Received response: From {source_station} to {destination_station}, Departure at {departure_time}, Route {route_name}, Arrival at {arrival_time}, Arrival Station {arrival_station}")
        query_id = f"{my_station}-{destination_station}-{query_time}"
        if query_id in ongoing_queries:
            client_socket = ongoing_queries.pop(query_id)
            body = f"<html><body><h1>Available Route from {source_station} to {destination_station}</h1>"
            body += f"<p>{departure_time} from {source_station} to {arrival_station} arriving at {arrival_time} via {route_name}</p>"
            body += "</body></html>"
            response = create_response(200, body)
            print(f"Sending response to HTTP client: {response}")
            client_socket.sendall(response.encode())
            client_socket.close()
        else:
            print(f"Query ID {query_id} not found in ongoing queries")
