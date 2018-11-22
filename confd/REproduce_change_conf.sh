#!/bin/bash
PID=$(pidof REproduce)
SIGNAL=SIGUSR1
if [ -n "$PID" ]; then
  kill -s $SIGNAL $PID
fi
