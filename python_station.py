from http_handling import parse_request, dispatch_request, handle_query, parse_path, create_response, parse_time_param


import socket
import select
import sys
import logging
import datetime

logging.basicConfig(level=logging.DEBUG, format='%(asctime)s - %(levelname)s - %(message)s')



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


def main():
    if len(sys.argv) != 5:
        print("Incorrect usage. Usage: python station_server.py [station_name] [tcp_port] [myUDP_port] [station2_udp_port]")
        sys.exit(1)

    station_name, tcp_port, my_udp_port, station2_udp_port = sys.argv[1:5]
    print(f"Starting server for {station_name} on TCP port {tcp_port} and UDP port {my_udp_port}")
    
    routes = read_timetable_file(station_name)

    tcp_socket = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
    tcp_socket.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
    tcp_socket.bind(('127.0.0.1', int(tcp_port)))
    tcp_socket.listen(5)

    udp_socket = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    udp_socket.bind(('127.0.0.1', int(my_udp_port)))

    sockets_list = [tcp_socket, udp_socket]
    print(f"HTTP Server Running on port {tcp_port}")

    try:
        while True:
            read_sockets, _, exception_sockets = select.select(sockets_list, [], sockets_list)
            for notified_socket in read_sockets:
                if notified_socket == tcp_socket:
                    client_socket, client_address = tcp_socket.accept()
                    sockets_list.append(client_socket)
                    print(f"Accepted new connection from {client_address}")
                elif notified_socket == udp_socket:
                    data, address = udp_socket.recvfrom(1024)
                    print(f"Received UDP message from {address}: {data.decode()}")
                else:
                    message = notified_socket.recv(1024)
                    if not message:
                        sockets_list.remove(notified_socket)
                        notified_socket.close()
                        continue

                    try:
                        method, path, version, headers, body = parse_request(message.decode())
                        if method is None:
                            continue

                        path, query_params = parse_path(path)
                        print(f"Parsed query parameters: {query_params}")

                        if 'to' in query_params:
                            response = handle_query(body, query_params, routes, station_name)
                        else:
                            response = dispatch_request(method, path, body)
                        notified_socket.sendall(response.encode())
                    except Exception as e:
                        print(f"Error handling request: {e}")
                        notified_socket.close()
                        if notified_socket in sockets_list:
                            sockets_list.remove(notified_socket)

            for notified_socket in exception_sockets:
                if notified_socket in sockets_list:
                    sockets_list.remove(notified_socket)
                notified_socket.close()

    finally:
        for sock in sockets_list:
            sock.close()

if __name__ == '__main__':
    main()