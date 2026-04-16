#!/usr/bin/env bash

set -e

if [ "$#" -ne 2 ]; then
    echo "Ошибка: нужно передать ровно 2 аргумента: source_ip dest_ip" >&2
    echo "Пример: ./run_client.sh 10.200.1.1 10.200.1.2" >&2
    exit 1
fi

source_ip="$1"
dest_ip="$2"

echo "Compiling client.cpp..."
g++ client.cpp -o client -O3 -std=c++23

echo "Adding iptables rule to drop outgoing TCP RST to ${dest_ip}:3333..."
sudo iptables -A OUTPUT -p tcp --tcp-flags RST RST -d "$dest_ip" --dport 3333 -j DROP

echo "Running client..."
sudo ./client "$source_ip" "$dest_ip"