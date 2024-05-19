from helpers import create_response, parse_time_param, find_routes

def read_timetable_file(station_name):
    file_path = f"tt-{station_name}"
    routes = []
    try:
        with open(file_path, 'r') as file:
            print(f"Reading timetable file for station: {station_name}")
            next(file)  # Skip station name, longitude, latitude header
            next(file)  # Skip column headers
            for line in file:
                if not line.startswith('#'):
                    parts = line.strip().split(',')
                    if len(parts) == 5:
                        routes.append({
                            'departure_time': parts[0],
                            'route_name': parts[1],
                            'departing_from': station_name,
                            'arrival_time': parts[3],
                            'arrival_station': parts[4]
                        })
    except FileNotFoundError:
        print(f"File {file_path} not found.")
    except Exception as e:
        print(f"Failed to read timetable file: {e}")
    return routes

def handle_udp_message(message, routes, station_name, udp_socket, neighbors, sender_address=None):
    routes = read_timetable_file(station_name)  # Re-read timetable file

    print(f"Handling UDP message: {message}")
    parts = message.split()
    if len(parts) < 4:  # Ensure there are enough parts in the message
        print(f"Invalid UDP message format: {message}")
        return

    if parts[0] == "QUERY":
        if len(parts) == 4:
            _, current_source, destination_station, query_time = parts
            query_path = []
        else:
            _, current_source, destination_station, query_time, *query_path = parts

        current_time = parse_time_param(query_time)
        if current_time is None:
            print(f"Invalid time format in UDP message: {query_time}")
            return

        # Check for direct route
        found_routes = find_routes(station_name, destination_station, routes, current_time)
        if found_routes:
            response_message = f"catch {found_routes[0]['route_name']} from {found_routes[0]['departing_from']} at {found_routes[0]['departure_time']} to arrive at {found_routes[0]['arrival_station']} at {found_routes[0]['arrival_time']} by {station_name}"
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
            new_query_path = query_path + [station_name]
            new_query_message = f"QUERY {station_name} {destination_station} {query_time} {' '.join(new_query_path)}"
            print(f"Sending new query: {new_query_message}")
            for neighbor in neighbors:
                neighbor_address = (neighbor[0], int(neighbor[1]))
                if not sender_address or neighbor_address != sender_address:
                    udp_socket.sendto(new_query_message.encode(), neighbor_address)
                    print(f"Sent UDP query to neighbor {neighbor_address}")

    elif parts[0] == "catch":
        try:
            print(f"Received response: {message}")

            # Extract previous station's name from the response message
            response_parts = message.split(" by ")
            if len(response_parts) > 1:
                previous_station_name = response_parts[-1].strip()
                print(f"Previous station: {previous_station_name}")
            else:
                print("Invalid response format")
                return

            # Extract relevant information from the message
            catch_parts = message.split()
            print(f"Catch parts: {catch_parts}")

            # Correctly identify the arrival station and arrival time
            source_station = catch_parts[3]
            departure_time = catch_parts[5]
            route_name = catch_parts[1]
            arrival_station = catch_parts[9]
            arrival_time = catch_parts[11]

            print(f"Source station: {source_station}, Departure time: {departure_time}, Route name: {route_name}, Arrival station: {arrival_station}, Arrival time: {arrival_time}")

            # Parse the departure time
            parsed_departure_time = parse_time_param(departure_time)
            print(f"Parsed departure time: {parsed_departure_time}")

            # Find route from the current station to the previous station
            found_routes = find_routes(station_name, source_station, routes, parsed_departure_time)
            print(f"Found routes: {found_routes}")
            
            if found_routes:
                route_to_previous = found_routes[0]
                catch_message = f"catch {route_to_previous['route_name']} from {station_name} at {route_to_previous['departure_time']} to arrive at {route_to_previous['arrival_station']} at {route_to_previous['arrival_time']}"
                response_message = message.replace(f"by {previous_station_name}", catch_message)
                response_message += f" by {station_name}"
                print(f"Updated response message: {response_message}")

                # Send the updated response back to the previous station
                previous_station_address = next((neighbor for neighbor in neighbors if neighbor[0] == previous_station_name), None)
                if previous_station_address:
                    previous_station_address = ('127.0.0.1', int(previous_station_address[1]))
                    print(f"Forwarding response to {previous_station_address}")
                    udp_socket.sendto(response_message.encode(), previous_station_address)
            else:
                print(f"No route found from {station_name} to {source_station}")
        except Exception as e:
            print(f"Error processing response: {e}")

