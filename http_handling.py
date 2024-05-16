import datetime

# Response Utility Functions
def generate_status_line(status_code):
    """ Generate the status line with corresponding textual description. """
    status_texts = {
        200: 'OK',
        404: 'Not Found',
        500: 'Internal Server Error'
    }
    return f"HTTP/1.1 {status_code} {status_texts.get(status_code, 'Unknown')}"

# Timetable look up Parsing Query Strings
# 
def parse_path(path):
    """ Parse the path to separate the endpoint and the query string. """
    if '?' in path:
        path, query_string = path.split('?', 1)
        query_params = dict(q.split('=') for q in query_string.split('&') if '=' in q)
    else:
        query_params = {}
    return path, query_params

def parse_time_param(time_str):
    """ Parse the time parameter in HH:MM format. """
    try:
        return datetime.datetime.strptime(time_str, "%H:%M").time()
    except ValueError:
        return None
    

def find_routes(from_station, to_station, routes, current_time):
    results = []
    print("From station:", from_station)
    print("To station:", to_station)
    for route in routes:
        departure_time_str = route['departure_time']
        departure_time = datetime.datetime.strptime(departure_time_str, "%H:%M").time()
        if route['departing_from'] == from_station and route['arrival_station'] == to_station and departure_time >= current_time:
            results.append(route)
    return results

def generate_headers(headers_dict):
    """ Generate the formatted headers from a dictionary. """
    headers = ""
    for key, value in headers_dict.items():
        headers += f"{key}: {value}\r\n"
    return headers

def create_response(status_code, body, content_type='text/html'):
    headers = {
        'Content-Type': content_type,
        'Content-Length': str(len(body)),
        'Connection': 'close'
    }
    status_line = generate_status_line(status_code)
    headers_section = generate_headers(headers)
    return f"{status_line}\r\n{headers_section}\r\n{body}"

# Handler Functions
def handle_query(request, query_params, routes, my_station, udp_socket, neighbors, ongoing_queries, client_socket):
    to_station = query_params.get('to')
    if not to_station:
        response = create_response(400, "<html><body><h1>Bad Request</h1><p>Missing 'to' parameter.</p></body></html>")
        client_socket.sendall(response.encode())
        client_socket.close()
        return

    # Get current time from query parameters or use the system's current time
    time_param = query_params.get('time')
    if time_param:
        current_time = parse_time_param(time_param)
        if current_time is None:
            response = create_response(400, "<html><body><h1>Bad Request</h1><p>Invalid 'time' parameter.</p></body></html>")
            client_socket.sendall(response.encode())
            client_socket.close()
            return
    else:
        current_time = datetime.datetime.now().time()

    print(f"Query received: to={to_station} from {my_station} at {current_time}")

    found_routes = find_routes(my_station, to_station, routes, current_time)
    if found_routes:
        body = f"<html><body><h1>Available Routes from {my_station} to {to_station}</h1>"
        for route in found_routes:
            body += f"<p>{route['departure_time']} from {route['departing_from']} to {route['arrival_station']} arriving at {route['arrival_time']}</p>"
        body += "</body></html>"
        response = create_response(200, body)
        client_socket.sendall(response.encode())
        client_socket.close()
    else:
        # No direct route found, send UDP query to neighbors
        query_message = f"QUERY {my_station} {to_station} {current_time.strftime('%H:%M')}"
        query_id = f"{my_station}-{to_station}-{current_time.strftime('%H:%M')}"
        ongoing_queries[query_id] = client_socket
        print(f"Query ID: {query_id}")
        print(f"Neighbors: {neighbors}")  # Debug statement to print neighbors before sending UDP query
        for neighbor in neighbors:
            udp_socket.sendto(query_message.encode(), neighbor)
            print(f"Sent UDP query to neighbor {neighbor}")

# Assuming the rest of the code is unchanged


def handle_udp_message(message, routes, my_station, udp_socket, neighbors, ongoing_queries):
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
            for neighbor in neighbors:
                if neighbor[0] == source_station:
                    udp_socket.sendto(response_message.encode(), (neighbor[0], neighbor[1]))
                    print(f"Sent UDP response to {neighbor}")
        else:
            print(f"No route found from {my_station} to {destination_station} for time {query_time}")

    elif parts[0] == "RESPONSE":
        _, source_station, destination_station, query_time, departure_time, route_name, arrival_time, arrival_station = parts
        print(f"Received response: From {source_station} to {destination_station}, Departure at {departure_time}, Route {route_name}, Arrival at {arrival_time}, Arrival Station {arrival_station}")
        query_id = f"{source_station}-{destination_station}-{query_time}"
        if query_id in ongoing_queries:
            client_socket = ongoing_queries.pop(query_id)
            body = f"<html><body><h1>Available Route from {source_station} to {destination_station}</h1>"
            body += f"<p>{departure_time} from {source_station} to {arrival_station} arriving at {arrival_time} via {route_name}</p>"
            body += "</body></html>"
            response = create_response(200, body)
            client_socket.sendall(response.encode())
            client_socket.close()





def parse_request(request_text):
    """
    Parses an HTTP request text into its components and returns them.
    Handles extraction of the method, path, version, headers, and possibly a body.
    """
    print("Parsing request:", request_text)
    headers = {}
    lines = request_text.split('\r\n')
    method, path, version = lines[0].split()
    
    i = 1
    while i < len(lines) and lines[i]:
        header_key, header_value = lines[i].split(': ', 1)
        headers[header_key.lower()] = header_value
        i += 1

    body = '\r\n'.join(lines[i+1:]) if i + 1 < len(lines) else ''

    print("Parsed method:", method)
    print("Parsed path:", path)
    print("Parsed version:", version)
    print("Parsed headers:", headers)
    print("Parsed body:", body)
    
    return method, path, version, headers, body

# Additional original functions
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