#!/bin/bash

PORT=$1

if [ -z "$PORT" ]; then
  echo "Usage: $0 port_number"
  exit 1
fi

PID=$(lsof -t -i :$PORT)
if [ -z "$PID" ]; then
  echo "No process found using port $PORT"
else
  kill -9 $PID
  echo "Killed process $PID using port $PORT"
fi