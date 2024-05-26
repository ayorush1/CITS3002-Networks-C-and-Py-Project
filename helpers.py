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

# Timetable look up Parsing Query Strings
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

def find_routes1(from_station, to_station, routes, current_time):
    results = []
    print("From station:", from_station)
    print("To station:", to_station)
    for route in routes[::-1]:
        arrival_time_str = route['departure_time']
        arrival_time = datetime.datetime.strptime(arrival_time_str, "%H:%M").time()
        if route['departing_from'] == from_station and route['arrival_station'] == to_station and arrival_time <= current_time:
            results.append(route)
    return results


def parse_request(request_text):
    """
    Parses an HTTP request text into its components and returns them.
    Handles extraction of the method, path, version, headers, and possibly a body.
    """
    #print("Parsing request:", request_text)
    headers = {}
    lines = request_text.split('\r\n')
    method, path, version = lines[0].split()

    i = 1
    while i < len(lines) and lines[i]:
        header_key, header_value = lines[i].split(': ', 1)
        headers[header_key.lower()] = header_value
        i += 1

    body = '\r\n'.join(lines[i+1:]) if i + 1 < len(lines) else ''

    #print("Parsed method:", method)
    #print("Parsed path:", path)
    #print("Parsed version:", version)
    #print("Parsed headers:", headers)
    #print("Parsed body:", body)

    return method, path, version, headers, body
