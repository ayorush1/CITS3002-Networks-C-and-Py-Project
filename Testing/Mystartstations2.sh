./station-server StationA 4001 4002 localhost:4006 localhost:4008 &
./station-server TerminalB 4003 4004 localhost:4006 &
python3 station-server.py JunctionC 4005 4006 localhost:4002 localhost:4004 &
python3 station-server.py BusportD 4007 4008 localhost:4002 &