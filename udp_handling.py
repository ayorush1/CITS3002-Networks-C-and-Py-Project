from helpers import create_response, parse_time_param, find_routes, find_routes1
import select



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

import select
import socket
import datetime
import select
import socket
import datetime
def handle_udp_message(message, routes, station_name, udp_socket, neighbors, sender_address=None):
    routes = read_timetable_file(station_name)  # Re-read timetable file

    print(f"Handling UDP message: {message}")
    parts = message.split()
    if len(parts) < 4:  # Ensure there are enough parts in the message
        print(f"Invalid UDP message format: {message}")
        return

    if parts[0] == "QUERY":
        _, original_source, destination_station, query_time = parts


        current_time = parse_time_param(query_time)
        if current_time is None:
            print(f"Invalid time format in UDP message: {query_time}")
            return

        current_datetime = datetime.datetime.combine(datetime.date.today(), current_time)
        current_datetime = current_datetime + datetime.timedelta(hours=1, minutes=10)
        current_time = current_datetime.time() 

        query_time = current_time.strftime("%H:%M") 
        found_routes = find_routes(station_name, destination_station, routes, current_time)

        if found_routes:
            response_message = f"catch {found_routes[0]['route_name']} from {station_name} at {found_routes[0]['departure_time']} to arrive at {found_routes[0]['arrival_station']} at {found_routes[0]['arrival_time']} by {station_name}"
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
            new_query_message = f"QUERY {station_name} {destination_station} {query_time}"
            for neighbor in neighbors:
                neighbor_address = (socket.gethostbyname(neighbor[0]), int(neighbor[1]))
                if not sender_address or neighbor_address != sender_address:
                    udp_socket.sendto(new_query_message.encode(), neighbor_address)
                    print(f"Sent UDP query to neighbor {neighbor_address}")

            # Use select to wait for a response for up to 15 seconds
            ready = select.select([udp_socket], [], [], 15)
            if ready[0]:
                data, addr = udp_socket.recvfrom(1024)
                print(f"Received response from {addr}: {data.decode()}")
                new_message = handle_udp_message(data.decode(), routes, station_name, udp_socket, neighbors, sender_address=addr)
                if new_message:
                    udp_socket.sendto(new_message.encode(), sender_address)
            else:
                print("No response received within the timeout period.")
                if sender_address:
                    response_message = f"No available routes found from {station_name} to {destination_station}"
                    udp_socket.sendto(response_message.encode(), sender_address)

    elif parts[0] == "catch":
        # Extract each "catch" part
        catch_parts = message.split(" catch ")
        catch_parts = [f"catch {part}" if not part.startswith("catch") else part for part in catch_parts]

        print(f"Catch parts: {catch_parts}")

        if not catch_parts:
            print("No valid catch parts found.")
            return

        # Process the last "catch" part to find the previous station
        last_catch_part = catch_parts[-1]
        last_catch_parts = last_catch_part.split()
        
        if len(last_catch_parts) < 9:
            print(f"Invalid last catch format: {last_catch_parts}")
            return

        previous_station_name = last_catch_parts[-1]
        print(f"Previous station name: {previous_station_name}")

        # Finding route back to the previous station
        current_t  = parse_time_param(last_catch_parts[6][:-1])
        if current_t is None:
            print(f"Invalid time format in UDP message. Ignoring Query")
            return
        print("current_t: ", current_t)
        found_routes = find_routes1(station_name, previous_station_name, routes, current_t)
        if found_routes:
            new_route = f"catch {found_routes[0]['route_name']} from {station_name} at {found_routes[0]['departure_time']} to arrive at {found_routes[0]['arrival_station']} at {found_routes[0]['arrival_time']}"
            response_message = message.replace(f"by {previous_station_name}", "")
            response_message = f"{response_message}\n {new_route} by {station_name}"
            print(f"Revised response: {response_message}")

            return response_message
        else:
            print(f"No route found from {station_name} to {previous_station_name}")
            response_message = f"No route found from {station_name} to {previous_station_name}"
            return response_message 



