from helpers import create_response, parse_time_param, find_routes

def read_timetable_file(station_name):
    file_path = f"tt-{station_name}"
    routes = []
    try:
        with open(file_path, 'r') as file:
            print("Reading timetable file for station:", station_name)
            next(file)  # Skip station name, longitude, latitude header
            next(file)  # Skip column headers
            for line in file:
                if not line.startswith('#'):
                    parts = line.strip().split(',')
                    if len(parts) == 5:
                        routes.append({
                            'departure_time': parts[0],
                            'route_name': parts[1],
                            'departing_from': station_name,  # Use the station name instead of stopA
                            'arrival_time': parts[3],
                            'arrival_station': parts[4]
                        })
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"Failed to read timetable file: {e}")
    return routes

def handle_udp_message(message, routes, station_name, udp_socket, neighbors, ongoing_queries, sender_address=None):
    routes = read_timetable_file(station_name)  # Re-read timetable file

    print(f"Handling UDP message: {message}")
    parts = message.split()
    if len(parts) < 5:  # Ensure there are enough parts in the message
        print(f"Invalid UDP message format: {message}")
        return

    if parts[0] == "QUERY":
        if len(parts) == 6:
            _, original_source, current_source, destination_station, query_time, query_path = parts
            journey_steps = []
        else:
            _, original_source, current_source, destination_station, query_time, *journey_steps_and_path = parts
            journey_steps = []
            query_path = []
            in_query_path = False
            for part in journey_steps_and_path:
                if part == 'PATH':
                    in_query_path = True
                elif in_query_path:
                    query_path.append(part)
                else:
                    journey_steps.append(part)

        current_time = parse_time_param(query_time)
        if current_time is None:
            print(f"Invalid time format in UDP message: {query_time}")
            return

        # Check if the query has been seen before
        query_id = f"{original_source}-{destination_station}-{query_time}"
        if query_id in ongoing_queries:
            print(f"Duplicate query received: {query_id}")
            return

        found_routes = find_routes(station_name, destination_station, routes, current_time)

        if found_routes:
            response_message = f"RESPONSE {station_name} {destination_station} {query_time} {found_routes[0]['departure_time']} {found_routes[0]['route_name']} {found_routes[0]['arrival_time']} {found_routes[0]['arrival_station']} {original_source} {' '.join(journey_steps)} PATH {' '.join(query_path)}"
            print(f"Found routes: {found_routes}")
            print(f"Response message: {response_message}")

            if sender_address:
                print(f"Sending UDP response to {sender_address}")
                udp_socket.sendto(response_message.encode(), sender_address)
                print(f"Sent UDP response to {sender_address}")
            else:
                print("No sender address provided for UDP response.")
        else:
            # No direct route found, send UDP query to neighbors
            print(f"No route found from {station_name} to {destination_station} for time {query_time}")
            new_query_path = query_path if query_path else []
            new_query_path.append(f"{station_name}:{sender_address[1]}")
            new_query_message = f"QUERY {original_source} {station_name} {destination_station} {query_time} {' '.join(journey_steps)} PATH {' '.join(new_query_path)}"
            ongoing_queries[query_id] = (None, journey_steps, {station_name}, len(neighbors), current_time)  # Add a dummy entry to avoid duplicate processing
            print(f"Added query ID {query_id} to ongoing queries")
            for neighbor in neighbors:
                neighbor_address = (neighbor[0], neighbor[1])
                if not sender_address or neighbor_address != sender_address:
                    udp_socket.sendto(new_query_message.encode(), neighbor_address)
                    print(f"Sent UDP query to neighbor {neighbor_address}")

    elif parts[0] == "RESPONSE":
        _, source_station, destination_station, query_time, departure_time, route_name, arrival_time, arrival_station, original_source, *journey_steps_and_path = parts
        journey_steps = []
        query_path = []
        in_query_path = False
        for part in journey_steps_and_path:
            if part == 'PATH':
                in_query_path = True
            elif in_query_path:
                query_path.append(part)
            else:
                journey_steps.append(part)

        print(f"Received response: From {source_station} to {destination_station}, Departure at {departure_time}, Route {route_name}, Arrival at {arrival_time}, Arrival Station {arrival_station}")
        query_id = f"{original_source}-{destination_station}-{query_time}"
        if query_id in ongoing_queries:
            client_socket, ongoing_journey_steps, queried_stations, expected_responses, initial_time = ongoing_queries[query_id]
            ongoing_journey_steps.append(f"Take {route_name} from {source_station} at {departure_time} to {arrival_station}, arriving at {arrival_time}")
            expected_responses -= 1
            print(f"Updated journey steps for query ID {query_id}: {ongoing_journey_steps}")

            if arrival_station == destination_station:
                # Check if this station is the original querying station
                if station_name == original_source:
                    body = f"<html><body><h1>Journey from {original_source} to {destination_station}</h1>"
                    for step in journey_steps:
                        body += f"{step} "
                    body += "</body></html>"
                    response = create_response(200, body)
                    print(f"Sending response to HTTP client: {response}")
                    if client_socket:
                        client_socket.sendall(response.encode())
                        client_socket.close()
                    del ongoing_queries[query_id]
                else:
                    # Add the segment from this station to the previous station
                    if query_path:
                        previous_station = query_path.pop()
                        previous_station_name, previous_station_port = previous_station.split(':')
                        route_to_previous = find_routes(station_name, source_station, routes, parse_time_param(query_time))
                        print(f"route_to_previous: {route_to_previous}")  # Add debug statement
                        if route_to_previous:
                            ongoing_journey_steps.insert(0, f"Take {route_to_previous[0]['route_name']} from {station_name} at {route_to_previous[0]['departure_time']} to {source_station}, arriving at {route_to_previous[0]['arrival_time']}")
                            print(f"Added journey step: {ongoing_journey_steps[0]}")  # Add debug statement

                        # Forward the response back to the station that sent the query
                        previous_station_address = ('127.0.0.1', int(previous_station_port))
                        new_response_message = f"RESPONSE {source_station} {destination_station} {query_time} {departure_time} {route_name} {arrival_time} {arrival_station} {original_source} {' '.join(ongoing_journey_steps)} PATH {' '.join(query_path)}"
                        print(f"Forwarding response to the station that sent the query: {previous_station_address}")
                        udp_socket.sendto(new_response_message.encode(), previous_station_address)
            elif expected_responses <= 0:
                body = f"<html><body><h1>No available routes found from {original_source} to {destination_station}</h1></body></html>"
                response = create_response(404, body)
                print(f"Sending response to HTTP client: {response}")
                if client_socket:
                    client_socket.sendall(response.encode())
                    client_socket.close()
                del ongoing_queries[query_id]
            else:
                ongoing_queries[query_id] = (client_socket, ongoing_journey_steps, queried_stations, expected_responses, initial_time)
                queried_stations.add(station_name)
                new_query_message = f"QUERY {original_source} {arrival_station} {destination_station} {arrival_time} {' '.join(ongoing_journey_steps)} PATH {' '.join(query_path)}"
                new_query_id = f"{original_source}-{destination_station}-{arrival_time}"
                ongoing_queries[new_query_id] = (client_socket, ongoing_journey_steps, queried_stations, len(neighbors) - 1, initial_time)
                print(f"Updated ongoing query ID {query_id} with new journey steps and neighbors")
                for neighbor in neighbors:
                    neighbor_address = (neighbor[0], neighbor[1])
                    if sender_address != neighbor_address:  # Avoid sending back to the sender
                        udp_socket.sendto(new_query_message.encode(), neighbor_address)
                        print(f"Sent UDP query to neighbor {neighbor_address}")

        else:
            print(f"Query ID {query_id} not found in ongoing queries")
            # If not the original querying station, forward response back to the station that sent the query
            if sender_address:
                print(f"Forwarding response to the station that sent the query: {sender_address}")
                udp_socket.sendto(message.encode(), sender_address)
