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
