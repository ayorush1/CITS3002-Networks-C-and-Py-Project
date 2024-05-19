./station_server StationA 4001 4002 localhost:4006 localhost:4008 &
./station_server TerminalB 4003 4004 localhost:4006 &
./station_server JunctionC 4005 4006 localhost:4002 localhost:4004 &
./station_server BusportD 4007 4008 localhost:4002 &
