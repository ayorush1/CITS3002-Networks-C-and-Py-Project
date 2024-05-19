import datetime
from helpers import create_response, parse_time_param, parse_path, find_routes, parse_request
import socket
from udp_handling import handle_udp_message
import time
import select


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


# Handler Functions
import select

import urllib.parse

def handle_query(request, query_params, station_name, udp_socket, neighbors, client_socket):
    # Re-read the timetable file for the station
    routes = read_timetable_file(station_name)

    to_station = query_params.get('to')
    if not to_station:
        response = create_response(400, "<html><body><h1>Bad Request</h1><p>Missing 'to' parameter.</p></body></html>")
        client_socket.sendall(response.encode())
        client_socket.close()
        return

    # Get current time from query parameters or use the system's current time
    leave_param = query_params.get('leave')
    if leave_param:
        leave_param = urllib.parse.unquote(leave_param)  # Decode %3A to :
        current_time = parse_time_param(leave_param)
        if current_time is None:
            response = create_response(400, "<html><body><h1>Bad Request</h1><p>Invalid 'leave' parameter.</p></body></html>")
            client_socket.sendall(response.encode())
            client_socket.close()
            return
    else:
        current_time = datetime.datetime.now().time()

    print(f"Query received: to={to_station} from {station_name} at {current_time}")

    # Check for direct route
    found_routes = find_routes(station_name, to_station, routes, current_time)
    if found_routes:
        body = f"<html><body><h1>Available Routes from {station_name} to {to_station}</h1>"
        for route in found_routes:
            body += f"<p>{route['departure_time']} from {route['departing_from']} to {route['arrival_station']} arriving at {route['arrival_time']}</p>"
        body += "</body></html>"
        response = create_response(200, body)
        client_socket.sendall(response.encode())
        client_socket.close()
    else:
        # No direct route found, send UDP query to neighbors
        query_message = f"QUERY {station_name} {to_station} {current_time.strftime('%H:%M')}"
        for neighbor in neighbors:
            udp_socket.sendto(query_message.encode(), neighbor)
            print(f"Sent UDP query to neighbor {neighbor}")

        # Use select to wait for a response for up to 15 seconds
        ready = select.select([udp_socket], [], [], 15)
        if ready[0]:
            data, addr = udp_socket.recvfrom(1024)
            print(f"Received response from {addr}: {data.decode()}")
            final_response = handle_udp_message(data.decode(), routes, station_name, udp_socket, neighbors, sender_address=addr)
            if final_response:
                body = f"<html><body><h1>Journey from {station_name} to {to_station}</h1>"
                body += f"<p>{final_response}</p>"
                body += "</body></html>"
                response = create_response(200, body)
                client_socket.sendall(response.encode())
                client_socket.close()
        else:
            print("No response received within the timeout period.")
            body = f"<html><body><h1>No available routes found from {station_name} to {to_station}</h1></body></html>"
            response = create_response(404, body)
            client_socket.sendall(response.encode())
            client_socket.close()



def handle_root(request):
    body = "<html><body><h1>Welcome to the Home Page!</h1></body></html>"
    return create_response(200, body)

def handle_about(request):
    body = "<html><body><h1>About Us</h1><p>Here is some information about our company.</p></body></html>"
    return create_response(200, body)

def handle_404(request):
    body = "<html><body><h1>404 Not Found</h1><p>The requested resource was not found on this server.</p></body></html>"
    return create_response(404, body)

# Route Mapping
ROUTES = {
    '/': handle_root,
    '/about': handle_about
}

# Request Dispatching
def dispatch_request(method, path, request):
    if path in ROUTES:
        return ROUTES[path](request)
    else:
        return handle_404(request)

def parse_headers(header_lines):
    headers = {}
    current_header_key = None
    current_header_value = []
    for line in header_lines:
        if line.startswith((' ', '\t')) and current_header_key:
            current_header_value.append(line.strip())
        else:
            if current_header_key:
                headers[current_header_key] = ' '.join(current_header_value)
            if ': ' in line:
                current_header_key, header_value_part = line.split(': ', 1)
                current_header_key = current_header_key.lower()
                current_header_value = [header_value_part.strip()]
    if current_header_key:
        headers[current_header_key] = ' '.join(current_header_value)
    return headers