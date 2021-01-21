#!/bin/bash
while ! ./src/udpserver -i ens5
do
  sleep 1
  echo "Restarting program..."
done