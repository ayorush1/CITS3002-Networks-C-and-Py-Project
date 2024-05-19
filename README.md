#PYTHON STATION. 

### Start a Station Server
python3 python_station.py [station-name] [tcp-port] [udp-port] [neighbour1-ip]:[neighbour1-udp] [neighbour2... ]

### Example: 

python3 python_station.py TerminalA 8080 5000 127.0.0.1:5001 127.0.0.1:5002

python3 python_station.py TerminalB 8081 5001 127.0.0.1:5003

python3 python_station.py TerminalC 8082 5002 127.0.0.1:5004

python3 python_station.py TerminalD 8083 5003 127.0.0.1:5005

python3 python_station.py TerminalE 8084 5004 127.0.0.1:5005

python3 python_station.py TerminalF 8085 5005 127.0.0.1:5006

Test it using:
curl "http://127.0.0.1:8080/query?to=TerminalF&time=00:00"




FOR handle_udp_message: 

# Use select to wait for a response for up to 15 seconds
            ready = select.select([udp_socket], [], [], 15)
            if ready[0]:
                data, addr = udp_socket.recvfrom(1024)
                print(f"Received response from {addr}: {data.decode()}")
                handle_udp_message(data.decode(), routes, station_name, udp_socket, neighbors, sender_address)
            else:
                print("No response received within the timeout period.")

# Assume other functions like read_timetable_file, parse_time_param, find_routes are defined elsewhere



FOR handle_query: 



        # Use select to wait for a response for up to 15 seconds
        ready = select.select([udp_socket], [], [], 15)
        if ready[0]:
            data, addr = udp_socket.recvfrom(1024)
            print(f"Received response from {addr}: {data.decode()}")
            handle_udp_message(data.decode(), routes, station_name, udp_socket, neighbors)
        else:
            print("No response received within the timeout period.")
            response = create_response(404, "<html><body><h1>Not Found</h1><p>No routes available at this time.</p></body></html>")
            client_socket.sendall(response.encode())
            client_socket.close()
